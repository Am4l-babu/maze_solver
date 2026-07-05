// ============================================================
// TEST 03 — Motor + encoder bring-up (forward / back / speed)
// Drives each motor individually, then both together, then a
// PWM speed sweep, printing encoder-measured mm and mm/s at
// every step. Confirms direction wiring, encoder polarity, and
// gives real numbers to sanity-check the gear-ratio assumption.
//
// SAFETY: put the chassis on a stand so the wheels spin free —
// this sequence WILL drive both motors.
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <ESP32Encoder.h>

// ---- pins (must match the real robot — see tests/README.md) ----
#define PIN_SDA     5    // XIAO D4
#define PIN_SCL     6    // XIAO D5
#define PIN_PWM_L   1    // XIAO D0
#define PIN_PWM_R   2    // XIAO D1
#define PIN_ENC_LA  3    // XIAO D2
#define PIN_ENC_LB  4    // XIAO D3
#define PIN_ENC_RA  43   // XIAO D6
#define PIN_ENC_RB  44   // XIAO D7
#define PIN_BTN     9    // XIAO D10

#define ADDR_MCP23017 0x20
#define MCP_AIN1 0
#define MCP_AIN2 1
#define MCP_BIN1 2
#define MCP_BIN2 3
#define MCP_STBY 4

// ---- physical constants (match src/config.h — verify these!) ----
const float GEAR_RATIO      = 150.0f;   // datasheet conflict: 100:1 vs 150:1
const float ENCODER_PPR     = 3.0f;
const float COUNTS_PER_REV  = GEAR_RATIO * ENCODER_PPR * 4.0f; // x4 quadrature
const float WHEEL_DIA_MM    = 43.0f;
const float WHEEL_CIRC_MM   = PI * WHEEL_DIA_MM;
const float MM_PER_COUNT    = WHEEL_CIRC_MM / COUNTS_PER_REV;

Adafruit_MCP23X17 mcp;
ESP32Encoder encL, encR;

// LEDC API differs between Arduino-ESP32 core 2.x and 3.x.
#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void pwmInit(uint8_t pin)              { ledcAttach(pin, 20000, 8); }
static void pwmWrite(uint8_t pin, uint32_t d) { ledcWrite(pin, d); }
#else
static constexpr uint8_t CH_L = 0, CH_R = 1;
static void pwmInit(uint8_t pin) {
    uint8_t ch = (pin == PIN_PWM_L) ? CH_L : CH_R;
    ledcSetup(ch, 20000, 8);
    ledcAttachPin(pin, ch);
}
static void pwmWrite(uint8_t pin, uint32_t d) {
    ledcWrite((pin == PIN_PWM_L) ? CH_L : CH_R, d);
}
#endif

void setDir(bool leftMotor, int8_t dir) { // dir: +1 fwd, -1 rev, 0 brake
    uint8_t in1 = leftMotor ? MCP_AIN1 : MCP_BIN1;
    uint8_t in2 = leftMotor ? MCP_AIN2 : MCP_BIN2;
    if (dir > 0)      { mcp.digitalWrite(in1, HIGH); mcp.digitalWrite(in2, LOW);  }
    else if (dir < 0) { mcp.digitalWrite(in1, LOW);  mcp.digitalWrite(in2, HIGH); }
    else              { mcp.digitalWrite(in1, LOW);  mcp.digitalWrite(in2, LOW);  } // brake/coast
}

void drive(int8_t dirL, uint8_t pwmL, int8_t dirR, uint8_t pwmR) {
    setDir(true, dirL);  pwmWrite(PIN_PWM_L, dirL == 0 ? 0 : pwmL);
    setDir(false, dirR); pwmWrite(PIN_PWM_R, dirR == 0 ? 0 : pwmR);
}

void stopAll() { drive(0, 0, 0, 0); }

// Run one motor for durationMs at the given PWM/direction, printing
// distance and speed derived from the encoder before/after.
void runStep(const char* label, bool isLeft, int8_t dir, uint8_t pwm, uint32_t durationMs) {
    ESP32Encoder& enc = isLeft ? encL : encR;
    long before = enc.getCount();

    if (isLeft) drive(dir, pwm, 0, 0);
    else        drive(0, 0, dir, pwm);

    delay(durationMs);
    stopAll();
    delay(200); // let it fully stop before reading

    long after = enc.getCount();
    long counts = after - before;
    float mm = counts * MM_PER_COUNT;
    float mm_s = mm / (durationMs / 1000.0f);

    Serial.printf("  %-22s counts=%6ld  dist=%7.1f mm  speed~=%6.1f mm/s\n",
                  label, counts, mm, mm_s);
}

void waitForButton(const char* prompt) {
    Serial.println(prompt);
    while (digitalRead(PIN_BTN) == HIGH) delay(20);
    while (digitalRead(PIN_BTN) == LOW)  delay(20); // wait for release
    delay(100);
}

void setup() {
    Serial.begin(115200);
    delay(300);
    pinMode(PIN_BTN, INPUT_PULLUP);

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);

    if (!mcp.begin_I2C(ADDR_MCP23017, &Wire)) {
        Serial.println("[FAIL] MCP23017 not found at 0x20 — check wiring.");
        while (true) delay(1000);
    }
    for (uint8_t p : {MCP_AIN1, MCP_AIN2, MCP_BIN1, MCP_BIN2, MCP_STBY})
        mcp.pinMode(p, OUTPUT);
    mcp.digitalWrite(MCP_STBY, HIGH); // TB6612 enabled (active low standby)

    pwmInit(PIN_PWM_L);
    pwmInit(PIN_PWM_R);
    stopAll();

    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    encL.attachFullQuad(PIN_ENC_LA, PIN_ENC_LB);
    encR.attachFullQuad(PIN_ENC_RB, PIN_ENC_RA); // swapped: right motor is mirrored
    encL.clearCount();
    encR.clearCount();

    Serial.println("========================================================");
    Serial.println(" TEST 03 - Motor + Encoder bring-up");
    Serial.println(" Put the chassis on a stand (wheels off the ground).");
    Serial.printf(" MM_PER_COUNT = %.4f  (gear ratio assumed %.0f:1 -- verify!)\n",
                  MM_PER_COUNT, GEAR_RATIO);
    Serial.println("========================================================");
    waitForButton("Press START to begin the automatic test sequence...");
}

void loop() {
    static bool done = false;
    if (done) { delay(1000); return; }

    const uint8_t PWM_TEST = 160; // ~63% duty, moderate bench-test speed

    Serial.println("\n-- Individual motor direction test --");
    runStep("LEFT  forward", true,  +1, PWM_TEST, 1000);
    delay(300);
    runStep("LEFT  backward", true, -1, PWM_TEST, 1000);
    delay(300);
    runStep("RIGHT forward", false, +1, PWM_TEST, 1000);
    delay(300);
    runStep("RIGHT backward", false, -1, PWM_TEST, 1000);
    delay(300);

    Serial.println("\n-- Straight-line check (both motors together) --");
    {
        long beforeL = encL.getCount(), beforeR = encR.getCount();
        drive(+1, PWM_TEST, +1, PWM_TEST);
        delay(1500);
        stopAll();
        delay(200);
        float mmL = (encL.getCount() - beforeL) * MM_PER_COUNT;
        float mmR = (encR.getCount() - beforeR) * MM_PER_COUNT;
        float mismatchPct = (mmR != 0) ? 100.0f * (mmL - mmR) / mmR : 0;
        Serial.printf("  L=%.1f mm  R=%.1f mm  mismatch=%.1f%%"
                      " (>10%% suggests drive/friction imbalance)\n",
                      mmL, mmR, mismatchPct);
    }

    Serial.println("\n-- Short-brake stopping distance --");
    {
        long before = encL.getCount();
        drive(+1, 255, +1, 255);
        delay(500);
        // engage short-brake: both direction pins LOW, PWM held high
        setDir(true, 0); setDir(false, 0);
        pwmWrite(PIN_PWM_L, 255); pwmWrite(PIN_PWM_R, 255);
        delay(300);
        stopAll();
        long counts = encL.getCount() - before;
        Serial.printf("  left wheel travelled %.1f mm from full speed to brake engage+300ms\n",
                      counts * MM_PER_COUNT);
    }

    Serial.println("\n-- PWM speed sweep (left motor, open-loop) --");
    for (int pwm = 80; pwm <= 255; pwm += 35) {
        char label[24];
        snprintf(label, sizeof(label), "PWM=%3d", pwm);
        runStep(label, true, +1, (uint8_t)pwm, 500);
        delay(250);
    }

    Serial.println("\n[DONE] Sequence complete. Reset the board to run again.");
    done = true;
}

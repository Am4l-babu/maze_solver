#include "motor_driver.h"

static constexpr uint32_t PWM_FREQ_HZ = 20000; // above audible, well under TB6612's 100 kHz
static constexpr uint8_t  PWM_RES_BITS = 8;

// LEDC API changed between Arduino-ESP32 core 2.x (channel-based) and
// 3.x (pin-based) — support both.
#if ESP_ARDUINO_VERSION_MAJOR >= 3
static void pwmInit(uint8_t pin)               { ledcAttach(pin, PWM_FREQ_HZ, PWM_RES_BITS); }
static void pwmWrite(uint8_t pin, uint32_t d)  { ledcWrite(pin, d); }
#else
static constexpr uint8_t CH_L = 0, CH_R = 1;
static void pwmInit(uint8_t pin) {
    uint8_t ch = (pin == PIN_PWM_L) ? CH_L : CH_R;
    ledcSetup(ch, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcAttachPin(pin, ch);
}
static void pwmWrite(uint8_t pin, uint32_t d) {
    ledcWrite((pin == PIN_PWM_L) ? CH_L : CH_R, d);
}
#endif

bool MotorDriver::begin(Adafruit_MCP23X17* mcp) {
    _mcp = mcp;
    for (uint8_t p : {MCP_AIN1, MCP_AIN2, MCP_BIN1, MCP_BIN2, MCP_STBY})
        _mcp->pinMode(p, OUTPUT);

    pwmInit(PIN_PWM_L);
    pwmInit(PIN_PWM_R);
    pwmWrite(PIN_PWM_L, 0);
    pwmWrite(PIN_PWM_R, 0);

    coast();
    standby(false);          // STBY high = driver enabled
    return true;
}

void MotorDriver::writeDir(bool isLeft, Dir d) {
    Dir& cur = isLeft ? _dirL : _dirR;
    if (cur == d) return;    // skip redundant I2C traffic
    cur = d;

    uint8_t in1 = isLeft ? MCP_AIN1 : MCP_BIN1;
    uint8_t in2 = isLeft ? MCP_AIN2 : MCP_BIN2;
    switch (d) {
        case FWD:   _mcp->digitalWrite(in1, HIGH); _mcp->digitalWrite(in2, LOW);  break;
        case REV:   _mcp->digitalWrite(in1, LOW);  _mcp->digitalWrite(in2, HIGH); break;
        case BRAKE: _mcp->digitalWrite(in1, LOW);  _mcp->digitalWrite(in2, LOW);  break;
        case COAST: _mcp->digitalWrite(in1, LOW);  _mcp->digitalWrite(in2, LOW);  break;
    }
}

void MotorDriver::apply(uint8_t pwmPin, bool isLeft, float pwm) {
    // Voltage compensation: keep effective drive constant as pack sags.
    float scale = (_battV > 6.0f) ? (11.1f / _battV) : 1.0f;
    pwm = constrain(pwm * scale, -255.0f, 255.0f);

    if (fabsf(pwm) < 1.0f) {
        writeDir(isLeft, COAST);
        pwmWrite(pwmPin, 0);
        return;
    }
    writeDir(isLeft, pwm > 0 ? FWD : REV);
    pwmWrite(pwmPin, (uint32_t)fabsf(pwm));
}

void MotorDriver::setLeft(float pwm)  { apply(PIN_PWM_L, true,  pwm); }
void MotorDriver::setRight(float pwm) { apply(PIN_PWM_R, false, pwm); }

void MotorDriver::shortBrake() {
    writeDir(true,  BRAKE);
    writeDir(false, BRAKE);
    // Full PWM in short-brake mode = strongest braking on TB6612
    pwmWrite(PIN_PWM_L, 255);
    pwmWrite(PIN_PWM_R, 255);
}

void MotorDriver::coast() {
    writeDir(true,  COAST);
    writeDir(false, COAST);
    pwmWrite(PIN_PWM_L, 0);
    pwmWrite(PIN_PWM_R, 0);
}

void MotorDriver::standby(bool en) {
    _mcp->digitalWrite(MCP_STBY, en ? LOW : HIGH); // active low
}

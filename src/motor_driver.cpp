#include "motor_driver.h"

static constexpr uint32_t PWM_FREQ_HZ = 20000; // above audible, well under TB6612's 100 kHz
static constexpr uint8_t  PWM_RES_BITS = 8;

bool MotorDriver::begin(Adafruit_MCP23X17* mcp) {
    _mcp = mcp;
    for (uint8_t p : {MCP_AIN1, MCP_AIN2, MCP_BIN1, MCP_BIN2, MCP_STBY})
        _mcp->pinMode(p, OUTPUT);

    ledcAttach(PIN_PWM_L, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcAttach(PIN_PWM_R, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcWrite(PIN_PWM_L, 0);
    ledcWrite(PIN_PWM_R, 0);

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
        ledcWrite(pwmPin, 0);
        return;
    }
    writeDir(isLeft, pwm > 0 ? FWD : REV);
    ledcWrite(pwmPin, (uint32_t)fabsf(pwm));
}

void MotorDriver::setLeft(float pwm)  { apply(PIN_PWM_L, true,  pwm); }
void MotorDriver::setRight(float pwm) { apply(PIN_PWM_R, false, pwm); }

void MotorDriver::shortBrake() {
    writeDir(true,  BRAKE);
    writeDir(false, BRAKE);
    // Full PWM in short-brake mode = strongest braking on TB6612
    ledcWrite(PIN_PWM_L, 255);
    ledcWrite(PIN_PWM_R, 255);
}

void MotorDriver::coast() {
    writeDir(true,  COAST);
    writeDir(false, COAST);
    ledcWrite(PIN_PWM_L, 0);
    ledcWrite(PIN_PWM_R, 0);
}

void MotorDriver::standby(bool en) {
    _mcp->digitalWrite(MCP_STBY, en ? LOW : HIGH); // active low
}

#pragma once
#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include "config.h"

// TB6612FNG driver. PWM comes from the XIAO's LEDC peripheral;
// direction pins and STBY live on the MCP23017 to save GPIOs.
// Direction changes cost one I2C write (~25 µs @ 400 kHz) and only
// happen on sign changes, so the 100 Hz loop is unaffected.
class MotorDriver {
public:
    bool begin(Adafruit_MCP23X17* mcp);

    // pwm in [-255, 255]; sign = direction, 0 = coast
    void setLeft(float pwm);
    void setRight(float pwm);
    void setBoth(float left, float right) { setLeft(left); setRight(right); }

    void shortBrake();   // both dir pins LOW: shorts windings, hard stop
    void coast();
    void standby(bool en);

    // Scales commanded PWM by nominal/actual pack voltage so tuning
    // holds as the battery sags. Call once per loop from main.
    void setBatteryVoltage(float v) { _battV = v; }

private:
    enum Dir { FWD, REV, BRAKE, COAST };
    void writeDir(bool isLeft, Dir d);
    void apply(uint8_t pwmPin, bool isLeft, float pwm);

    Adafruit_MCP23X17* _mcp = nullptr;
    Dir   _dirL = COAST, _dirR = COAST;
    float _battV = 11.1f;
};

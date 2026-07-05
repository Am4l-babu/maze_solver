#pragma once
#include <Arduino.h>

// Generic PID with anti-windup clamping and derivative-on-measurement
// filtering. dt is passed in by the caller so all controllers share the
// control-loop timebase instead of each reading millis().
class PIDController {
public:
    PIDController(float kp, float ki, float kd, float outMin, float outMax)
        : _kp(kp), _ki(ki), _kd(kd), _outMin(outMin), _outMax(outMax) {}

    float compute(float setpoint, float measurement, float dt);
    void  reset();
    void  setTunings(float kp, float ki, float kd) { _kp = kp; _ki = ki; _kd = kd; }

private:
    float _kp, _ki, _kd;
    float _outMin, _outMax;
    float _integral   = 0.0f;
    float _prevMeas   = 0.0f;
    float _dFiltered  = 0.0f;
    bool  _first      = true;
};

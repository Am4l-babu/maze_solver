#include "pid_controller.h"

float PIDController::compute(float setpoint, float measurement, float dt) {
    if (dt <= 0.0f) dt = 0.001f;

    float error = setpoint - measurement;

    // Integral with clamped anti-windup
    _integral += error * dt;
    if (_ki > 0.0f) {
        float iMax = _outMax / _ki;
        float iMin = _outMin / _ki;
        _integral = constrain(_integral, iMin, iMax);
    }

    // Derivative on measurement (avoids setpoint-change kick),
    // low-pass filtered to reject encoder quantization noise.
    float dRaw = _first ? 0.0f : -(measurement - _prevMeas) / dt;
    _first = false;
    _prevMeas = measurement;
    const float alpha = 0.15f;
    _dFiltered += alpha * (dRaw - _dFiltered);

    float out = _kp * error + _ki * _integral + _kd * _dFiltered;
    return constrain(out, _outMin, _outMax);
}

void PIDController::reset() {
    _integral  = 0.0f;
    _dFiltered = 0.0f;
    _first     = true;
}

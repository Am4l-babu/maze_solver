#pragma once
#include <Arduino.h>
#include <ESP32Encoder.h>
#include "config.h"

// Quadrature decoding via the ESP32-S3 PCNT peripheral (through the
// ESP32Encoder library) — zero CPU cost, no missed edges at speed.
class EncoderSystem {
public:
    void begin();
    void update(float dt);      // call once per control loop
    void zero();

    long  getLeftCount()  const { return _countL; }
    long  getRightCount() const { return _countR; }

    float getLeftSpeed()  const { return _speedL; }   // mm/s, filtered
    float getRightSpeed() const { return _speedR; }

    float getLeftDistance()  const { return _countL * MM_PER_COUNT; }
    float getRightDistance() const { return _countR * MM_PER_COUNT; }

private:
    ESP32Encoder _encL, _encR;
    long  _countL = 0, _countR = 0;
    long  _prevL  = 0, _prevR  = 0;
    float _speedL = 0, _speedR = 0;
};

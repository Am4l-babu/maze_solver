#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include "config.h"

// Distances in millimetres. Out-of-range reads are clamped to RANGE_MAX.
struct SensorData {
    float left   = 0;   // -90°
    float diagL  = 0;   // -45°
    float front  = 0;   //   0°
    float diagR  = 0;   // +45°
    float right  = 0;   // +90°
    bool  valid[NUM_TOF] = {false, false, false, false, false};
};

// Five VL53L0X behind a PCA9548A mux. All sensors keep the default
// address 0x29; the mux isolates them onto separate bus segments.
// Sensors run in continuous mode so a full 5-sensor sweep costs
// roughly one channel-select + result-read each (~1 ms/sensor).
class SensorArray {
public:
    static constexpr float RANGE_MAX = 2000.0f;

    bool begin();
    void readAll();
    const SensorData& getData() const { return _data; }

    // Per-sensor mounting offset (mm), subtracted from raw reading.
    // Set during CALIBRATING against a known-distance jig.
    void setOffset(uint8_t idx, float mm) { if (idx < NUM_TOF) _offset[idx] = mm; }

private:
    void  selectChannel(uint8_t ch);
    float readOne(uint8_t idx);

    Adafruit_VL53L0X _tof[NUM_TOF];
    float            _offset[NUM_TOF] = {0};
    SensorData       _data;
};

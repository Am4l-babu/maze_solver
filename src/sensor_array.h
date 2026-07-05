#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config.h"
#include "i2c_bus.h"

// Distances in millimetres. Out-of-range reads are clamped to RANGE_MAX.
struct SensorData {
    float left   = 0;   // -90°
    float diagL  = 0;   // -30°
    float front  = 0;   //   0°
    float diagR  = 0;   // +30°
    float right  = 0;   // +90°
    bool  valid[NUM_TOF] = {false, false, false, false, false};
};

// Five VL53L0X behind a PCA9548A mux. All sensors keep the default
// address 0x29; the mux isolates them onto separate bus segments.
// Sensors run in continuous mode so a full 5-sensor sweep costs
// roughly one channel-select + result-read each (~1 ms/sensor).
//
// readAll() normally runs on its own FreeRTOS task pinned to core 0 (see
// beginTask()), decoupling the I2C sweep from the core-1 control loop.
// getDataSafe() is the cross-core accessor the control loop should use;
// getData() is only safe when called from the same core that's calling
// readAll() (e.g. during single-threaded setup()/CALIBRATING).
class SensorArray {
public:
    static constexpr float RANGE_MAX = 2000.0f;

    bool begin();
    void readAll();
    const SensorData& getData() const { return _data; }
    SensorData getDataSafe() const;

    // Spins up the background ranging task on core 0. Call once, after
    // begin() succeeds and after I2CBus::begin() has run.
    void beginTask();

    // Per-sensor mounting offset (mm), subtracted from raw reading.
    // Set during CALIBRATING against a known-distance jig.
    void setOffset(uint8_t idx, float mm) { if (idx < NUM_TOF) _offset[idx] = mm; }

private:
    void  selectChannel(uint8_t ch);
    float readOne(uint8_t idx);
    static void taskFn(void* param);

    Adafruit_VL53L0X _tof[NUM_TOF];
    float            _offset[NUM_TOF] = {0};
    SensorData       _data;
    mutable portMUX_TYPE _dataMux = portMUX_INITIALIZER_UNLOCKED;
};

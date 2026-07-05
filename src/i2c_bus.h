#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Guards the shared I2C bus. The TOF sensor task runs on core 0; the main
// control loop's MCP23017 motor-direction writes run on core 1. Arduino's
// Wire driver isn't safe against being interleaved across cores, so every
// transaction — a PCA9548A channel select + VL53L0X read, or an MCP23017
// read-modify-write — must complete inside one lock/unlock pair before the
// other core can touch the bus.
class I2CBus {
public:
    static void begin() { _mux = xSemaphoreCreateMutex(); }
    static void lock()   { xSemaphoreTake(_mux, portMAX_DELAY); }
    static void unlock() { xSemaphoreGive(_mux); }

private:
    inline static SemaphoreHandle_t _mux = nullptr;
};

// RAII helper so a lock can't be left held on an early return.
class I2CGuard {
public:
    I2CGuard()  { I2CBus::lock(); }
    ~I2CGuard() { I2CBus::unlock(); }
};

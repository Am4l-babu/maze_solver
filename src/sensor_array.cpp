#include "sensor_array.h"

void SensorArray::selectChannel(uint8_t ch) {
    Wire.beginTransmission(ADDR_PCA9548A);
    Wire.write(1 << ch);
    Wire.endTransmission();
}

bool SensorArray::begin() {
    // Runs during single-threaded setup(), before beginTask() exists —
    // no other core can be touching the bus yet, so no I2CGuard needed here.
    bool ok = true;
    for (uint8_t i = 0; i < NUM_TOF; i++) {
        selectChannel(i);
        if (!_tof[i].begin(ADDR_VL53L0X, false, &Wire,
                           Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED)) {
            Serial.printf("[TOF] sensor %u failed to init\n", i);
            ok = false;
            continue;
        }
        _tof[i].startRangeContinuous(20); // 20 ms inter-measurement
    }
    return ok;
}

float SensorArray::readOne(uint8_t idx) {
    I2CGuard guard;  // channel select + read must not interleave with
                     // MotorDriver's MCP23017 writes on the other core
    selectChannel(idx);
    if (!_tof[idx].isRangeComplete()) {
        _data.valid[idx] = false;
        return NAN;
    }
    uint16_t mm = _tof[idx].readRangeResult();
    _data.valid[idx] = true;
    if (mm == 0 || mm > RANGE_MAX) return RANGE_MAX;
    float d = (float)mm - _offset[idx];
    return d < 0 ? 0 : d;
}

void SensorArray::readAll() {
    float v;
    float left = _data.left, diagL = _data.diagL, front = _data.front,
          diagR = _data.diagR, right = _data.right;

    if (!isnan(v = readOne(CH_TOF_LEFT)))   left  = v;
    if (!isnan(v = readOne(CH_TOF_DIAG_L))) diagL = v;
    if (!isnan(v = readOne(CH_TOF_FRONT)))  front = v;
    if (!isnan(v = readOne(CH_TOF_DIAG_R))) diagR = v;
    if (!isnan(v = readOne(CH_TOF_RIGHT)))  right = v;

    // Publish the whole sweep as one atomic update — a consumer on the
    // other core should never see a mix of this sweep and the last one.
    portENTER_CRITICAL(&_dataMux);
    _data.left = left; _data.diagL = diagL; _data.front = front;
    _data.diagR = diagR; _data.right = right;
    portEXIT_CRITICAL(&_dataMux);
}

SensorData SensorArray::getDataSafe() const {
    SensorData snapshot;
    portENTER_CRITICAL(&_dataMux);
    snapshot = _data;
    portEXIT_CRITICAL(&_dataMux);
    return snapshot;
}

void SensorArray::taskFn(void* param) {
    SensorArray* self = static_cast<SensorArray*>(param);
    for (;;) {
        self->readAll();
        vTaskDelay(pdMS_TO_TICKS(5)); // sensors range every 20ms; poll faster than that
    }
}

void SensorArray::beginTask() {
    xTaskCreatePinnedToCore(taskFn, "tof_sensors", 4096, this, 1, nullptr, 0);
}

#include "sensor_array.h"

void SensorArray::selectChannel(uint8_t ch) {
    Wire.beginTransmission(ADDR_PCA9548A);
    Wire.write(1 << ch);
    Wire.endTransmission();
}

bool SensorArray::begin() {
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
    selectChannel(idx);
    if (!_tof[idx].isRangeComplete()) {
        // Result not ready yet — keep previous value, mark stale.
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
    if (!isnan(v = readOne(CH_TOF_LEFT)))   _data.left  = v;
    if (!isnan(v = readOne(CH_TOF_DIAG_L))) _data.diagL = v;
    if (!isnan(v = readOne(CH_TOF_FRONT)))  _data.front = v;
    if (!isnan(v = readOne(CH_TOF_DIAG_R))) _data.diagR = v;
    if (!isnan(v = readOne(CH_TOF_RIGHT)))  _data.right = v;
}

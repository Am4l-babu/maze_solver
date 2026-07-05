#include "encoder.h"

void EncoderSystem::begin() {
    ESP32Encoder::useInternalWeakPullResistors = puType::up;
    _encL.attachFullQuad(PIN_ENC_LA, PIN_ENC_LB);
    _encR.attachFullQuad(PIN_ENC_RB, PIN_ENC_RA); // swapped: right motor is mirrored
    zero();
}

void EncoderSystem::zero() {
    _encL.clearCount();
    _encR.clearCount();
    _countL = _countR = _prevL = _prevR = 0;
    _speedL = _speedR = 0;
}

void EncoderSystem::update(float dt) {
    if (dt <= 0.0f) return;
    _countL = (long)_encL.getCount();
    _countR = (long)_encR.getCount();

    float rawL = (_countL - _prevL) * MM_PER_COUNT / dt;
    float rawR = (_countR - _prevR) * MM_PER_COUNT / dt;
    _prevL = _countL;
    _prevR = _countR;

    // Light low-pass: at 450 CPR and 100 Hz the per-tick quantization
    // is ~30 mm/s, enough to upset the D term without filtering.
    const float a = 0.35f;
    _speedL += a * (rawL - _speedL);
    _speedR += a * (rawR - _speedR);
}

#pragma once
#include <soc/gpio_struct.h>

// Direct-register GPIO — the ESP32-S3 analog of AVR's PORTB/PIND bit
// twiddling. Bypasses Arduino's digitalWrite/digitalRead pin-table lookup
// for pins toggled every control-loop tick (status LED, start button).
// GPIO 0-31 live in the out/in bank; GPIO 32-48 live in the out1/in1 bank.
static inline void fastPinWrite(uint8_t pin, bool level) {
    if (pin < 32) {
        if (level) GPIO.out_w1ts = (1UL << pin);
        else       GPIO.out_w1tc = (1UL << pin);
    } else {
        uint32_t bit = 1UL << (pin - 32);
        if (level) GPIO.out1_w1ts.data = bit;
        else       GPIO.out1_w1tc.data = bit;
    }
}

static inline bool fastPinRead(uint8_t pin) {
    if (pin < 32) return (GPIO.in >> pin) & 1;
    return (GPIO.in1.data >> (pin - 32)) & 1;
}

// ============================================================
// TEST 04 — SSD1306 0.96" I2C OLED (128x64) bring-up
// New component, not yet wired into the main robot firmware.
// Runs a splash screen, a full-frame test pattern (corners +
// border, to catch column/row offset issues), then a live
// counter so you can confirm the refresh isn't hanging.
// ============================================================
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define PIN_SDA 5   // XIAO D4 / GPIO5 — shared main I2C bus
#define PIN_SCL 6   // XIAO D5 / GPIO6

#define SCREEN_W 128
#define SCREEN_H 64

// Most 0.96" blue SSD1306 boards are 0x3C; some clones ship as 0x3D.
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);
uint8_t foundAddr = 0;

bool tryInit(uint8_t addr) {
    if (display.begin(SSD1306_SWITCHCAPVCC, addr)) {
        foundAddr = addr;
        return true;
    }
    return false;
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("[TEST] SSD1306 128x64 OLED");

    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(400000);

    if (!tryInit(0x3C) && !tryInit(0x3D)) {
        Serial.println("[FAIL] no SSD1306 found at 0x3C or 0x3D. Check: "
                        "VCC->3.3V, GND, SDA->D4, SCL->D5.");
        while (true) delay(1000);
    }
    Serial.printf("[OK] SSD1306 found at 0x%02X\n", foundAddr);

    // --- splash screen ---
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("MICRO-MAZE SOLVER");
    display.println("PRIM.E Chapter II");
    display.setCursor(0, 30);
    display.setTextSize(2);
    display.println("OLED OK");
    display.display();
    delay(1500);

    // --- test pattern: border + 4 corner pixels, catches any
    //     row/column offset or mirrored-panel wiring issue ---
    display.clearDisplay();
    display.drawRect(0, 0, SCREEN_W, SCREEN_H, SSD1306_WHITE);
    display.fillCircle(4, 4, 2, SSD1306_WHITE);
    display.fillCircle(SCREEN_W - 5, 4, 2, SSD1306_WHITE);
    display.fillCircle(4, SCREEN_H - 5, 2, SSD1306_WHITE);
    display.fillCircle(SCREEN_W - 5, SCREEN_H - 5, 2, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(38, 28);
    display.println("PATTERN");
    display.display();
    delay(2000);
}

void loop() {
    static uint32_t frame = 0;
    static uint32_t lastMs = 0;
    uint32_t now = millis();
    if (now - lastMs < 200) return;
    lastMs = now;
    frame++;

    // Live counter + a moving bar, so a hang shows up immediately as a
    // frozen screen instead of a subtle no-op.
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("LIVE REFRESH TEST");
    display.setCursor(0, 16);
    display.printf("frame: %lu", frame);
    display.setCursor(0, 28);
    display.printf("uptime: %lus", now / 1000);

    int barX = (frame * 4) % (SCREEN_W - 10);
    display.fillRect(barX, 48, 10, 10, SSD1306_WHITE);
    display.drawRect(0, 48, SCREEN_W, 10, SSD1306_WHITE);

    display.display();
}

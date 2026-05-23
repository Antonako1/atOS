// LCD_TEST.ino
// Tests the 20x4 I2C LCD.
// Cycles through several messages so you can verify the display
// and backlight work before flashing more complex firmware.

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Change address to 0x27 if 0x3F does not work
LiquidCrystal_I2C lcd(0x27, 20, 4);

#define DELAY_MS 1500

void setup() {
    Wire.begin(21, 22);   // SDA=21, SCL=22

    lcd.init();
    // lcd.backlight();
}

void loop() {
    // --- Test 1: basic text on all 4 rows ---
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("LCD_TEST 20x4");
    lcd.setCursor(0, 1);
    lcd.print("Row 2 OK");
    lcd.setCursor(0, 2);
    lcd.print("Row 3 OK");
    lcd.setCursor(0, 3);
    lcd.print("Row 4 OK");
    delay(DELAY_MS);

    // --- Test 2: fill all 80 characters ---
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("01234567890123456789");
    lcd.setCursor(0, 1);
    lcd.print("ABCDEFGHIJKLMNOPQRST");
    lcd.setCursor(0, 2);
    lcd.print("abcdefghijklmnopqrst");
    lcd.setCursor(0, 3);
    lcd.print("!@#$%^&*()-=[]{}|;'");
    delay(DELAY_MS);

    // --- Test 3: backlight toggle ---
    lcd.noBacklight();
    delay(500);
    lcd.backlight();
    delay(DELAY_MS);

    // --- Test 4: cursor sweep across all 4 rows ---
    lcd.clear();
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 20; col++) {
            lcd.setCursor(col, row);
            lcd.print("*");
            delay(40);
        }
    }
    delay(DELAY_MS);
}


// SERIAL.ino
// Reads RS-232 data via MAX3232 on UART2 (RX=16, TX=17),
// displays each line on a 16x2 I2C LCD, and echoes to USB serial.

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// UART2: RX=16, TX=17  (MAX3232 connected here)
HardwareSerial RS232(2);

static int lcd_col = 0;
static int lcd_row = 0;

void lcd_reset() {
    lcd.clear();
    lcd_col = 0;
    lcd_row = 0;
    lcd.setCursor(0, 0);
}

void lcd_putc(char c) {
    if (c == '\r') return;

    if (c == '\n') {
        lcd_row++;
        lcd_col = 0;
        if (lcd_row >= 2) { lcd_reset(); return; }
        lcd.setCursor(0, lcd_row);
        return;
    }

    // Drop non-printable ASCII — control chars render as LCD CGROM glyphs
    if ((unsigned char)c < 0x20 || (unsigned char)c > 0x7E) return;

    if (lcd_col >= 16) {
        lcd_row++;
        lcd_col = 0;
        if (lcd_row >= 2) { lcd_reset(); }
        else { lcd.setCursor(0, lcd_row); }
    }

    lcd.print(c);
    lcd_col++;
}

void setup() {
    Serial.begin(115200);
    RS232.begin(115200, SERIAL_8N1, 16, 17);
    pinMode(16, INPUT_PULLUP); // Hold RX HIGH when line is idle/floating (RS-232 MARK state)
    Wire.begin(21, 22);

    lcd.init();
    lcd.backlight();
    lcd_reset();
    lcd.print("ESP32 READY");

    Serial.println("ESP32 READY");
    delay(2000);
    lcd_reset();
    lcd.print("Waiting...");
}

void loop() {
    while (RS232.available()) {
        char c = RS232.read();
        Serial.write(c);
        lcd_putc(c);
    }
}

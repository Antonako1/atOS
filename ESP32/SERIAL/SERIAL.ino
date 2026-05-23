// SERIAL.ino
// Reads RS-232 data via MAX3232 on UART2 (RX=16, TX=17),
// displays each line on a 16x2 I2C LCD, and echoes to USB serial.

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// UART2: RX=16, TX=17  (MAX3232 connected here)
HardwareSerial RS232(2);

String incoming = "";

void setup() {
    Serial.begin(115200);

    RS232.begin(9600, SERIAL_8N1, 16, 17);

    Wire.begin(21, 22);

    lcd.init();
    lcd.backlight();

    lcd.setCursor(0, 0);
    lcd.print("ESP32 READY");

    Serial.println("ESP32 READY");

    delay(2000);

    lcd.clear();
    lcd.print("Waiting...");
}

void loop() {
    while (RS232.available()) {
        char c = RS232.read();

        Serial.write(c);

        if (c == '\n') {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(incoming);

            Serial.println();
            Serial.print("LCD: ");
            Serial.println(incoming);

            incoming = "";
        } else {
            incoming += c;

            // Prevent LCD overflow (16 chars per row)
            if (incoming.length() > 16) {
                incoming.remove(16);
            }
        }
    }
}

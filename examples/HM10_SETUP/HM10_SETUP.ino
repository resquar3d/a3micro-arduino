/*
 * HM10_SETUP.ino - name and check an HM-10 Bluetooth module
 *
 * Talk to the HM-10 from the Serial Monitor (9600 baud; any line-ending setting works).
 * Type a command and press Send; the module's answer is printed. Useful commands:
 *   AT              the module answers OK (it is wired up and listening)
 *   AT+NAME?        its Bluetooth name            AT+NAMERover   rename it to Rover (up to 12 characters)
 *   AT+ADDR?        its address                   AT+RESET       restart it (a new name shows after this)
 * Commands only work while no phone is connected to the module.
 * Wiring: HM-10 TXD -> pin 7, RXD -> pin 8 (through a 5 V to 3.3 V divider), VCC -> 5V, GND -> GND.
 *
 * Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
 * Released under the MIT License (see LICENSE).
 */

#include <SoftwareSerial.h>

SoftwareSerial hm10(7, 8);

void setup() {
  Serial.begin(9600);
  hm10.begin(9600);
  Serial.println("HM-10 setup: type AT and press Send");
}

void loop() {
  // Serial Monitor -> module. The HM-10 wants commands without line endings, so those are left out.
  while (Serial.available()) {
    char c = Serial.read();
    if (c != '\r' && c != '\n') hm10.write(c);
  }
  // Module -> Serial Monitor
  while (hm10.available()) Serial.write(hm10.read());
}

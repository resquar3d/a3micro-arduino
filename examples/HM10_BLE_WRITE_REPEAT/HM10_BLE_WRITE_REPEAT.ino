/*
 * HM10_BLE_WRITE_REPEAT.ino - the board reports to the app every second
 *
 * Sends a counter and the board's uptime together, once a second: ##;count:7,up:7.0;##
 * Add displays with labels count and up in the app.
 *
 * Board: UNO R3 (or any board) with an HM-10 on pins 7 / 8.
 *
 * Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
 * Released under the MIT License (see LICENSE).
 */

#include <A3Micro.h>
#include <SoftwareSerial.h>

SoftwareSerial bleSerial(7, 8);   // HM-10: TXD -> pin 7, RXD -> pin 8 (through a 5 V to 3.3 V divider)
A3Micro a3(bleSerial);

unsigned long lastSent = 0, count = 0;

void setup() {
  Serial.begin(9600);
  bleSerial.begin(9600);
  a3.begin();
}

void loop() {
  if (millis() - lastSent < 1000) return;
  lastSent = millis();

  A3MicroPacket out;                           // two readings in one message
  out.add("count", ++count);
  out.add("up", millis() / 1000.0, 1);
  a3.send(out);
  Serial.println(out.toMessage());
}

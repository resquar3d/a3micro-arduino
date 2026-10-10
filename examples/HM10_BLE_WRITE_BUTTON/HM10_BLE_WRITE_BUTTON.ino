/*
 * HM10_BLE_WRITE_BUTTON.ino - a push button on the board shows in the app
 *
 * Sends ##;btn:1;## while the button is held and ##;btn:0;## when it is let go. Add a display with
 * label btn in the app to see it.
 * Button between pin 2 and GND (the internal pull-up is used).
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

const int BUTTON_PIN = 2;
const unsigned long SETTLE_MS = 25;            // contacts bounce: a change must last this long to count

int stableState = HIGH, lastReading = HIGH;
unsigned long changedAt = 0;

void setup() {
  Serial.begin(9600);
  bleSerial.begin(9600);
  a3.begin();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastReading) {
    lastReading = reading;
    changedAt = millis();
  }
  if (reading != stableState && millis() - changedAt >= SETTLE_MS) {
    stableState = reading;
    bool pressed = stableState == LOW;
    a3.send("btn", pressed ? 1 : 0);
    Serial.println(pressed ? "pressed" : "released");
  }
}

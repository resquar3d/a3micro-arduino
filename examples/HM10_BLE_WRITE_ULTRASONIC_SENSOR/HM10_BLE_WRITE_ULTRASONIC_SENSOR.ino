/*
 * HM10_BLE_WRITE_ULTRASONIC_SENSOR.ino - a distance sensor reports to the app
 *
 * Measures with an HC-SR04 ten times a second and sends ##;dist:42;## (centimetres).
 * Out of range sends ##;dist:-;##. Add a display with label dist in the app.
 * HC-SR04: TRIG -> pin 9, ECHO -> pin 10, VCC -> 5V, GND -> GND.
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

const int TRIG_PIN = 9, ECHO_PIN = 10;
const unsigned long ECHO_TIMEOUT_US = 30000;   // about 5 m: no echo after this = nothing in range

unsigned long lastMeasured = 0;

void setup() {
  Serial.begin(9600);
  bleSerial.begin(9600);
  a3.begin();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  if (millis() - lastMeasured < 100) return;
  lastMeasured = millis();

  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);                // a 10 microsecond pulse starts a measurement
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long echo = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (echo == 0) a3.send("dist", "-");
  else a3.send("dist", (long)(echo / 58));     // sound: about 58 microseconds per centimetre there and back
}

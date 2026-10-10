/*
 * UNO_R3_RGB_LED.ino - three sliders mix the colour of an RGB LED
 *
 * Sliders sl0, sl1 and sl2 in the app (0-100) set red, green and blue.
 * Common-cathode RGB LED: red -> pin 9, green -> pin 10, blue -> pin 11 (each through a 220 ohm resistor).
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

const int RED_PIN = 9, GREEN_PIN = 10, BLUE_PIN = 11;

// A slider's 0-100 as an LED brightness 0-255
int brightness(const A3MicroPacket &in, const char *label) {
  return map(constrain(in.getInt(label), 0, 100), 0, 100, 0, 255);
}

void setup() {
  Serial.begin(9600);
  bleSerial.begin(9600);
  a3.begin();
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
}

void loop() {
  A3MicroPacket in;
  if (!a3.receive(in)) return;

  if (in.has("sl0")) analogWrite(RED_PIN, brightness(in, "sl0"));
  if (in.has("sl1")) analogWrite(GREEN_PIN, brightness(in, "sl1"));
  if (in.has("sl2")) analogWrite(BLUE_PIN, brightness(in, "sl2"));
}

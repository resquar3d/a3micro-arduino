/*
 * UNO_R4_MINIMA_LED.ino - a button in the app switches an LED
 *
 * In the app, a button or switch with label b0 sends ##;b0:1;## when pressed / on and ##;b0:0;## when
 * released / off. This sketch turns the LED on pin 13 on and off with it.
 *
 * Board: UNO R4 Minima with an HM-10 on Serial1 (D0 / D1).
 *
 * Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
 * Released under the MIT License (see LICENSE).
 */

#include <A3Micro.h>

A3Micro a3(Serial1);              // HM-10 on the UNO R4 Minima's hardware serial: TXD -> D0, RXD -> D1

const int LED_PIN = 13;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  a3.begin();
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  A3MicroPacket in;
  if (!a3.receive(in)) return;                 // nothing new yet

  Serial.println(in.toMessage());              // everything that arrived, for the Serial Monitor
  if (in.has("b0")) {
    digitalWrite(LED_PIN, in.getBool("b0") ? HIGH : LOW);
  }
}

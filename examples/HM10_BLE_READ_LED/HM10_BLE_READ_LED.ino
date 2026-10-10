/*
 * HM10_BLE_READ_LED.ino - a button in the app switches an LED
 *
 * In the app, a button or switch with label b0 sends ##;b0:1;## when pressed / on and ##;b0:0;## when
 * released / off. This sketch turns the LED on pin 13 on and off with it.
 *
 * Board: any Arduino-compatible board with an HM-10 on pins 7 / 8.
 *
 * Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
 * Released under the MIT License (see LICENSE).
 */

#include <A3Micro.h>
#include <SoftwareSerial.h>

SoftwareSerial bleSerial(7, 8);   // HM-10: TXD -> pin 7, RXD -> pin 8 (through a 5 V to 3.3 V divider)
A3Micro a3(bleSerial);

const int LED_PIN = 13;

void setup() {
  Serial.begin(9600);
  bleSerial.begin(9600);
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

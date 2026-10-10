/*
 * UNO_R4_WIFI_LED.ino - a button in the app switches an LED
 *
 * In the app, a button or switch with label b0 sends ##;b0:1;## when pressed / on and ##;b0:0;## when
 * released / off. This sketch turns the LED on pin 13 on and off with it.
 *
 * Board: UNO R4 WiFi, using its own Bluetooth LE radio.
 *
 * Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
 * Released under the MIT License (see LICENSE).
 */

#include <A3Micro.h>

A3Micro a3;                       // the UNO R4 WiFi's own Bluetooth LE radio

const int LED_PIN = 13;

void setup() {
  Serial.begin(9600);
  if (!a3.begin("UNO R4 WiFi")) Serial.println("Bluetooth LE did not start");
  Serial.println("Waiting for the A3Micro Control app...");
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

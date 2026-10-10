/*
 * UNO_R4_WIFI_SERVO.ino - a slider in the app turns a servo
 *
 * Slider sl0 (0-100) turns the servo from 0 to 179 degrees; the board answers with the angle (label angle).
 * Servo signal -> A0, power -> 5V, ground -> GND.
 *
 * Board: UNO R4 WiFi, using its own Bluetooth LE radio.
 *
 * Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
 * Released under the MIT License (see LICENSE).
 */

#include <A3Micro.h>

A3Micro a3;                       // the UNO R4 WiFi's own Bluetooth LE radio
#include <Servo.h>

Servo servo;

void setup() {
  Serial.begin(9600);
  if (!a3.begin("UNO R4 WiFi")) Serial.println("Bluetooth LE did not start");
  Serial.println("Waiting for the A3Micro Control app...");
  servo.attach(A0);
}

void loop() {
  A3MicroPacket in;
  if (!a3.receive(in) || !in.has("sl0")) return;

  int angle = map(constrain(in.getInt("sl0"), 0, 100), 0, 100, 0, 179);
  servo.write(angle);
  a3.send("angle", angle);
}

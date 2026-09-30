/*
 * UNO_R4_WIFI_LED.ino
 *
 * Description:
 * Demonstrates reading BLE commands on the Arduino UNO R4 WiFi using its built-in
 * BLE radio via `A3MicroManager` (no HM-10 module needed) and toggling the
 * onboard LED. The sketch parses incoming A3Micro messages (`b0` with value `1`
 * or `0`) and applies the LED state accordingly.
 *
 * Developed for TwinSparks Development (www.twinsparksdevelopment.com)
 * Modified and written by R.E Espino of twinsparks.dev
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "A3Micro.h"

// Create an instance of the A3MicroManager for the built-in BLE radio
// (no constructor argument = use the board's own radio instead of an HM-10)
A3MicroManager manager;

// LED pin for onboard LED
const int LED = 13;

void setup() {
  Serial.begin(9600);  // Initialize USB serial communication

  // Initialize built-in BLE and start advertising as "UNO R4 WIFI"
  if (!manager.begin("UNO R4 WIFI")) {
    Serial.println("Starting Bluetooth® Low Energy failed!");
  }
  Serial.println("BLE LED Peripheral, waiting for connections....");

  pinMode(LED, OUTPUT);  // Set the LED pin as an output
}

void loop() {
  // Only handle messages while the A3Micro app is connected
  if (manager.isConnected()) {
    // Read a message from BLE
    A3MicroMessage msg = manager.read();

    // Print message details if both ID and Value are valid
    if (msg.hasId() && msg.hasValue()) {
      Serial.println(msg.toString());
    }

    // Control LED based on message content
    if (msg.id == "b0") {
      if (msg.value == "1") {
        digitalWrite(LED, HIGH);  // Turn LED on
      } else if (msg.value == "0") {
        digitalWrite(LED, LOW);  // Turn LED off
      }
    }
  }
}

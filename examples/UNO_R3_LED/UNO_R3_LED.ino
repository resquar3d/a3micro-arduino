/*
 * UNO_R3_LED.ino
 *
 * Description:
 * Arduino sketch that utilizes the `A3MicroManager` class to receive and interpret BLE messages
 * for device control, such as turning an LED on or off based on message data. Initializes BLE and
 * serial communication for message handling.
 *
 * Developed for TwinSparks Development (www.twinsparksdevelopment.com)
 * Modified and written by R.E Espino of twinsparks.dev
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "A3Micro.h"

// Define BLE communication pins and create software serial for BLE
#include "SoftwareSerial.h"
const int rXPin = 7;
const int tXPin = 8;
SoftwareSerial SSerial(rXPin, tXPin);

// Create an instance of the A3MicroManager for managing messages
A3MicroManager manager(SSerial);

// LED pin for onboard LED
const int LED = 13;

void setup() {
  Serial.begin(9600);    // Initialize USB serial communication
  SSerial.begin(9600);   // Initialize software serial for BLE communication
  manager.begin();       // Initialize A3Micro messaging
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

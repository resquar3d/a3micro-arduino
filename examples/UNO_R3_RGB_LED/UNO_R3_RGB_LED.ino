/*
 * UNO_R3_RGB_LED.ino
 *
 * Description:
 * Arduino sketch that controls an RGB LED based on Bluetooth Low Energy (BLE) messages.
 * The `A3MicroManager` is used to receive and process BLE commands to adjust the brightness
 * of the red, green, and blue components of the LED. Each color is controlled using PWM.
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

// Pin declarations for RGB LED
const int redLED = 9;     // Red LED pin
const int greenLED = 10;  // Green LED pin
const int blueLED = 11;   // Blue LED pin

void setup() {
  Serial.begin(9600);         // Initialize USB serial communication
  SSerial.begin(9600);        // Initialize software serial for BLE communication
  manager.begin();            // Initialize A3Micro messaging
  pinMode(redLED, OUTPUT);    // Set red LED pin as output
  pinMode(greenLED, OUTPUT);  // Set green LED pin as output
  pinMode(blueLED, OUTPUT);   // Set blue LED pin as output
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

    // Control the LED based on the received ID
    if (msg.id == "sl0") {
      // Map the received value (0-100) to PWM range (0-255) for red LED
      int value = msg.value.toInt();
      int redlight = map(value, 0, 100, 0, 255);
      analogWrite(redLED, redlight);  // Adjust red LED brightness
    } else if (msg.id == "sl1") {
      // Map the received value (0-100) to PWM range (0-255) for blue LED
      int value = msg.value.toInt();
      int bluelight = map(value, 0, 100, 0, 255);
      analogWrite(blueLED, bluelight);  // Adjust blue LED brightness
    } else if (msg.id == "sl2") {
      // Map the received value (0-100) to PWM range (0-255) for green LED
      int value = msg.value.toInt();
      int greenlight = map(value, 0, 100, 0, 255);
      analogWrite(greenLED, greenlight);  // Adjust green LED brightness
    }
  }
}

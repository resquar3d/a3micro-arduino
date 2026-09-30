/*
 * HM10_BLE_WRITE_REPEAT.ino
 *
 * Description:
 * Demonstrates continuously sending BLE values using `A3MicroManager::write`.
 * The sketch alternates between sending `1` and `0` with a fixed delay.
 *
 * Wiring:
 * - HM-10 TXD -> Arduino pin 7 (RX)
 * - HM-10 RXD -> Arduino pin 8 (TX)
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

// Message ID and repeat interval
const char* REPEAT_ID = "b0";
const unsigned long REPEAT_DELAY_MS = 1000;

void setup() {
  Serial.begin(9600);   // Initialize USB serial communication
  SSerial.begin(9600);  // Initialize software serial for BLE communication
  manager.begin();      // Initialize A3Micro messaging
}

void loop() {
  // Wait for the A3Micro app to connect
  if (!manager.isConnected()) {
    return;
  }

  manager.write(REPEAT_ID, "1");  // Send value 1
  Serial.println("Sent: b0=1");   // Debug output
  delay(REPEAT_DELAY_MS);

  manager.write(REPEAT_ID, "0");  // Send value 0
  Serial.println("Sent: b0=0");   // Debug output
  delay(REPEAT_DELAY_MS);
}

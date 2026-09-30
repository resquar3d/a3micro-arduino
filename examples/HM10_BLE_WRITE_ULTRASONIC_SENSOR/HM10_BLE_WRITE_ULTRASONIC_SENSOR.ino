/*
 * HM10_BLE_WRITE_ULTRASONIC_SENSOR.ino
 *
 * Description:
 * Demonstrates sending ultrasonic distance readings over BLE to A3Micro.
 * The sketch triggers an HC-SR04 sensor, measures the echo pulse duration,
 * converts it to centimeters, and sends the value via `A3MicroManager::write`.
 *
 * Wiring:
 * - HM-10 TXD -> Arduino pin 7 (RX)
 * - HM-10 RXD -> Arduino pin 8 (TX)
 * - HC-SR04 TRIG -> Arduino pin 9
 * - HC-SR04 ECHO -> Arduino pin 10
 * - HC-SR04 VCC  -> 5V
 * - HC-SR04 GND  -> GND
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

// Ultrasonic sensor pins
const int TRIG_PIN = 9;
const int ECHO_PIN = 10;

// Message ID and measurement interval
const char* SENSOR_ID = "us0";
const unsigned long MEASURE_DELAY_MS = 100;

// Longest echo pulse we wait for, in microseconds. The HC-SR04 ranges to about
// 400 cm, which is a ~23 ms round trip; 30 ms gives margin. Without a timeout,
// pulseIn() would block the loop for a full second whenever nothing is in range.
const unsigned long ECHO_TIMEOUT_US = 30000;

void setup() {
  Serial.begin(9600);   // Initialize USB serial communication
  SSerial.begin(9600);  // Initialize software serial for BLE communication
  manager.begin();      // Initialize A3Micro messaging
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  // Wait for the A3Micro app to connect
  if (!manager.isConnected()) {
    return;
  }

  // Send a 10-microsecond pulse to trigger the sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure the duration of the echo pulse (in microseconds).
  // Returns 0 if no echo arrives within the timeout (nothing in range).
  long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);

  if (duration == 0) {
    Serial.println("Distance: out of range");
  } else {
    // Convert to distance in centimeters:
    // Speed of sound ~ 343 m/s = 34300 cm/s = 0.0343 cm/us
    // The echo pulse travels to the object and back (2x the distance),
    // so: distance = duration * 0.0343 / 2 = duration / 58.3
    long distanceCm = duration / 58;

    // Send the distance value over BLE
    manager.write(SENSOR_ID, String(distanceCm));
    Serial.print("Distance: ");
    Serial.print(distanceCm);
    Serial.println(" cm");
  }

  delay(MEASURE_DELAY_MS);
}

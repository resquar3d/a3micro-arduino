/*
 * UNO_R4_WIFI_SERVO.ino
 *
 * Description:
 * Demonstrates controlling a servo motor from the A3Micro app on the Arduino
 * UNO R4 WiFi using its built-in BLE radio via `A3MicroManager` (no HM-10
 * module needed). A slider message (`sl0` with value 0-100) is mapped to the
 * servo angle range.
 *
 * Developed for TwinSparks Development (www.twinsparksdevelopment.com)
 * Modified and written by R.E Espino of twinsparks.dev
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "A3Micro.h"
#include <Servo.h>

// Create an instance of the A3MicroManager for the built-in BLE radio
// (no constructor argument = use the board's own radio instead of an HM-10)
A3MicroManager manager;

const int SERVO_PIN = A0;  // Servo signal pin
Servo myServo;             // Servo instance for motor control

void setup() {
  Serial.begin(9600);  // Initialize USB serial communication

  // Initialize built-in BLE and start advertising as "UNO R4 WIFI"
  if (!manager.begin("UNO R4 WIFI")) {
    Serial.println("Starting Bluetooth® Low Energy failed!");
  }
  Serial.println("BLE Servo Peripheral, waiting for connections....");

  myServo.attach(SERVO_PIN);  // Attach servo to specified pin
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

    // Control servo motor based on BLE message with ID "sl0"
    if (msg.id == "sl0") {
      int intValue = msg.value.toInt();
      int servoAngle = map(intValue, 0, 100, 0, 179);  // Map value to servo angle range
      myServo.write(servoAngle);                       // Set servo to mapped angle
    }
  }
}

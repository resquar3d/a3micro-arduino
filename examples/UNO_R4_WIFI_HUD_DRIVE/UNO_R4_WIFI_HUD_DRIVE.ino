/*
 * UNO_R4_WIFI_HUD_DRIVE.ino
 *
 * Description:
 * Drives a two-motor robot from the "HUD Drive" layout of the A3Micro app on the
 * Arduino UNO R4 WiFi, using its built-in BLE radio via `A3MicroManager`.
 *
 *   r0   Reactor direction ring and knob: "up", "down", "left", "right", "stop"
 *   j0   Reactor knob position: "x,y" from -100 to 100 (up is +y), like the joystick
 *   sp0  Reactor speed gauge: 0-100, the top speed of the motors
 *
 * Tapping a direction segment drives straight or spins in place. Dragging the
 * knob steers smoothly: the knob position is mixed into left/right motor speeds
 * ("arcade drive"), scaled by the speed gauge.
 *   sw0  Lights switch: 1 = on, 0 = off
 *   b1   Horn button: 1 = pressed, 0 = released
 *   b0   LED button: onboard LED
 *   sl0  Servo slider: 0-100 mapped to 0-179 degrees
 *   sl1  Aux slider: 0-100 mapped to a PWM output
 *
 * Wiring (L298N or similar dual H-bridge):
 *   Left motor:  ENA -> D5 (PWM), IN1 -> D7, IN2 -> D8
 *   Right motor: ENB -> D6 (PWM), IN3 -> D9, IN4 -> D10
 *   Lights LED -> D4, horn buzzer -> D2, servo signal -> A0, aux output -> D3
 *
 * For safety the motors stop when the app sends "stop" and whenever the app
 * disconnects.
 *
 * Developed for TwinSparks Development (www.twinsparksdevelopment.com)
 * Modified and written by R.E Espino of twinsparks.dev
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "A3Micro.h"
#include <Servo.h>

// Built-in BLE radio (no constructor argument)
A3MicroManager manager;

const int LEFT_EN = 5, LEFT_IN1 = 7, LEFT_IN2 = 8;
const int RIGHT_EN = 6, RIGHT_IN3 = 9, RIGHT_IN4 = 10;
const int LIGHTS_PIN = 4;
const int HORN_PIN = 2;
const int LED_PIN = LED_BUILTIN;
const int SERVO_PIN = A0;
const int AUX_PIN = 3;

Servo servo;
String direction = "stop";  // Last direction from the reactor ring
int speedPwm = 0;           // Last speed from the gauge, as PWM duty (0-255)
int joyX = 0, joyY = 0;     // Last knob position, -100..100
bool wasConnected = false;

// Sets one motor: speed -255..255, negative = reverse
void setMotor(int en, int inA, int inB, int speed) {
  digitalWrite(inA, speed > 0 ? HIGH : LOW);
  digitalWrite(inB, speed < 0 ? HIGH : LOW);
  analogWrite(en, abs(speed));
}

// Applies the current knob position or direction, and the speed, to both motors
void drive() {
  int left = 0, right = 0;
  if (joyX != 0 || joyY != 0) {
    // Knob held: arcade mix, forward/back from y and turning from x
    left = constrain(joyY + joyX, -100, 100) * speedPwm / 100;
    right = constrain(joyY - joyX, -100, 100) * speedPwm / 100;
  } else if (direction == "up") {
    left = speedPwm;
    right = speedPwm;
  } else if (direction == "down") {
    left = -speedPwm;
    right = -speedPwm;
  } else if (direction == "left") {  // Spin left in place
    left = -speedPwm;
    right = speedPwm;
  } else if (direction == "right") {  // Spin right in place
    left = speedPwm;
    right = -speedPwm;
  }
  setMotor(LEFT_EN, LEFT_IN1, LEFT_IN2, left);
  setMotor(RIGHT_EN, RIGHT_IN3, RIGHT_IN4, right);
}

void setup() {
  Serial.begin(9600);

  pinMode(LEFT_EN, OUTPUT);
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);
  pinMode(RIGHT_EN, OUTPUT);
  pinMode(RIGHT_IN3, OUTPUT);
  pinMode(RIGHT_IN4, OUTPUT);
  pinMode(LIGHTS_PIN, OUTPUT);
  pinMode(HORN_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(AUX_PIN, OUTPUT);
  servo.attach(SERVO_PIN);
  drive();  // Motors off

  if (!manager.begin("UNO R4 WIFI")) {
    Serial.println("Starting Bluetooth® Low Energy failed!");
  }
  Serial.println("HUD Drive ready, waiting for the app...");
}

void loop() {
  bool connected = manager.isConnected();

  // Stop the motors as soon as the app disconnects
  if (!connected) {
    if (wasConnected) {
      direction = "stop";
      joyX = 0;
      joyY = 0;
      drive();
      noTone(HORN_PIN);
      Serial.println("App disconnected: motors stopped");
    }
    wasConnected = false;
    return;
  }
  wasConnected = true;

  A3MicroMessage msg = manager.read();
  if (!msg.hasId()) {
    return;  // Nothing new
  }
  Serial.println(msg.toString());

  if (msg.id == "r0") {
    direction = msg.value;  // up, down, left, right or stop
    if (direction == "stop") {
      joyX = 0;  // Stop always wins, even if "0,0" was lost
      joyY = 0;
    }
    drive();
  } else if (msg.id == "j0") {
    int comma = msg.value.indexOf(',');
    if (comma > 0) {
      joyX = constrain(msg.value.substring(0, comma).toInt(), -100, 100);
      joyY = constrain(msg.value.substring(comma + 1).toInt(), -100, 100);
      drive();
    }
  } else if (msg.id == "sp0") {
    speedPwm = map(constrain(msg.value.toInt(), 0, 100), 0, 100, 0, 255);
    drive();
  } else if (msg.id == "sw0") {
    digitalWrite(LIGHTS_PIN, msg.value == "1" ? HIGH : LOW);
  } else if (msg.id == "b1") {
    if (msg.value == "1") {
      tone(HORN_PIN, 440);
    } else {
      noTone(HORN_PIN);
    }
  } else if (msg.id == "b0") {
    digitalWrite(LED_PIN, msg.value == "1" ? HIGH : LOW);
  } else if (msg.id == "sl0") {
    servo.write(map(constrain(msg.value.toInt(), 0, 100), 0, 100, 0, 179));
  } else if (msg.id == "sl1") {
    analogWrite(AUX_PIN, map(constrain(msg.value.toInt(), 0, 100), 0, 100, 0, 255));
  }
}

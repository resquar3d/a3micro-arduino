/*
 * UNO_R4_WIFI_HUD_DRIVE.ino - drive a two-motor robot with the HUD Drive layout
 *
 * Labels the HUD Drive layout sends:
 *   r0          direction ring: up, down, left, right or stop
 *   j0x, j0y    the knob, -100 to 100 each (up is +y), sent together: ##;j0x:40,j0y:85;##
 *   sp0         speed gauge, 0-100 (top speed of the motors)
 *   sw0         lights switch, 1 / 0      b1   horn button, 1 while held
 *   b0          LED button                sl0  servo slider, 0-100      sl1  aux slider, 0-100
 * The board reports its battery (label bat, volts) every two seconds: give a display the label bat.
 *
 * Wiring (L298N dual H-bridge or similar):
 *   left motor ENA -> D5, IN1 -> D7, IN2 -> D8     right motor ENB -> D6, IN3 -> D9, IN4 -> D10
 *   lights -> D4, horn buzzer -> D2, servo -> A0, aux -> D3, battery through a 1:2 divider -> A1
 * For safety the motors stop on stop and whenever the app disconnects.
 *
 * Board: UNO R4 WiFi, using its own Bluetooth LE radio.
 *
 * Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
 * Released under the MIT License (see LICENSE).
 */

#include <A3Micro.h>

A3Micro a3;                       // the UNO R4 WiFi's own Bluetooth LE radio
#include <Servo.h>

const int LEFT_EN = 5, LEFT_A = 7, LEFT_B = 8;
const int RIGHT_EN = 6, RIGHT_A = 9, RIGHT_B = 10;
const int LIGHTS_PIN = 4, HORN_PIN = 2, AUX_PIN = 3, SERVO_PIN = A0, BATTERY_PIN = A1;

Servo servo;
String heading = "stop";        // from the direction ring
int topSpeed = 0;               // 0-255, from the speed gauge
int knobX = 0, knobY = 0;       // -100..100, from the knob
bool wasConnected = false;
unsigned long lastBattery = 0;

// One motor: -255 (full reverse) .. 255 (full forward)
void motor(int en, int a, int b, int speed) {
  digitalWrite(a, speed > 0 ? HIGH : LOW);
  digitalWrite(b, speed < 0 ? HIGH : LOW);
  analogWrite(en, min(abs(speed), 255));
}

void drive() {
  int left = 0, right = 0;
  if (knobX || knobY) {                                // the knob steers: mix it into the two motors
    left = constrain(knobY + knobX, -100, 100) * topSpeed / 100;
    right = constrain(knobY - knobX, -100, 100) * topSpeed / 100;
  } else if (heading == "up") {
    left = right = topSpeed;
  } else if (heading == "down") {
    left = right = -topSpeed;
  } else if (heading == "left") {                      // turn on the spot
    left = -topSpeed;
    right = topSpeed;
  } else if (heading == "right") {
    left = topSpeed;
    right = -topSpeed;
  }
  motor(LEFT_EN, LEFT_A, LEFT_B, left);
  motor(RIGHT_EN, RIGHT_A, RIGHT_B, right);
}

void stopAll() {
  heading = "stop";
  knobX = knobY = 0;
  drive();
  noTone(HORN_PIN);
}

int percent(const A3MicroPacket &in, const char *label) {
  return constrain(in.getInt(label), 0, 100);
}

void setup() {
  const int outputs[] = { LEFT_EN, LEFT_A, LEFT_B, RIGHT_EN, RIGHT_A, RIGHT_B, LIGHTS_PIN, HORN_PIN, AUX_PIN, LED_BUILTIN };
  for (int pin : outputs) pinMode(pin, OUTPUT);
  servo.attach(SERVO_PIN);
  Serial.begin(9600);
  if (!a3.begin("HUD Drive")) Serial.println("Bluetooth LE did not start");
  Serial.println("Waiting for the A3Micro Control app...");
}

void loop() {
  if (!a3.connected()) {
    if (wasConnected) {
      stopAll();
      Serial.println("App gone: motors stopped");
    }
    wasConnected = false;
    return;
  }
  wasConnected = true;

  if (millis() - lastBattery >= 2000) {                // battery: A1 sees half the voltage (1:2 divider)
    lastBattery = millis();
    a3.send("bat", analogRead(BATTERY_PIN) * 5.0 / 1023.0 * 2.0);
  }

  A3MicroPacket in;
  if (!a3.receive(in)) return;
  Serial.println(in.toMessage());

  if (in.has("r0")) {
    heading = in.get("r0");
    if (heading == "stop") knobX = knobY = 0;          // stop always wins
  }
  if (in.has("j0x")) knobX = constrain(in.getInt("j0x"), -100, 100);
  if (in.has("j0y")) knobY = constrain(in.getInt("j0y"), -100, 100);
  if (in.has("sp0")) topSpeed = map(percent(in, "sp0"), 0, 100, 0, 255);
  if (in.has("r0") || in.has("j0x") || in.has("j0y") || in.has("sp0")) drive();

  if (in.has("sw0")) digitalWrite(LIGHTS_PIN, in.getBool("sw0") ? HIGH : LOW);
  if (in.has("b1")) {
    if (in.getBool("b1")) tone(HORN_PIN, 440);
    else noTone(HORN_PIN);
  }
  if (in.has("b0")) digitalWrite(LED_BUILTIN, in.getBool("b0") ? HIGH : LOW);
  if (in.has("sl0")) servo.write(map(percent(in, "sl0"), 0, 100, 0, 179));
  if (in.has("sl1")) analogWrite(AUX_PIN, map(percent(in, "sl1"), 0, 100, 0, 255));
}

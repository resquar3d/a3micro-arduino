/*
 * UNO_R4_WIFI_WRITE_BUTTON.ino
 *
 * Description:
 * Demonstrates sending BLE messages to A3Micro from a physical button on the
 * Arduino UNO R4 WiFi using its built-in BLE radio via `A3MicroManager` (no
 * HM-10 module needed). When the button is pressed, the sketch sends value
 * `Pressed`; when released, it sends value `Released`.
 *
 * Wiring:
 * - Button between pin 2 and GND (uses INPUT_PULLUP)
 *
 * Developed for TwinSparks Development (www.twinsparksdevelopment.com)
 * Modified and written by R.E Espino of twinsparks.dev
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "A3Micro.h"

// Create an instance of the A3MicroManager for the built-in BLE radio
// (no constructor argument = use the board's own radio instead of an HM-10)
A3MicroManager manager;

// Button input pin and message ID for A3Micro button widget
const int BUTTON_PIN = 2;
const char* BUTTON_ID = "b0";

// Debounce state:
// Buttons can "bounce" electrically when pressed/released, causing noisy rapid toggles.
// These variables filter that noise so each physical action sends only one BLE message.
int lastButtonReading = HIGH;
int buttonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY_MS = 25;

void setup() {
  Serial.begin(9600);  // Initialize USB serial communication

  // Initialize built-in BLE and start advertising as "UNO R4 WIFI"
  if (!manager.begin("UNO R4 WIFI")) {
    Serial.println("Starting Bluetooth® Low Energy failed!");
  }
  Serial.println("BLE Button Peripheral, waiting for connections....");

  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Use internal pull-up; pressed state reads LOW
}

void loop() {
  // Only send messages while the A3Micro app is connected
  if (!manager.isConnected()) {
    return;
  }

  // Read current button state from the input pin
  int reading = digitalRead(BUTTON_PIN);

  // If raw input changed, restart debounce timer.
  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  // Accept the new state only if it stays unchanged for the debounce window.
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (reading != buttonState) {
      buttonState = reading;  // Update stable button state after debounce
      if (buttonState == LOW) {
        manager.write(BUTTON_ID, "Pressed");  // Send pressed value
        Serial.println("Button pressed");     // Debug output
      } else {
        manager.write(BUTTON_ID, "Released");  // Send released value
        Serial.println("Button released");     // Debug output
      }
    }
  }

  // Save raw reading for the next debounce comparison
  lastButtonReading = reading;
}

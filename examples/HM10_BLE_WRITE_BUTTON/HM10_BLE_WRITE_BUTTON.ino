/*
 * HM10_BLE_WRITE_BUTTON.ino
 *
 * Description:
 * Demonstrates sending BLE messages to A3Micro from a physical button.
 * When the button is pressed, the sketch sends value `Pressed`; when released,
 * it sends value `Released`.
 *
 * Wiring:
 * - HM-10 TXD -> Arduino pin 7 (RX)
 * - HM-10 RXD -> Arduino pin 8 (TX)
 * - Button between pin 2 and GND (uses INPUT_PULLUP)
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
  Serial.begin(9600);                 // Initialize USB serial communication
  SSerial.begin(9600);                // Initialize software serial for BLE communication
  manager.begin();                    // Initialize A3Micro messaging
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Use internal pull-up; pressed state reads LOW
}

void loop() {
  // Wait for the A3Micro app to connect
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

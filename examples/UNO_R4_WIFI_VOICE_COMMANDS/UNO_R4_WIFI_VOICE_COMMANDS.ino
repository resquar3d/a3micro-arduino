/*
 * UNO_R4_WIFI_VOICE_COMMANDS.ino - the board does what you say to the A3 Micro app
 *
 * With Voice commands switched on in the app (Sound & voice, A3 Micro Pro), you tap the mic and speak:
 *   - a control by its label ("light on", "speed 60") arrives as that control's own message, ##;b0:1;##
 *   - "stop" always arrives as ##;vc:stop;## (and stops the D-pad / drive)
 *   - anything else arrives as the words themselves: ##;vc:open the gate;##
 * This sketch switches the LED on pin 13 from button b0 and from the words "light on" / "light off", blinks it
 * on "blink", and tells the app what it understood (or that it didn't).
 *
 * Board: UNO R4 WiFi, using its own Bluetooth LE radio. The same handling works on any board and link.
 *
 * Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
 * Released under the MIT License (see LICENSE).
 */

#include <A3Micro.h>

A3Micro a3;                       // the UNO R4 WiFi's own Bluetooth LE radio

const int LED_PIN = 13;

void setLight(bool on) {
  digitalWrite(LED_PIN, on ? HIGH : LOW);
  a3.send("light", on ? "on" : "off");
}

void setup() {
  Serial.begin(9600);
  if (!a3.begin("UNO R4 Voice")) Serial.println("Bluetooth LE did not start");
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  A3MicroPacket in;
  if (!a3.receive(in)) return;                 // nothing new yet
  Serial.println(in.toMessage());

  // A control the app matched by its label (or pressed on screen)
  if (in.has("b0")) setLight(in.getBool("b0"));

  // The words themselves
  if (in.has("vc")) {
    String said = in.get("vc");
    said.toLowerCase();
    if (said.indexOf("stop") >= 0) {
      setLight(false);
    } else if (said.indexOf("light") >= 0 && said.indexOf("off") >= 0) {
      setLight(false);
    } else if (said.indexOf("light") >= 0 && said.indexOf("on") >= 0) {
      setLight(true);
    } else if (said.indexOf("blink") >= 0) {
      for (int i = 0; i < 6; i++) { digitalWrite(LED_PIN, i % 2 ? LOW : HIGH); delay(200); }
      setLight(false);
    } else {
      a3.send("heard", "not understood: " + said);   // shows in the app's log and in a display labelled heard
    }
  }
}

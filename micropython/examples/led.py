"""
led.py

Description:
MicroPython version of the UNO_R4_WIFI_LED example. Receives messages from the
A3Micro app over the board's built-in BLE radio and toggles the onboard LED:
message b0 with value 1 turns it on, value 0 turns it off.

Copy a3micro.py and this file to the board (save this one as main.py to run it
at power-on).

Developed for TwinSparks Development (www.twinsparksdevelopment.com)
Modified and written by R.E Espino of twinsparks.dev
Licensed under the MIT License. See LICENSE for details.
"""

import time
from machine import Pin
from a3micro import A3MicroManager

try:
    led = Pin("LED", Pin.OUT)  # Raspberry Pi Pico W / Pico 2 W
except (TypeError, ValueError):
    led = Pin(2, Pin.OUT)      # Most ESP32 boards

manager = A3MicroManager()  # no argument = built-in BLE

if not manager.begin("A3Micro"):
    print("Starting BLE failed!")

while True:
    # Only handle messages while the A3Micro app is connected
    if manager.is_connected():
        msg = manager.read()

        if msg.has_id() and msg.has_value():
            print(msg.to_string())

        if msg.id == "b0":
            if msg.value == "1":
                led.value(1)
            elif msg.value == "0":
                led.value(0)
    time.sleep_ms(5)

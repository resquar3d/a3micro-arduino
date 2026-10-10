"""
led.py - a button in the A3Micro Control app switches the board's LED (own Bluetooth radio)

The app's button or switch with label b0 sends ##;b0:1;## (on) and ##;b0:0;## (off).
Boards: ESP32, Raspberry Pi Pico W / Pico 2 W and other MicroPython boards with Bluetooth.
Copy a3micro.py and this file to the board (save this one as main.py to start it at power-on).

Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
Released under the MIT License (see LICENSE).
"""

import time
from machine import Pin
from a3micro import A3Micro

try:
    led = Pin("LED", Pin.OUT)      # Pico W / Pico 2 W
except (TypeError, ValueError):
    led = Pin(2, Pin.OUT)          # most ESP32 boards

a3 = A3Micro()                     # the board's own Bluetooth LE radio
if not a3.begin("A3Micro LED"):
    print("Bluetooth LE did not start")

while True:
    packet = a3.receive()
    if packet:
        print(packet.to_message())
        if packet.has("b0"):
            led.value(1 if packet.get_bool("b0") else 0)
    time.sleep_ms(5)

"""
write_button.py - a push button on the board shows in the A3Micro Control app

Sends ##;btn:1;## while the button is held and ##;btn:0;## when it is let go, plus the board's uptime
every two seconds (##;up:12;##). Give displays in the app the labels btn and up.
Wiring: a push button between GPIO 15 and GND (the internal pull-up is used).

Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
Released under the MIT License (see LICENSE).
"""

import time
from machine import Pin
from a3micro import A3Micro

button = Pin(15, Pin.IN, Pin.PULL_UP)
a3 = A3Micro()
if not a3.begin("A3Micro Button"):
    print("Bluetooth LE did not start")

last = None
last_report = time.ticks_ms()
while True:
    if a3.connected():
        pressed = button.value() == 0
        if pressed != last:
            last = pressed
            a3.send("btn", pressed)
        if time.ticks_diff(time.ticks_ms(), last_report) >= 2000:
            last_report = time.ticks_ms()
            a3.send("up", time.ticks_ms() // 1000)
    else:
        last = None                # send the button again after reconnecting
    a3.receive()                   # keep incoming messages moving even when not used
    time.sleep_ms(20)              # also smooths out contact bounce

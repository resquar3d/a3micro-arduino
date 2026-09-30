"""
write_button.py

Description:
MicroPython version of the UNO_R4_WIFI_WRITE_BUTTON example. Sends b0 = 1 when a
button is pressed and b0 = 0 when it is released, so a Display control with the
message ID b0 in the A3Micro app shows the button's state.

Wiring: a push button between GPIO 15 and GND (the internal pull-up is used).

Developed for TwinSparks Development (www.twinsparksdevelopment.com)
Modified and written by R.E Espino of twinsparks.dev
Licensed under the MIT License. See LICENSE for details.
"""

import time
from machine import Pin
from a3micro import A3MicroManager

BUTTON_ID = "b0"
button = Pin(15, Pin.IN, Pin.PULL_UP)

manager = A3MicroManager()

if not manager.begin("A3Micro"):
    print("Starting BLE failed!")

last_pressed = None

while True:
    # Only send messages while the A3Micro app is connected
    if manager.is_connected():
        pressed = button.value() == 0
        if pressed != last_pressed:
            last_pressed = pressed
            manager.write(BUTTON_ID, "1" if pressed else "0")
    else:
        last_pressed = None  # Send the current state again after reconnecting
    time.sleep_ms(20)  # Debounce

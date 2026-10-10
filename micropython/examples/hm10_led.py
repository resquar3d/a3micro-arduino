"""
hm10_led.py - a button in the A3Micro Control app switches the LED, through an HM-10 module

For boards without their own Bluetooth (or when an HM-10 is preferred). Label b0: 1 = on, 0 = off.
Wiring (Raspberry Pi Pico): HM-10 TXD -> GP5 (UART1 RX), RXD -> GP4 (UART1 TX), VCC -> VBUS (5V), GND -> GND.
On an ESP32 use UART(2, 9600, tx=17, rx=16).

Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
Released under the MIT License (see LICENSE).
"""

import time
from machine import Pin, UART
from a3micro import A3Micro

uart = UART(1, 9600, tx=Pin(4), rx=Pin(5))
a3 = A3Micro(uart)                 # the HM-10 advertises by itself

try:
    led = Pin("LED", Pin.OUT)
except (TypeError, ValueError):
    led = Pin(25, Pin.OUT)         # Raspberry Pi Pico (no W)

while True:
    packet = a3.receive()
    if packet and packet.has("b0"):
        led.value(1 if packet.get_bool("b0") else 0)
    time.sleep_ms(5)

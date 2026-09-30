"""
hm10_led.py

Description:
MicroPython version of the HM10_BLE_READ_LED example, for boards without their
own Bluetooth (or when you prefer an HM-10 module). The HM-10 is connected to a
UART at 9600 baud; message b0 with value 1 turns the onboard LED on, 0 turns it off.

Wiring (Raspberry Pi Pico): HM-10 TXD -> GP5 (UART1 RX), HM-10 RXD -> GP4 (UART1 TX),
VCC -> VBUS (5V), GND -> GND. On an ESP32, use UART(2, 9600, tx=17, rx=16).

Developed for TwinSparks Development (www.twinsparksdevelopment.com)
Modified and written by R.E Espino of twinsparks.dev
Licensed under the MIT License. See LICENSE for details.
"""

import time
from machine import Pin, UART
from a3micro import A3MicroManager

uart = UART(1, 9600, tx=Pin(4), rx=Pin(5))
manager = A3MicroManager(uart)  # HM-10 mode

try:
    led = Pin("LED", Pin.OUT)
except (TypeError, ValueError):
    led = Pin(25, Pin.OUT)  # Raspberry Pi Pico (non-W)

manager.begin()  # No-op in HM-10 mode: the module advertises on its own

while True:
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

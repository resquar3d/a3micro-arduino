# A3Micro for MicroPython

`a3micro.py` is the MicroPython version of the A3Micro Arduino library. It has the same classes and methods and sends the same `[1][ID][2][VALUE][3]` messages. The A3Micro app ([get it here](https://a3micro.twinsparks.dev/tiers.html)) and web controller work with it unchanged.

In built-in Bluetooth mode the board uses the same Bluetooth service as the UNO R4 WiFi, so the app treats it exactly like an UNO R4 WiFi.

## Boards

- **Built-in Bluetooth:** ESP32, ESP32-S3, ESP32-C3, Raspberry Pi Pico W, Pico 2 W, and any other MicroPython board with the `bluetooth` module.
- **HM-10 module:** any MicroPython board with a UART, such as the Raspberry Pi Pico.

## Install

1. Flash MicroPython onto the board (https://micropython.org/download).
2. Copy `a3micro.py` to the board, for example with Thonny (**File > Save as > MicroPython device**) or `mpremote cp a3micro.py :`.
3. Copy one of the examples. Save it as `main.py` to run it every time the board powers on.

## Usage

```python
from a3micro import A3MicroManager

manager = A3MicroManager()      # built-in Bluetooth
# manager = A3MicroManager(uart)  # HM-10 on a machine.UART at 9600 baud

manager.begin("A3Micro")        # advertising name

while True:
    if manager.is_connected():
        msg = manager.read()
        if msg.id == "b0":
            print("button", msg.value)
    manager.write("status", "ok")  # shows in a Display control with ID "status"
```

## API

It matches the Arduino library. The Arduino method names (`isConnected`, `hasId`, `hasValue`, `toString`) also work.

| Arduino | MicroPython |
|---|---|
| `A3MicroManager manager;` | `manager = A3MicroManager()` |
| `A3MicroManager manager(SSerial);` | `manager = A3MicroManager(uart)` |
| `manager.begin("name")` | `manager.begin("name")`: returns `False` if Bluetooth fails to start |
| `manager.isConnected()` | `manager.is_connected()`: always `True` in HM-10 mode |
| `A3MicroMessage msg = manager.read();` | `msg = manager.read()`: never blocks; returns an empty message when nothing new arrived |
| `manager.write("id", "value")` | `manager.write("id", value)`: the value can be any type, for example a number |
| `msg.id`, `msg.value` | `msg.id`, `msg.value` |
| `msg.hasId()`, `msg.hasValue()`, `msg.toString()` | `msg.has_id()`, `msg.has_value()`, `msg.to_string()` |

As in the Arduino library, received values are lowercased and have their spaces removed.

## Examples

| File | Arduino equivalent | What it does |
|---|---|---|
| `examples/led.py` | `UNO_R4_WIFI_LED` | The app's `b0` button turns the onboard LED on and off |
| `examples/write_button.py` | `UNO_R4_WIFI_WRITE_BUTTON` | Sends `b0` = 1/0 when a button on GPIO 15 is pressed/released |
| `examples/hm10_led.py` | `HM10_BLE_READ_LED` | Same as `led.py`, through an HM-10 module on a UART |

## Other MicroPython Bluetooth code

The app also connects to boards running the Nordic UART Service (the "BLE UART" from MicroPython's `ble_simple_peripheral.py` example and many tutorials). Send and receive the same `[1][ID][2][VALUE][3]` frames over it.

# A3Micro for MicroPython

`a3micro.py` is the MicroPython version of the A3Micro library. It speaks the same A3Micro protocol as the Arduino library, `##;label:value,...;##` (see the [main README](../README.md#the-a3micro-protocol)), so the A3 Micro app (1.8 or newer, [get it here](https://a3micro.twinsparks.dev/tiers.html)) works with it the same way.

With its own Bluetooth radio, the board offers the same A3Micro service as the UNO R4 WiFi.

## Boards

- **Own Bluetooth:** ESP32, ESP32-S3, ESP32-C3, Raspberry Pi Pico W, Pico 2 W, and any other MicroPython board with the `bluetooth` module.
- **HM-10 module:** any MicroPython board with a UART, such as the Raspberry Pi Pico.

## Install

1. Flash MicroPython onto the board (https://micropython.org/download).
2. Copy `a3micro.py` to the board, for example with Thonny (**File > Save as > MicroPython device**) or `mpremote cp a3micro.py :`.
3. Copy one of the examples. Save it as `main.py` to run it every time the board powers on.

## Usage

```python
from a3micro import A3Micro

a3 = A3Micro()            # own Bluetooth
# a3 = A3Micro(uart)      # an HM-10 on a machine.UART at 9600 baud
a3.begin("My Robot")      # the name the app shows

while True:
    packet = a3.receive()                 # None until a whole message has arrived
    if packet:
        if packet.has("b0"):
            print("button", packet.get_bool("b0"))
        x = packet.get_int("j0x")         # a joystick sends j0x and j0y together
    a3.send("bat", 3.71)                  # ##;bat:3.71;##
    a3.send_packet({"bat": 3.71, "dist": 42})   # ##;bat:3.71,dist:42;##
```

## API

| | |
|---|---|
| `A3Micro()` / `A3Micro(uart)` | Own Bluetooth / a serial module on a UART |
| `begin(name="A3Micro")` | Starts advertising (own Bluetooth); `False` if it didn't start |
| `connected()` | `True` while the app is connected (always `True` with a serial module) |
| `receive()` | The next message as an `A3MicroPacket`, or `None`. Never waits. |
| `send(label, value)` | One pair; numbers, text and `True`/`False` (sent as 1/0) |
| `send_packet(pairs)` | Several pairs: an `A3MicroPacket`, a `dict` or a list of `(label, value)` |
| `packet.has(label)`, `packet.get(label, fallback=None)` | Whether a label came, and its value as text |
| `packet.get_int(...)`, `packet.get_float(...)`, `packet.get_bool(...)` | The value as a number or `True`/`False` |
| `packet.pairs`, `for label, value in packet` | All pairs, in order |
| `packet.to_message()` | The message as sent |

## Examples

| File | Arduino equivalent | What it does |
|---|---|---|
| `examples/led.py` | `UNO_R4_WIFI_LED` | The app's `b0` switches the onboard LED |
| `examples/write_button.py` | `UNO_R4_WIFI_WRITE_BUTTON` | A button on GPIO 15 shows in the app (`btn`), plus the uptime (`up`) |
| `examples/hm10_led.py` | `HM10_BLE_READ_LED` | Same as `led.py`, through an HM-10 on a UART |

## Other MicroPython Bluetooth code

The app also connects to boards running the Nordic UART Service (the "BLE UART" in MicroPython's `ble_simple_peripheral.py` example and many tutorials). Send and receive A3Micro messages over it as text: `uart.write("##;bat:3.71;##")`.

## License

MIT License, Copyright (c) 2026 R.E Espino, TwinSparks Development. See [LICENSE](../LICENSE).

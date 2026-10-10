# A3Micro

Arduino library for the **A3Micro Control** app: control your board from an Android phone over Bluetooth LE, with an HM-10 module on any board or the built-in radio of the UNO R4 WiFi. A MicroPython version for the ESP32 and Raspberry Pi Pico W is in [`micropython`](micropython).

[![Get the A3 Micro app](https://img.shields.io/badge/Get_the_A3_Micro_app-a3micro.twinsparks.dev-e2b762?style=for-the-badge&logo=android&logoColor=white)](https://a3micro.twinsparks.dev/tiers.html)

This library is free and open source. The **A3 Micro app** that controls your board is available at **[a3micro.twinsparks.dev](https://a3micro.twinsparks.dev/tiers.html)**, with a Free edition and Premium and Pro plans. See [Get the A3 Micro app](#get-the-a3-micro-app).

> **Version 3 uses the new A3Micro protocol** (`##;label:value,...;##`). It needs the A3 Micro app **1.8 or newer**, and sketches written for version 2 need the small changes in [Moving from version 2](#moving-from-version-2).

## Features

- One readable text protocol in both directions: `##;j1x:273,j1y:120;##`
- Several values in one message (a joystick's x and y, a set of sensor readings)
- One `A3Micro` class for both links: an HM-10 module on a serial port, or the UNO R4 WiFi's own Bluetooth LE
- Never blocks: `receive()` returns straight away, messages that arrive in pieces are put back together
- Typed helpers: `getInt`, `getFloat`, `getBool`, and `send` for numbers and text

## Hardware

- Arduino UNO R3 or UNO R4 Minima with an HM-10 Bluetooth LE module, **or**
- Arduino UNO R4 WiFi (its own radio; uses the [ArduinoBLE](https://docs.arduino.cc/libraries/arduinoble/) library)
- The A3 Micro app for Android ([get it here](https://a3micro.twinsparks.dev/tiers.html))

## Get the A3 Micro app

The app turns your Android phone into a controller for your board: buttons, switches, sliders, a joystick, a D-pad and value displays, in layouts you can edit. Every example in this library works with it.

| | Free | Premium | Pro |
|---|---|---|---|
| Price | Free | €27, one-time | €35, one-time, 12 months |
| Controls | D-pad, joystick, button A, speed slider | Every control, unlimited layouts | Everything in Premium |
| Animated HUD skins, sounds and voice | | ✓ | ✓ |
| Logs, graphs and sync across devices | | | ✓ |
| New app versions, skins and features | | | ✓ (during the 12 months) |
| APK downloads | 2 a year | 3 a year | Unlimited for the 12 months |

1. Go to **[a3micro.twinsparks.dev](https://a3micro.twinsparks.dev/tiers.html)** and register (free).
2. Download the Free edition, or buy Premium or Pro. Payments go through Stripe.
3. Install the APK on your Android phone, then open the app.
4. Upload one of the [examples](#examples) to your board, tap **CONNECT** and pick your board.

All sales are final: see the website's terms before buying.

## Installation

### Arduino Library Manager

1. Open the Arduino IDE.
2. Go to **Sketch > Include Library > Manage Libraries**.
3. Search for "A3Micro" and click **Install**.

### From GitHub

1. On this repository's page, click **Code > Download ZIP**.
2. In the Arduino IDE, go to **Sketch > Include Library > Add .ZIP Library...** and choose the ZIP.
3. For the UNO R4 WiFi, also install [ArduinoBLE](https://docs.arduino.cc/libraries/arduinoble/) (the Library Manager does this for you; a ZIP install does not).

## Wiring

The UNO R4 WiFi needs no wiring: it uses its own radio.

For other boards, connect an HM-10:

| HM-10 | UNO R3 | UNO R4 Minima |
|---|---|---|
| VCC | 5V | 5V |
| GND | GND | GND |
| TXD | pin 7 (SoftwareSerial RX) | pin 0 (Serial1 RX) |
| RXD | pin 8 (SoftwareSerial TX), through a 5 V to 3.3 V divider | pin 1 (Serial1 TX), through a 5 V to 3.3 V divider |

The UNO R3's only hardware serial port is used for USB, so its examples use `SoftwareSerial` on pins 7 and 8. The UNO R4 Minima has a second port, `Serial1`, on pins 0 and 1. HM-10 modules talk at 9600 baud out of the box, which every example uses. To give the module its own name, upload **HM10_SETUP** and send `AT+NAMEYourName`.

## The A3Micro protocol

Every message, in both directions, is one line of text:

```
##;label1:value1,label2:value2,...,labelN:valueN;##
```

| Part | Meaning |
|---|---|
| `##` | Start of the message |
| `;` | Separates the start and end markers from the pairs |
| `label` | What the value is: `j1x` (joystick 1, x direction), `sl0` (slider 0), `bat` (battery) |
| `:` | Between a label and its value |
| `value` | The value, as text: `273`, `on`, `3.71` |
| `,` | Between two pairs |
| `##` | End of the message |

The app sends one message for each action: a button press, a moved slider or joystick, a Send. A joystick sends its two directions together, `##;j0x:40,j0y:85;##`. The board answers in the same format, for example its battery voltage and a measured distance, `##;bat:3.71,dist:42;##`; a display in the app with that label shows the value.

A `;` `:` `,` `#` or `%` inside a label or value is sent as `%3B` `%3A` `%2C` `%23` `%25`, so text typed in the app can never break a message. The library encodes and decodes this for you.

Bluetooth LE: the UNO R4 WiFi (and MicroPython boards) offer the A3Micro service `A3C10000-9A58-4589-AD8F-6FCC3C2BF21A` with one characteristic, `A3C10001-9A58-4589-AD8F-6FCC3C2BF21A` (write from the app, notify to the app). HM-10 modules use their own serial service; the app finds either.

## Usage

### Receive: an LED from a button in the app

```cpp
#include <A3Micro.h>
#include <SoftwareSerial.h>

SoftwareSerial bleSerial(7, 8);
A3Micro a3(bleSerial);              // HM-10 on pins 7 and 8
// A3Micro a3;                      // UNO R4 WiFi: its own radio

void setup() {
  bleSerial.begin(9600);
  a3.begin();                       // on the UNO R4 WiFi: a3.begin("My Robot")
  pinMode(13, OUTPUT);
}

void loop() {
  A3MicroPacket in;
  if (a3.receive(in) && in.has("b0")) {
    digitalWrite(13, in.getBool("b0") ? HIGH : LOW);
  }
}
```

### A joystick

```cpp
A3MicroPacket in;
if (a3.receive(in)) {
  if (in.has("j0x")) x = in.getInt("j0x");    // -100 .. 100
  if (in.has("j0y")) y = in.getInt("j0y");
}
```

### Send: readings to the app

```cpp
a3.send("dist", 42);                // ##;dist:42;##
a3.send("bat", 3.71);               // ##;bat:3.71;##

A3MicroPacket out;                  // several at once: ##;bat:3.71,dist:42;##
out.add("bat", 3.71);
out.add("dist", 42);
a3.send(out);
```

## API reference

### `A3Micro`, the link

| | |
|---|---|
| `A3Micro a3(stream)` | An HM-10 (or similar serial Bluetooth module) on a `Stream`: `SoftwareSerial`, `Serial1`, ... |
| `A3Micro a3` | The UNO R4 WiFi's own radio (needs ArduinoBLE) |
| `bool begin(name = "A3Micro")` | Starts advertising as `name` (own radio); `false` if the radio didn't start. With a serial module it does nothing and returns `true`. |
| `bool connected()` | `true` while the app is connected (own radio). A serial module can't tell, so it is always `true`. |
| `bool receive(A3MicroPacket &in)` | `true` with the next complete message in `in`; `false` when nothing new has arrived. Never waits; one message per call. |
| `send(packet)` | Sends a packet |
| `send(label, value)` | Sends one pair; `value` can be text, a whole number or a decimal (`send("v", 3.14159, 3)` for 3 decimals) |

### `A3MicroPacket`, one message

| | |
|---|---|
| `bool add(label, value)` | Adds a pair: text, whole numbers, decimals (2 places, or `add("v", x, 3)`), `true`/`false` (sent as 1/0). `false` when it is full (8 pairs). |
| `uint8_t size()`, `bool empty()` | How many pairs it holds |
| `label(i)`, `value(i)` | Pair number `i`, in the order they came |
| `bool has(label)` | Whether the label is in the message |
| `String get(label, fallback = "")` | The value as text |
| `long getInt(label, fallback = 0)` | The value as a whole number |
| `float getFloat(label, fallback = 0)` | The value as a decimal |
| `bool getBool(label, fallback = false)` | `1`, `on`, `true`, `yes`, `high` are true; `0`, `off`, `false`, `no`, `low` are false |
| `String toMessage()` | The whole message as sent, handy for `Serial.println` |
| `clear()` | Empties it |

Limits (change them with a `#define` before `#include <A3Micro.h>`): `A3MICRO_MAX_PAIRS` 8 pairs per message, `A3MICRO_MAX_MESSAGE` 160 characters.

## Examples

Open them from **File > Examples > A3Micro** in the Arduino IDE.

**UNO R3 (or any board) with an HM-10 on pins 7 / 8**
- **UNO_R3_LED**, **HM10_BLE_READ_LED**: a button in the app (`b0`) switches the LED on pin 13
- **UNO_R3_RGB_LED**: three sliders (`sl0`, `sl1`, `sl2`) mix an RGB LED
- **HM10_BLE_WRITE_BUTTON**: a push button on the board shows in the app (`btn`)
- **HM10_BLE_WRITE_REPEAT**: a counter and the uptime, together, every second (`count`, `up`)
- **HM10_BLE_WRITE_ULTRASONIC_SENSOR**: HC-SR04 distance, ten times a second (`dist`)
- **HM10_SETUP**: name and check an HM-10 with AT commands from the Serial Monitor

**UNO R4 Minima with an HM-10 on Serial1 (pins 0 / 1)**
- **UNO_R4_MINIMA_LED**, **UNO_R4_MINIMA_RGB_LED**: the same as the UNO R3 versions

**UNO R4 WiFi, its own radio**
- **UNO_R4_WIFI_LED**: the LED from the app's `b0`
- **UNO_R4_WIFI_SERVO**: slider `sl0` turns a servo; the board reports the angle (`angle`)
- **UNO_R4_WIFI_WRITE_BUTTON**: a push button on the board shows in the app (`btn`)
- **UNO_R4_WIFI_HUD_DRIVE**: a two-motor robot for the app's HUD Drive layout. Direction ring `r0`, knob `j0x` / `j0y`, speed `sp0`, lights `sw0`, horn `b1`, LED `b0`, servo `sl0`, aux `sl1`; it reports its battery as `bat`. The motors stop on `stop` and when the app disconnects.

## Moving from version 2

Version 3 replaces the old message format and API, so the app and the board must both be updated: the A3 Micro app 1.8 (or newer) and this library 3.

| Version 2 | Version 3 |
|---|---|
| `A3MicroManager manager(SSerial);` | `A3Micro a3(SSerial);` |
| `A3MicroManager manager;` (UNO R4 WiFi) | `A3Micro a3;` |
| `manager.isConnected()` | `a3.connected()` |
| `A3MicroMessage msg = manager.read();` then `if (msg.id == "b0") ... msg.value` | `A3MicroPacket in; if (a3.receive(in)) ... in.has("b0") ... in.get("b0")` / `in.getInt("b0")` |
| A joystick's `j0` with the value `"x,y"` | Two labels in one message: `j0x` and `j0y` |
| `manager.write("dist", String(d));` | `a3.send("dist", d);` |

Values are no longer lowercased or stripped of spaces: what the app sends is what you get (`getBool` accepts `on`, `ON`, `1`, ...).

## License

MIT License, Copyright (c) 2026 R.E Espino, TwinSparks Development ([www.twinsparksdevelopment.com](https://www.twinsparksdevelopment.com)). See [LICENSE](LICENSE).

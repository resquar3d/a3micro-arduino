# A3Micro

Arduino library for Bluetooth Low Energy (BLE) communication between the A3Micro mobile app and Arduino boards, using either an HM-10 Bluetooth module or the built-in BLE radio of the UNO R4 WiFi.

[![Get the A3 Micro app](https://img.shields.io/badge/Get_the_A3_Micro_app-a3micro.twinsparks.dev-e2b762?style=for-the-badge&logo=android&logoColor=white)](https://a3micro.twinsparks.dev/tiers.html)

This library is free and open source. The **A3 Micro app** that controls your board from an Android phone is available at **[a3micro.twinsparks.dev](https://a3micro.twinsparks.dev/tiers.html)**, with a Free edition and Premium and Pro plans. See [Get the A3 Micro app](#get-the-a3-micro-app).

## Features

- Simple message-based communication protocol
- Easy-to-use API for reading and writing BLE messages
- Support for command ID and value pairs
- Compatible with Arduino UNO R3 and R4 boards
- One `A3MicroManager` class for both transports: HM-10 BLE modules on any board, or the UNO R4 WiFi's built-in BLE (no HM-10 needed)

## Hardware Requirements

- Arduino UNO R3 or Arduino UNO R4 Minima with an HM-10 BLE module, **or**
- Arduino UNO R4 WiFi (uses its built-in BLE radio; requires the [ArduinoBLE](https://docs.arduino.cc/libraries/arduinoble/) library)
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

### Via Arduino Library Manager

1. Open Arduino IDE
2. Go to **Sketch** > **Include Library** > **Manage Libraries**
3. Search for "A3Micro"
4. Click **Install**

### Manual Installation

1. On this repository's GitHub page, click **Code** > **Download ZIP**
2. In Arduino IDE, go to **Sketch** > **Include Library** > **Add .ZIP Library...** and choose the ZIP you downloaded
3. If you are using the UNO R4 WiFi, also install the [ArduinoBLE](https://docs.arduino.cc/libraries/arduinoble/) library (the Library Manager does this automatically; a manual install does not)

## Wiring

No wiring is needed for BLE on the Arduino UNO R4 WiFi — it uses the built-in radio.

For other boards, connect the HM-10 module to your Arduino:

| HM-10 Pin | Arduino UNO R3 Pin          | Arduino UNO R4 Minima Pin |
|-----------|-----------------------------|---------------------------|
| VCC       | 5V                          | 5V                        |
| GND       | GND                         | GND                       |
| TXD       | Pin 7 (RX, SoftwareSerial)  | Pin 0 (RX, Serial1)       |
| RXD       | Pin 8 (TX, SoftwareSerial)  | Pin 1 (TX, Serial1)       |

The UNO R3 has a single hardware UART, which is used for USB, so the HM-10 examples use `SoftwareSerial` on pins 7 and 8. The UNO R4 Minima has a second hardware UART (`Serial1`) on pins 0 and 1, so the R4 Minima examples pass `Serial1` to `A3MicroManager` instead — no `SoftwareSerial` needed.

The HM-10 module ships with a default baud rate of 9600, which is what every example uses.

## Usage

### Basic Example - LED Control

```cpp
#include "A3Micro.h"
#include "SoftwareSerial.h"

// Setup BLE communication
const int rXPin = 7;
const int tXPin = 8;
SoftwareSerial SSerial(rXPin, tXPin);

// Create A3Micro manager
A3MicroManager manager(SSerial);

const int LED = 13;

void setup() {
  Serial.begin(9600);
  SSerial.begin(9600);
  manager.begin();  // Initialize A3Micro messaging
  pinMode(LED, OUTPUT);
}

void loop() {
  // Only handle messages while the A3Micro app is connected
  if (manager.isConnected()) {
    // Read message from BLE
    A3MicroMessage msg = manager.read();

    // Check if message is valid
    if (msg.hasId() && msg.hasValue()) {
      Serial.println(msg.toString());
    }

    // Control LED based on message
    if (msg.id == "b0") {
      if (msg.value == "1") {
        digitalWrite(LED, HIGH);
      } else if (msg.value == "0") {
        digitalWrite(LED, LOW);
      }
    }
  }
}
```

### UNO R4 WiFi - Built-in BLE

On the UNO R4 WiFi, construct `A3MicroManager` with no arguments to use the board's built-in BLE radio — no HM-10 or SoftwareSerial needed:

```cpp
#include "A3Micro.h"

A3MicroManager manager; // no argument = built-in BLE

const int LED = 13;

void setup() {
  Serial.begin(9600);
  if (!manager.begin("UNO R4 WIFI")) {
    Serial.println("Starting BLE failed!");
  }
  pinMode(LED, OUTPUT);
}

void loop() {
  if (manager.isConnected()) {
    A3MicroMessage msg = manager.read();
    if (msg.id == "b0") {
      digitalWrite(LED, msg.value == "1" ? HIGH : LOW);
    }
  }
}
```

The same `begin()`/`isConnected()`/`read()`/`write()` calls work in HM-10 mode too (`begin()` and `isConnected()` are harmless no-ops there, since the HM-10 manages the connection itself), so sketches can share one structure across all boards.

### Send Data to A3Micro

```cpp
// Send [1][status][2][ok][3]
manager.write("status", "ok");
```

## The A3 Micro app

The **A3 Micro app** (with a Free edition) targets Android 16, including Samsung One UI 8.5, and uses Android's own Bluetooth. It speaks the message protocol below, so every example works with it unchanged. [Get it here](https://a3micro.twinsparks.dev/tiers.html). The app itself is not part of this repository.

## MicroPython

[`micropython`](micropython) has `a3micro.py`, a MicroPython version of this library with the same classes, methods and messages. It works on boards with built-in Bluetooth, such as the ESP32 and Raspberry Pi Pico W, and with an HM-10 module on a UART. The app treats these boards exactly like an UNO R4 WiFi. See its [README](micropython/README.md).

## Message Protocol

Messages follow this format:
```
[1][ID][2][VALUE][3]
```

- Byte 1: Start delimiter
- ID: Command identifier (e.g., "b0" for button 0)
- Byte 2: Separator
- VALUE: Command value (e.g., "1" for on, "0" for off)
- Byte 3: End delimiter

## API Reference

### A3MicroMessage

Represents a BLE message with an ID and value.

#### Properties
- `String id` - Message identifier
- `String value` - Message value

#### Methods
- `bool hasId()` - Returns true if ID is non-empty
- `bool hasValue()` - Returns true if value is non-empty
- `String toString()` - Returns formatted string "id:[ID] value:[VALUE]"
- `static A3MicroMessage parse(const uint8_t *buffer, size_t size)` - Parses a raw `[1][ID][2][VALUE][3]` frame into a message; the value is lowercased and stripped of spaces

### A3MicroManager

Manages BLE communication and message parsing. One class, two transports.

#### Constructors
- `A3MicroManager(Stream &s)` - HM-10 mode: initialize with the module's Stream (Serial or SoftwareSerial)
- `A3MicroManager()` - Built-in BLE mode: use the board's own radio (UNO R4 WiFi only; requires ArduinoBLE)

#### Methods
- `bool begin(const char *deviceName = "A3Micro")` - Start BLE advertising under the given name; returns false if the radio fails to start. In HM-10 mode this is a no-op returning true (the module advertises on its own).
- `bool isConnected()` - Returns true while the A3Micro app is connected. In HM-10 mode the connection state isn't visible, so this always returns true.
- `A3MicroMessage read()` - Read and parse the latest message; returns an empty message if nothing new arrived
- `void write(const String &id, const String &value)` - Send a message to the app in the format `[1][ID][2][VALUE][3]`

## Examples

Open them from **File > Examples > A3Micro** in the Arduino IDE.

### UNO R3 with HM-10 (SoftwareSerial on pins 7/8)

- **UNO_R3_LED** - Simple LED on/off control
- **UNO_R3_RGB_LED** - RGB LED color control from three sliders
- **HM10_BLE_READ_LED** - Minimal read example: toggle the onboard LED from the app
- **HM10_BLE_WRITE_BUTTON** - Send button presses and releases to the app
- **HM10_BLE_WRITE_REPEAT** - Send an alternating 1/0 value every second
- **HM10_BLE_WRITE_ULTRASONIC_SENSOR** - Send HC-SR04 distance readings to the app

### UNO R4 Minima with HM-10 (Serial1 on pins 0/1)

- **UNO_R4_MINIMA_LED** - Simple LED on/off control
- **UNO_R4_MINIMA_RGB_LED** - RGB LED color control from three sliders

### UNO R4 WiFi with built-in BLE (no HM-10)

- **UNO_R4_WIFI_LED** - Simple LED on/off control
- **UNO_R4_WIFI_SERVO** - Servo control from a slider
- **UNO_R4_WIFI_WRITE_BUTTON** - Send button presses and releases to the app
- **UNO_R4_WIFI_HUD_DRIVE** - Two-motor robot for the app's HUD Drive layout. The reactor ring sends the direction on `r0`, its center knob sends a joystick position on `j0` for smooth steering, and its speedometer sends the top speed on `sp0`. The layout also has lights (`sw0`), a horn (`b1`), the LED (`b0`), a servo (`sl0`) and an aux output (`sl1`). The motors stop on `stop` and when the app disconnects.

### Utilities

- **Rename_HM10_Bluetooth** - Rename and configure an HM-10 module over AT commands (GPL v3, see License below)

## License

Developed for TwinSparks Development ([www.twinsparksdevelopment.com](https://www.twinsparksdevelopment.com)). Modified and written by R.E Espino of [twinsparks.dev](https://twinsparks.dev).

MIT License — Copyright (c) 2026 R.E Espino, TwinSparks Development; based on original work Copyright (c) 2026 A+ Mobile Solutions Inc. See [LICENSE](LICENSE) for details.

**Exception:** the `Rename_HM10_Bluetooth` example is a standalone HM-10 configuration utility originally written by Arik Yavilevich and modified by Anurag Purwar. It is distributed under the [GPL v3](https://www.gnu.org/licenses/gpl-3.0.html), not MIT. It does not include or link against the A3Micro library, so the library itself and all other examples remain MIT.

# Changelog

## 3.0.0

- **New A3Micro protocol** (breaking): every message, in both directions, is `##;label:value,label:value;##`. Several values travel in one message, and a `;` `:` `,` `#` or `%` inside a label or value is sent as `%3B` `%3A` `%2C` `%23` `%25`. Needs the A3 Micro app 1.8 or newer.
- **A joystick sends two labels** in one message, `j0x` and `j0y`, instead of one `"x,y"` value.
- **New API**, written from scratch: `A3Micro` (the link: `begin`, `connected`, `receive`, `send`) and `A3MicroPacket` (one message: `add`, `has`, `get`, `getInt`, `getFloat`, `getBool`, `toMessage`). See "Moving from version 2" in the README.
- **New Bluetooth LE service** for boards with their own radio: `A3C10000-9A58-4589-AD8F-6FCC3C2BF21A`, characteristic `A3C10001-9A58-4589-AD8F-6FCC3C2BF21A`.
- Messages that arrive in pieces are joined up again on every link; a message that loses its end is skipped at the next start.
- The board's replies are sent in 20-byte notifications on the UNO R4 WiFi and MicroPython boards, so long readings reach every phone.
- MicroPython library rewritten to match: `A3Micro`, `A3MicroPacket`, `receive()`, `send()`, `send_packet()`.
- All examples rewritten. `HUD_DRIVE` reads `j0x` / `j0y` and reports the battery (`bat`); `WRITE_REPEAT` sends two readings in one message; `SERVO` reports the angle.
- New example `HM10_SETUP` (name and check an HM-10 with AT commands); the GPL-licensed `Rename_HM10_Bluetooth` example was removed.
- License: MIT, Copyright R.E Espino, TwinSparks Development. All code in this version is newly written.

## Unreleased

- The A3 Micro controller is now the Android app only: the browser-based web controller has been retired. Get the app at https://a3micro.twinsparks.dev/tiers.html.
- **Breaking:** renamed the library to A3Micro. The header is now `A3Micro.h` and the classes are `A3MicroManager` and `A3MicroMessage`. The default BLE device name is now "A3Micro". The BLE UUIDs and message protocol are unchanged.
- Fixed lost messages on the UNO R4 WiFi (built-in BLE).
  - `read()` only saw the characteristic's latest value, so a message was lost whenever another arrived before `loop()` called `read()` again. This dropped the press of a quick button tap and the value of one slider when two moved together.
  - Every write is now queued, and `read()` returns the messages one per call, in order, like HM-10 mode. A write without the end delimiter still counts as a whole frame.
- Added the MicroPython library (`extras/micropython/a3micro.py`) with the same API and messages as the Arduino library. It supports built-in BLE (ESP32, Pico W) using the UNO R4 WiFi's service UUIDs, and HM-10 modules on a UART. Examples: `led.py`, `write_button.py`, `hm10_led.py`.
- Web controller and Android app: also connect to boards using the Nordic UART Service (MicroPython BLE UART).
- Web controller and Android app: new **Reactor drive** control.
  - A round HUD control. The inner ring is a 4-way direction pad sending `up`/`down`/`left`/`right`/`stop`, like the D-pad.
  - Directions work in **Hold** mode (drive while held) or **Toggle** mode (tap to latch, tap again or tap the center to stop).
  - The outer 270° speedometer sets the motor speed on its own ID (`sp0`). It has a numbered scale, green/amber/red zones and a large readout, and its range can be changed.
  - One finger can steer while another sets the speed.
  - The center is a draggable knob that moves 360° like a joystick.
    - It stays inside the direction ring, so it never reaches the speed gauge, and it never changes the speed.
    - Dragging it toward up, down, left or right sends that direction. It sends `stop` in the dead zone near the center and when you let go, and it springs back.
    - It also works like the joystick control: it sends its position as `"x,y"` on its own ID (`j0` in HUD Drive), from −range to +range with up positive, including diagonals, and `"0,0"` on release. The range is adjustable, and leaving the ID empty sends directions only. Saved reactors get a free `j` ID automatically.
    - Tapping it still stops, as the old center button did. The speed readout moved to the gap at the bottom of the gauge.
  - Latched directions are stopped before switching layouts or entering edit mode.
- New **HUD Drive** layout, now the first layout. The reactor is centered, with the lights, horn, LED and distance display on one side and the servo and aux sliders on the other. It has a hand-made portrait arrangement. Saved layouts are kept, and HUD Drive is added once.
- New example `UNO_R4_WIFI_HUD_DRIVE`: a two-motor robot (L298N) for the HUD Drive layout.
- Web controller and Android app: animated start-up intro.
  - Shows the A3 Micro emblem. The outer ring, dashed rings and arcs spin slowly in different directions, the core disc flips like a coin, and "A3 MICRO" pulses.
  - After about 3 seconds it fades into the controller, and a tap skips it.
  - The Android app is now named **A3 Micro**.
- Web controller and Android app: new HUD design.
  - Glass panels with corner brackets, cyan glow, monospace readouts and reactor rings in the background.
  - Buttons show their message ID. Switches have ON/OFF readouts, sliders have tick marks and diamond markers, and the joystick has targeting rings.
  - Controls power up one after another at start.
  - Respects the system's reduce-motion setting.
- Web controller and Android app: interface sounds and voice announcements.
  - Sounds are generated on the device: press, release, toggle, slider ticks, joystick, D-pad, connect, disconnect and alerts.
  - A British English text-to-speech voice announces start-up, connection established/terminated/lost/restored, edit mode and layout changes.
  - A new speaker button sets sounds, voice and volume.
  - The Android app speaks through Android's text-to-speech (`@capacitor-community/text-to-speech`), version 1.2.
- Web controller and Android app: portrait and landscape support.
  - Each layout keeps a second arrangement for the other orientation. It's generated automatically and can be edited: rotate the phone in edit mode to edit it.
  - The grid scales to the screen without squashing controls, and the top bar is slimmer on short screens.
  - Controls held while the phone rotates send their release message (button off, joystick 0,0, D-pad stop).
  - The Layout sheet can keep the old single-arrangement behaviour per layout, or re-run the automatic arrangement.
  - Full screen locks to the orientation in use.
- Android app: version 1.1 (versionCode 2), so it installs over 1.0 as an update.
- Android app: replaced the plugin's device picker with the app's own list. It uses a low-latency scan, shows advertised names instead of "Unknown", lists boards first and shows signal strength. It adds **Show all devices** and **Search again**, and the back gesture closes it. On Android 11 and older it asks for Location to be turned on, which those versions need to find devices.
- Added the A3Micro Web Controller (`extras/web-controller`): a Web Bluetooth controller for Chrome on Android (Android 16 / One UI 8.5), installable to the home screen, with buttons, switches, sliders, joystick, D-pad and value displays in layouts you can edit. It uses the same message protocol as the A3Micro app and connects to both the UNO R4 WiFi's built-in BLE and HM-10 modules. The library itself is unchanged.
- Added the A3Micro Controller Android app (`extras/mobile-app`): a Capacitor 8 project targeting Android 16 (API 36) that packages the same controller as an installable APK. It uses native Bluetooth LE (only the "Nearby devices" permission on Android 12+), haptics, keep-screen-on, immersive full screen, and back-gesture handling.

## 2.0.0 (2026-09-13)

**Upgrading from 1.2.0.** Existing sketches compile unchanged, but two things behave differently:

- `read()` no longer blocks. In 1.2.0 it waited up to one second for a frame to arrive, which paced the loop. It now returns immediately with an empty message when no complete frame is ready, so check `hasId()` before acting on the result (all shipped examples already do) and add your own `delay()` if your loop relied on that pause.
- The library now declares a dependency on ArduinoBLE, so the Library Manager installs it alongside A3Micro. It is only compiled in for the UNO R4 WiFi; UNO R3 and UNO R4 Minima builds are unaffected.

**Changes**

- Removed the drive examples: `UNO_R3_DRIVE`, `UNO_R3_DRIVE_SERVO_LED`, `UNO_R4_MINIMA_DRIVE`, `UNO_R4_MINIMA_DRIVE_SERVO_LED`, `UNO_R4_WIFI_DRIVE`, `UNO_R4_WIFI_DRIVE_SERVO_LED`
- Built-in BLE support for the Arduino UNO R4 WiFi: construct `A3MicroManager` with no arguments to use the board's own radio via ArduinoBLE — no HM-10 module needed
- New `begin(deviceName)` and `isConnected()` methods (no-ops in HM-10 mode, so one sketch structure works on every board)
- `read()` in HM-10 mode is now fully non-blocking: frames are assembled byte by byte across calls, so split BLE packets, dropped bytes, and HM-10 status strings (`OK+CONN`/`OK+LOST`) no longer stall the loop or corrupt the next message
- New examples: `UNO_R4_WIFI_LED`, `UNO_R4_WIFI_SERVO`, `UNO_R4_WIFI_WRITE_BUTTON`
- `HM10_BLE_WRITE_ULTRASONIC_SENSOR`: `pulseIn()` now has a 30 ms timeout so an empty field no longer stalls the loop for a second; out-of-range readings are reported instead of sent as 0
- Built-in BLE mode: `read()` and `write()` now service the BLE stack themselves, so sketches that skip `isConnected()` still receive messages
- All HM-10 examples updated to the same `begin()`/`isConnected()` structure as the built-in BLE examples, so sketches look identical across boards
- Added Arduino's official `.clang-format` and formatted all sources
- Added CI: every example compiles for its target board, plus arduino-lint
- License clarified as MIT, copyright A+ Mobile Solutions Inc; the `Rename_HM10_Bluetooth` example is documented as the one GPL v3 exception (standalone utility, does not use the library)
- `Rename_HM10_Bluetooth`: fixed deleting the SoftwareSerial through a `Stream*` (undefined behaviour on retry)

## 1.2.0 (2026-03-17)

- Added ultrasonic sensor BLE write example (`HM10_BLE_WRITE_ULTRASONIC_SENSOR`)

## 1.1.0 (2026-02-25)

- Added `write()` API for sending messages to the A3Micro app
- Added HM-10 BLE write examples (`HM10_BLE_WRITE_BUTTON`, `HM10_BLE_WRITE_REPEAT`)
- Updated library metadata

## 1.0.1 (2025-11-25)

- Initial release: `A3MicroMessage` and `A3MicroManager` for HM-10 BLE modules, with LED, RGB LED, drive, and servo examples for UNO R3 and UNO R4 Minima

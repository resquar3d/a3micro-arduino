/*
 * ESP32_HID_BRIDGE.ino - the board becomes a keyboard, mouse and media remote, steered from your phone
 *
 * The phone (A3Micro Twin, the remote screen) talks to the board over Bluetooth LE with the A3Micro protocol;
 * the board presses the keys on a computer, TV or console that has paired with it as a Bluetooth keyboard.
 * On an ESP32-S3 built with USB Mode "USB-OTG (TinyUSB)", it is also a USB keyboard and mouse: plugged into a
 * computer's USB port, the keys go down the cable instead (no pairing needed).
 *
 * Automations: up to 6 macros are stored on the board (they survive a restart) and run from the app, from the
 * BOOT button (macro 1) or on a repeat timer, also with the phone away. A macro is steps joined with | :
 *   type Hello world          types the text
 *   key ctrl+alt+t            a key or a combination: ctrl shift alt win + a letter, digit or a named key
 *                             (enter esc tab space backspace delete up down left right home end pageup
 *                              pagedown insert printscreen capslock menu f1 ... f12)
 *   media play                play next prev stop volup voldown mute home back power
 *   wait 500                  waits 500 ms
 *   move 40 -20 · click 1 · scroll -3      the mouse (click 1 left, 2 right, 3 middle)
 *   e.g. win+r | wait 400 | type notepad | key enter | wait 800 | type Hello from A3Micro
 *
 * Messages from the app (one per key): k:<usage>,s:<modifiers> keyboard · c:<usage> media ·
 *   x,y,w,b mouse move, wheel, buttons held · p:<button> click · ms:<slot>,md:<steps> store a macro ·
 *   mr:<slot> run it · me:<slot>,mt:<seconds> repeat it (0 = stop) · ml:1 list the macros
 * The board answers: links:<n> (connections to it), out:ble|usb, m<slot>:<steps>, mrun:<slot>
 *
 * Boards: ESP32, ESP32-S3, ESP32-C3 (esp32 boards package 2.x or 3.x, Bluetooth LE). Pair the computer / TV with
 * "A3 HID Bridge" in its Bluetooth settings (it shows as a keyboard); connect the phone from A3Micro Twin
 * (Connect > Bluetooth LE). Keys use the US keyboard layout.
 *
 * Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
 * Released under the MIT License (see LICENSE).
 */

#if !defined(ARDUINO_ARCH_ESP32)
#error "ESP32_HID_BRIDGE needs an ESP32, ESP32-S3 or ESP32-C3 board"
#endif

#include <A3Micro.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEHIDDevice.h>
#include <Preferences.h>
#if !defined(CONFIG_NIMBLE_ENABLED)
#include <BLE2902.h>
#endif

// The ESP32-S3 (and S2) as a USB keyboard and mouse, when built with USB Mode "USB-OTG (TinyUSB)"
#if (defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S2)) && defined(ARDUINO_USB_MODE) && !ARDUINO_USB_MODE
#define BRIDGE_USB 1
#include <USB.h>
#include <USBHIDKeyboard.h>
#include <USBHIDMouse.h>
#include <USBHIDConsumerControl.h>
USBHIDKeyboard usbKeyboard;
USBHIDMouse usbMouse;
USBHIDConsumerControl usbMedia;
volatile bool usbUp = false;     // a computer has the USB cable
#endif

const char *NAME = "A3 HID Bridge";
#if defined(CONFIG_IDF_TARGET_ESP32C3)
const int BOOT_PIN = 9;          // the BOOT button runs macro 1
#else
const int BOOT_PIN = 0;
#endif
const int MACROS = 6;

// ---------- Bluetooth: the HID keyboard / media keys / mouse, and the A3Micro service for the phone ----------
// Report 1: keyboard (modifiers, reserved, 6 keys) · Report 2: media key (16-bit usage) · Report 3: mouse
static const uint8_t REPORT_MAP[] = {
  0x05, 0x01, 0x09, 0x06, 0xA1, 0x01, 0x85, 0x01,
  0x05, 0x07, 0x19, 0xE0, 0x29, 0xE7, 0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x08, 0x81, 0x02,
  0x95, 0x01, 0x75, 0x08, 0x81, 0x01,
  0x95, 0x05, 0x75, 0x01, 0x05, 0x08, 0x19, 0x01, 0x29, 0x05, 0x91, 0x02,
  0x95, 0x01, 0x75, 0x03, 0x91, 0x01,
  0x95, 0x06, 0x75, 0x08, 0x15, 0x00, 0x25, 0x65, 0x05, 0x07, 0x19, 0x00, 0x29, 0x65, 0x81, 0x00,
  0xC0,
  0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x02,
  0x15, 0x00, 0x26, 0xFF, 0x03, 0x19, 0x00, 0x2A, 0xFF, 0x03, 0x75, 0x10, 0x95, 0x01, 0x81, 0x00,
  0xC0,
  0x05, 0x01, 0x09, 0x02, 0xA1, 0x01, 0x85, 0x03, 0x09, 0x01, 0xA1, 0x00,
  0x05, 0x09, 0x19, 0x01, 0x29, 0x03, 0x15, 0x00, 0x25, 0x01, 0x95, 0x03, 0x75, 0x01, 0x81, 0x02,
  0x95, 0x01, 0x75, 0x05, 0x81, 0x01,
  0x05, 0x01, 0x09, 0x30, 0x09, 0x31, 0x09, 0x38, 0x15, 0x81, 0x25, 0x7F, 0x75, 0x08, 0x95, 0x03, 0x81, 0x06,
  0xC0, 0xC0,
};

BLEServer *server;
BLEHIDDevice *hid;
BLECharacteristic *keyboardIn, *mediaIn, *mouseIn, *a3Char;
Preferences store;

// What the phone writes arrives on the Bluetooth task: kept in a ring here, read in loop()
const size_t RING = 1024;
char ring[RING];
volatile size_t ringHead = 0, ringTail = 0;
void ringPut(char c) {
  size_t next = (ringHead + 1) % RING;
  if (next != ringTail) { ring[ringHead] = c; ringHead = next; }
}
int links = 0;
volatile bool linksChanged = false;

class ServerEvents : public BLEServerCallbacks {
  void onConnect(BLEServer *) override {
    linksChanged = true;
    BLEDevice::startAdvertising();                 // stay visible: the phone and a computer can both connect
  }
  void onDisconnect(BLEServer *) override {
    linksChanged = true;
    BLEDevice::startAdvertising();
  }
};
class A3Events : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    auto v = c->getValue();                        // String (esp32 3.x) or std::string (2.x)
    for (size_t i = 0; i < v.length(); i++) ringPut(v[i]);
  }
};

// ---------- Where the keys go: USB when a computer has the cable (ESP32-S3), else Bluetooth ----------
bool toUsb() {
#if defined(BRIDGE_USB)
  return usbUp;
#else
  return false;
#endif
}

void keyReport(uint8_t mods, uint8_t code) {
#if defined(BRIDGE_USB)
  if (toUsb()) {
    KeyReport r = { mods, 0, { code, 0, 0, 0, 0, 0 } };
    usbKeyboard.sendReport(&r);
    return;
  }
#endif
  uint8_t r[8] = { mods, 0, code, 0, 0, 0, 0, 0 };
  keyboardIn->setValue(r, sizeof(r));
  keyboardIn->notify();
}
void tapKey(uint8_t mods, uint8_t code) { keyReport(mods, code); delay(8); keyReport(0, 0); delay(8); }

void tapMedia(uint16_t usage) {
#if defined(BRIDGE_USB)
  if (toUsb()) { usbMedia.press(usage); delay(10); usbMedia.release(); return; }
#endif
  uint8_t r[2] = { (uint8_t)(usage & 0xFF), (uint8_t)(usage >> 8) };
  mediaIn->setValue(r, 2); mediaIn->notify(); delay(10);
  uint8_t z[2] = { 0, 0 };
  mediaIn->setValue(z, 2); mediaIn->notify();
}

uint8_t heldButtons = 0;
void mouseReport(int8_t x, int8_t y, int8_t wheel, uint8_t buttons) {
#if defined(BRIDGE_USB)
  if (toUsb()) {
    for (uint8_t b = 1; b <= 4; b <<= 1) {
      if ((buttons & b) && !(heldButtons & b)) usbMouse.press(b);
      if (!(buttons & b) && (heldButtons & b)) usbMouse.release(b);
    }
    heldButtons = buttons;
    if (x || y || wheel) usbMouse.move(x, y, wheel);
    return;
  }
#endif
  heldButtons = buttons;
  uint8_t r[4] = { buttons, (uint8_t)x, (uint8_t)y, (uint8_t)wheel };
  mouseIn->setValue(r, 4); mouseIn->notify();
}
void click(uint8_t button) { mouseReport(0, 0, 0, button); delay(15); mouseReport(0, 0, 0, 0); }

// ---------- Text and key names → keyboard usages (US layout) ----------
bool charKey(char ch, uint8_t &code, uint8_t &mods) {
  const uint8_t SHIFT = 0x02;
  mods = 0;
  if (ch >= 'a' && ch <= 'z') { code = 0x04 + (ch - 'a'); return true; }
  if (ch >= 'A' && ch <= 'Z') { code = 0x04 + (ch - 'A'); mods = SHIFT; return true; }
  if (ch >= '1' && ch <= '9') { code = 0x1E + (ch - '1'); return true; }
  static const char *plain = "0 \n-=[]\\;',./`";
  static const uint8_t plainCode[] = { 0x27, 0x2C, 0x28, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x33, 0x34, 0x36, 0x37, 0x38, 0x35 };
  static const char *shifted = ")!@#$%^&*(_+{}|:\"<>?~";
  static const uint8_t shiftedCode[] = { 0x27, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x33, 0x34, 0x36, 0x37, 0x38, 0x35 };
  for (size_t i = 0; plain[i]; i++) if (plain[i] == ch) { code = plainCode[i]; return true; }
  for (size_t i = 0; shifted[i]; i++) if (shifted[i] == ch) { code = shiftedCode[i]; mods = SHIFT; return true; }
  return false;
}

struct Named { const char *name; uint8_t code; };
static const Named NAMED[] = {
  { "enter", 0x28 }, { "return", 0x28 }, { "esc", 0x29 }, { "escape", 0x29 }, { "backspace", 0x2A }, { "tab", 0x2B },
  { "space", 0x2C }, { "capslock", 0x39 }, { "printscreen", 0x46 }, { "insert", 0x49 }, { "home", 0x4A },
  { "pageup", 0x4B }, { "delete", 0x4C }, { "del", 0x4C }, { "end", 0x4D }, { "pagedown", 0x4E },
  { "right", 0x4F }, { "left", 0x50 }, { "down", 0x51 }, { "up", 0x52 }, { "menu", 0x65 },
};

// "ctrl+alt+t", "win+r", "enter", "f5", "a" → modifiers + usage
bool comboKey(String combo, uint8_t &code, uint8_t &mods) {
  combo.trim(); combo.toLowerCase();
  mods = 0; code = 0;
  while (combo.length()) {
    int plus = combo.indexOf('+');
    String part = plus < 0 ? combo : combo.substring(0, plus);
    combo = plus < 0 ? String() : combo.substring(plus + 1);
    part.trim();
    if (part == "ctrl" || part == "control") mods |= 0x01;
    else if (part == "shift") mods |= 0x02;
    else if (part == "alt") mods |= 0x04;
    else if (part == "win" || part == "gui" || part == "cmd" || part == "meta") mods |= 0x08;
    else if (part.length() >= 2 && part[0] == 'f' && isDigit(part[1])) {
      int n = part.substring(1).toInt();
      if (n >= 1 && n <= 12) code = 0x3A + (n - 1);
    } else if (part.length() == 1) {
      uint8_t extra = 0;
      if (!charKey(part[0], code, extra)) return false;
      mods |= extra;
    } else {
      bool found = false;
      for (const Named &k : NAMED) if (part == k.name) { code = k.code; found = true; }
      if (!found) return false;
    }
  }
  return code != 0 || mods != 0;
}

uint16_t mediaUsage(String name) {
  name.trim(); name.toLowerCase();
  if (name == "play" || name == "pause" || name == "playpause") return 0xCD;
  if (name == "next") return 0xB5;
  if (name == "prev" || name == "previous") return 0xB6;
  if (name == "stop") return 0xB7;
  if (name == "volup") return 0xE9;
  if (name == "voldown") return 0xEA;
  if (name == "mute") return 0xE2;
  if (name == "home") return 0x223;
  if (name == "back") return 0x224;
  if (name == "power") return 0x30;
  return 0;
}

// ---------- Messages back to the phone (in 20-byte pieces) ----------
void sendToPhone(const A3MicroPacket &packet) {
  String m = packet.toMessage();
  for (unsigned int i = 0; i < m.length(); i += 20) {
    String piece = m.substring(i, i + 20);
    a3Char->setValue((uint8_t *)piece.c_str(), piece.length());
    a3Char->notify();
    delay(6);
  }
}
void sendState() {
  A3MicroPacket p;
  p.add("links", (int)links);
  p.add("out", toUsb() ? "usb" : "ble");
  sendToPhone(p);
}

// ---------- Macros (automations), kept in flash ----------
String macroSteps[MACROS];
unsigned long repeatEvery[MACROS], lastRun[MACROS];

void runMacro(int slot) {
  if (slot < 0 || slot >= MACROS || !macroSteps[slot].length()) return;
  String steps = macroSteps[slot];
  while (steps.length()) {
    int bar = steps.indexOf('|');
    String step = bar < 0 ? steps : steps.substring(0, bar);
    steps = bar < 0 ? String() : steps.substring(bar + 1);
    step.trim();
    int space = step.indexOf(' ');
    String verb = space < 0 ? step : step.substring(0, space);
    String arg = space < 0 ? String() : step.substring(space + 1);
    verb.toLowerCase();
    uint8_t code, mods;
    if (verb == "type") {
      for (unsigned int i = 0; i < arg.length(); i++) if (charKey(arg[i], code, mods)) tapKey(mods, code);
    } else if (verb == "key") {
      if (comboKey(arg, code, mods)) tapKey(mods, code);
    } else if (verb == "media") {
      uint16_t u = mediaUsage(arg); if (u) tapMedia(u);
    } else if (verb == "wait") {
      delay(constrain(arg.toInt(), 0, 60000));
    } else if (verb == "move") {
      arg.trim(); int sp = arg.indexOf(' ');
      mouseReport(constrain(arg.toInt(), -127, 127), constrain(sp < 0 ? 0 : arg.substring(sp + 1).toInt(), -127, 127), 0, 0);
    } else if (verb == "click") {
      int b = arg.toInt(); click(b == 2 ? 2 : b == 3 ? 4 : 1);
    } else if (verb == "scroll") {
      mouseReport(0, 0, constrain(arg.toInt(), -127, 127), 0);
    } else if (step.length() && comboKey(step, code, mods)) {
      tapKey(mods, code);                           // a bare key: "enter", "ctrl+s"
    }
  }
  A3MicroPacket p; p.add("mrun", slot); sendToPhone(p);
}

void loadMacros() {
  store.begin("a3bridge", false);
  for (int i = 0; i < MACROS; i++) {
    macroSteps[i] = store.getString(("m" + String(i)).c_str(), "");
    repeatEvery[i] = store.getULong(("t" + String(i)).c_str(), 0);
    lastRun[i] = millis();
  }
}

// ---------- One message from the phone ----------
void handle(const A3MicroPacket &in) {
  if (in.has("k")) tapKey((uint8_t)in.getInt("s"), (uint8_t)in.getInt("k"));
  if (in.has("c")) tapMedia((uint16_t)in.getInt("c"));
  if (in.has("x") || in.has("y") || in.has("w") || in.has("b"))
    mouseReport((int8_t)constrain(in.getInt("x"), -127, 127), (int8_t)constrain(in.getInt("y"), -127, 127),
                (int8_t)constrain(in.getInt("w"), -127, 127), (uint8_t)in.getInt("b"));
  if (in.has("p")) { int b = in.getInt("p"); click(b == 2 ? 2 : b == 3 ? 4 : 1); }
  if (in.has("ms")) {
    int slot = in.getInt("ms");
    if (slot >= 0 && slot < MACROS) { macroSteps[slot] = in.get("md"); store.putString(("m" + String(slot)).c_str(), macroSteps[slot]); }
  }
  if (in.has("me")) {
    int slot = in.getInt("me");
    if (slot >= 0 && slot < MACROS) {
      repeatEvery[slot] = (unsigned long)max(0L, in.getInt("mt")) * 1000UL; lastRun[slot] = millis();
      store.putULong(("t" + String(slot)).c_str(), repeatEvery[slot]);
    }
  }
  if (in.has("mr")) runMacro(in.getInt("mr"));
  if (in.has("ml")) {
    sendState();
    for (int i = 0; i < MACROS; i++) { A3MicroPacket p; p.add("m" + String(i), macroSteps[i]); sendToPhone(p); }
  }
}

// Messages are put back together from the pieces: ##; ... ;##
String body;
bool inside = false;
void readPhone() {
  static char tail[3] = { 0, 0, 0 };
  while (ringTail != ringHead) {
    char c = ring[ringTail]; ringTail = (ringTail + 1) % RING;
    if (!inside) {
      tail[0] = tail[1]; tail[1] = tail[2]; tail[2] = c;
      if (tail[0] == '#' && tail[1] == '#' && tail[2] == ';') { inside = true; body = ""; }
      continue;
    }
    body += c;
    if (body.endsWith(";##")) {
      inside = false; tail[0] = tail[1] = tail[2] = 0;
      A3MicroPacket in;
      if (in.parseBody(body.c_str(), body.length() - 3)) handle(in);
    } else if (body.length() > 600) { inside = false; }    // not a message: start again
  }
}

#if defined(BRIDGE_USB)
void usbEvent(void *, esp_event_base_t base, int32_t id, void *) {
  if (base != ARDUINO_USB_EVENTS) return;
  if (id == ARDUINO_USB_STARTED_EVENT || id == ARDUINO_USB_RESUME_EVENT) usbUp = true;
  if (id == ARDUINO_USB_STOPPED_EVENT || id == ARDUINO_USB_SUSPEND_EVENT) usbUp = false;
  linksChanged = true;
}
#endif

void setup() {
  pinMode(BOOT_PIN, INPUT_PULLUP);
  loadMacros();

#if defined(BRIDGE_USB)
  USB.onEvent(usbEvent);
  usbKeyboard.begin(); usbMouse.begin(); usbMedia.begin();
  USB.begin();
#endif

  BLEDevice::init(NAME);
  server = BLEDevice::createServer();
  server->setCallbacks(new ServerEvents());

  hid = new BLEHIDDevice(server);
  keyboardIn = hid->inputReport(1);
  mediaIn = hid->inputReport(2);
  mouseIn = hid->inputReport(3);
  hid->pnp(0x02, 0x303A, 0xA3B1, 0x0300);            // vendor-id source USB, Espressif's vendor id
  hid->hidInfo(0x00, 0x01);
  hid->reportMap((uint8_t *)REPORT_MAP, sizeof(REPORT_MAP));
  hid->startServices();
  hid->setBatteryLevel(100);

#if !defined(CONFIG_NIMBLE_ENABLED)
  BLESecurity *security = new BLESecurity();         // the computer pairs (bonds) with it like a keyboard
  security->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
#endif

  BLEService *a3 = server->createService(A3MICRO_SERVICE_UUID);
  a3Char = a3->createCharacteristic(A3MICRO_CHAR_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_NOTIFY);
#if !defined(CONFIG_NIMBLE_ENABLED)
  a3Char->addDescriptor(new BLE2902());
#endif
  a3Char->setCallbacks(new A3Events());
  a3->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->setAppearance(0x03C1);                        // a keyboard
  adv->addServiceUUID(hid->hidService()->getUUID());
  adv->setScanResponse(true);
  BLEDevice::startAdvertising();
}

void loop() {
  readPhone();

  if (linksChanged) {                                // counted here, once the Bluetooth stack has settled
    linksChanged = false; delay(50);
    links = server->getConnectedCount();
    if (links > 0) sendState();
  }

  // The BOOT button runs macro 1 (on release, after 30 ms of steady press)
  static bool wasDown = false;
  static unsigned long downAt = 0;
  bool down = digitalRead(BOOT_PIN) == LOW;
  if (down && !wasDown) downAt = millis();
  if (!down && wasDown && millis() - downAt > 30) runMacro(0);
  wasDown = down;

  // Repeating macros
  for (int i = 0; i < MACROS; i++) {
    if (repeatEvery[i] && millis() - lastRun[i] >= repeatEvery[i]) { lastRun[i] = millis(); runMacro(i); }
  }
  delay(2);
}

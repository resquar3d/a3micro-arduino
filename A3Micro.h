/*
 * A3Micro.h - talk to the A3Micro Control app from an Arduino-compatible board
 *
 * Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
 * Released under the MIT License (see LICENSE).
 *
 * THE A3MICRO PROTOCOL
 * Every message, in both directions, is one line of text:
 *
 *     ##;label1:value1,label2:value2,...,labelN:valueN;##
 *
 *   ##      start of a message
 *   ;       ends the start marker and starts the end marker
 *   :       between a label and its value
 *   ,       between two label:value pairs
 *   ##      end of a message
 *
 * A label names one input or reading, e.g. j1x (joystick 1, x direction), sl0 (slider 0), bat (battery).
 * The app sends one message per action (a button press, a moved slider or joystick, a Send);
 * the board answers in the same format (battery voltage, a measured distance, ...).
 * A ; : , # or % inside a label or value is sent as %3B %3A %2C %23 %25, so it can't break a message.
 *
 * HOW THE BOARD CONNECTS
 *   A3Micro a3(Serial1);   an HM-10 (or any serial BLE module) wired to a serial port: any board
 *   A3Micro a3;            the board's own Bluetooth LE radio (UNO R4 WiFi)
 * Both work the same way:
 *
 *   A3MicroPacket in;
 *   if (a3.receive(in)) {
 *     long x = in.getInt("j1x"), y = in.getInt("j1y");
 *   }
 *   a3.send("bat", 3.71);              // one reading:      ##;bat:3.71;##
 *   A3MicroPacket out;                 // several at once:  ##;bat:3.71,dist:42;##
 *   out.add("bat", 3.71); out.add("dist", 42); a3.send(out);
 */

#ifndef A3MICRO_H
#define A3MICRO_H

#include <Arduino.h>

// The board's own BLE radio is used where the board has one. (The board macro, not __has_include, so the
// Arduino IDE sees the ArduinoBLE dependency.)
#if defined(ARDUINO_UNOWIFIR4)
#define A3MICRO_BUILTIN_BLE 1
#include <ArduinoBLE.h>
#endif

#ifndef A3MICRO_MAX_PAIRS
#define A3MICRO_MAX_PAIRS 8      // label:value pairs one message can carry
#endif
#ifndef A3MICRO_MAX_MESSAGE
#define A3MICRO_MAX_MESSAGE 160  // longest message accepted, in characters (between the markers)
#endif

// The A3Micro BLE service and its one characteristic (write from the app, notify back to it)
#define A3MICRO_SERVICE_UUID "A3C10000-9A58-4589-AD8F-6FCC3C2BF21A"
#define A3MICRO_CHAR_UUID    "A3C10001-9A58-4589-AD8F-6FCC3C2BF21A"

// One message: a list of label:value pairs
class A3MicroPacket {
public:
  A3MicroPacket();

  // Adds a pair (false when the packet is full). Numbers are written as text; floats with `decimals` digits.
  bool add(const String &label, const String &value);
  bool add(const String &label, const char *value);
  bool add(const String &label, long value);
  bool add(const String &label, int value);
  bool add(const String &label, unsigned long value);
  bool add(const String &label, double value, unsigned char decimals = 2);
  bool add(const String &label, bool value);
  void clear();

  uint8_t size() const;
  bool empty() const;
  const String &label(uint8_t i) const;
  const String &value(uint8_t i) const;

  // Looking up a label (the last one wins when a label appears twice)
  bool has(const char *label) const;
  String get(const char *label, const String &fallback = "") const;
  long getInt(const char *label, long fallback = 0) const;
  float getFloat(const char *label, float fallback = 0) const;
  bool getBool(const char *label, bool fallback = false) const;   // 1 / on / true / yes = true

  // The whole message as sent: ##;label:value,...;##
  String toMessage() const;
  // Fills the packet from what is between ##; and ;## (false when it holds no valid pair)
  bool parseBody(const char *body, size_t length);

  // ; : , # % as %3B %3A %2C %23 %25, and back
  static String escape(const String &text);
  static String unescape(const char *text, size_t length);

private:
  String _label[A3MICRO_MAX_PAIRS];
  String _value[A3MICRO_MAX_PAIRS];
  uint8_t _count;
  int indexOf(const char *label) const;
};

// The link to the app: receives and sends A3Micro messages
class A3Micro {
public:
  explicit A3Micro(Stream &serial);   // a serial BLE module (HM-10 and similar)
#if defined(A3MICRO_BUILTIN_BLE)
  A3Micro();                          // the board's own BLE radio
#endif

  // Starts advertising as `name` (own radio). A serial module advertises by itself: always true.
  bool begin(const char *name = "A3Micro");

  // True while the app is connected (own radio). A serial module can't tell: always true.
  bool connected();

  // True, with the message in `packet`, when a complete message has arrived. Never waits:
  // false when nothing (or only part of a message) has arrived yet. One message per call.
  bool receive(A3MicroPacket &packet);

  void send(const A3MicroPacket &packet);
  void send(const String &label, const String &value);
  void send(const String &label, const char *value);
  void send(const String &label, long value);
  void send(const String &label, int value);
  void send(const String &label, double value, unsigned char decimals = 2);

private:
  Stream *_serial;   // nullptr when the board's own radio is used

  // Message assembly, one character at a time (messages may arrive in pieces)
  char _body[A3MICRO_MAX_MESSAGE + 1];
  size_t _length;
  bool _inside;      // between ##; and ;##
  char _tail[3];     // the last three characters seen outside a message (to spot ##;)
  bool take(char c, A3MicroPacket &packet);
  void sendText(const String &message);

#if defined(A3MICRO_BUILTIN_BLE)
  BLEService _service;
  BLECharacteristic _characteristic;
  // What the app wrote, kept in order until receive() reads it
  static const size_t QUEUE_SIZE = 512;
  char _queue[QUEUE_SIZE];
  size_t _qHead, _qTail;   // queue: next write / next read
  void enqueue(const uint8_t *data, size_t length);
  static A3Micro *_self;
  static void written(BLEDevice central, BLECharacteristic characteristic);
#endif
};

#endif

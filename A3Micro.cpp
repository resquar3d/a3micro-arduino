/*
 * A3Micro.cpp - the A3Micro protocol: ##;label:value,...;## (see A3Micro.h)
 *
 * Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
 * Released under the MIT License (see LICENSE).
 */

#include "A3Micro.h"

/* ---------------------------------------------------------------------------------------------------------------
 * A3MicroPacket: a list of label:value pairs
 * ------------------------------------------------------------------------------------------------------------- */

A3MicroPacket::A3MicroPacket() : _count(0) {}

bool A3MicroPacket::add(const String &label, const String &value) {
  if (_count >= A3MICRO_MAX_PAIRS || label.length() == 0) return false;
  _label[_count] = label;
  _value[_count] = value;
  _count++;
  return true;
}
bool A3MicroPacket::add(const String &label, const char *value) { return add(label, String(value)); }
bool A3MicroPacket::add(const String &label, long value) { return add(label, String(value)); }
bool A3MicroPacket::add(const String &label, int value) { return add(label, String(value)); }
bool A3MicroPacket::add(const String &label, unsigned long value) { return add(label, String(value)); }
bool A3MicroPacket::add(const String &label, double value, unsigned char decimals) { return add(label, String(value, decimals)); }
bool A3MicroPacket::add(const String &label, bool value) { return add(label, String(value ? "1" : "0")); }

void A3MicroPacket::clear() {
  for (uint8_t i = 0; i < _count; i++) { _label[i] = ""; _value[i] = ""; }
  _count = 0;
}

uint8_t A3MicroPacket::size() const { return _count; }
bool A3MicroPacket::empty() const { return _count == 0; }

const String &A3MicroPacket::label(uint8_t i) const {
  static const String none;
  return i < _count ? _label[i] : none;
}
const String &A3MicroPacket::value(uint8_t i) const {
  static const String none;
  return i < _count ? _value[i] : none;
}

int A3MicroPacket::indexOf(const char *label) const {
  for (int i = (int)_count - 1; i >= 0; i--) {   // newest first: the last value for a label wins
    if (_label[i] == label) return i;
  }
  return -1;
}

bool A3MicroPacket::has(const char *label) const { return indexOf(label) >= 0; }

String A3MicroPacket::get(const char *label, const String &fallback) const {
  int i = indexOf(label);
  return i >= 0 ? _value[i] : fallback;
}

long A3MicroPacket::getInt(const char *label, long fallback) const {
  int i = indexOf(label);
  if (i < 0 || _value[i].length() == 0) return fallback;
  return _value[i].toInt();
}

float A3MicroPacket::getFloat(const char *label, float fallback) const {
  int i = indexOf(label);
  if (i < 0 || _value[i].length() == 0) return fallback;
  return _value[i].toFloat();
}

bool A3MicroPacket::getBool(const char *label, bool fallback) const {
  int i = indexOf(label);
  if (i < 0) return fallback;
  String v = _value[i];
  v.toLowerCase();
  if (v == "1" || v == "on" || v == "true" || v == "yes" || v == "high") return true;
  if (v == "0" || v == "off" || v == "false" || v == "no" || v == "low") return false;
  return fallback;
}

String A3MicroPacket::toMessage() const {
  String m = "##;";
  for (uint8_t i = 0; i < _count; i++) {
    if (i) m += ',';
    m += escape(_label[i]);
    m += ':';
    m += escape(_value[i]);
  }
  m += ";##";
  return m;
}

// Splits "label:value,label:value" into pairs; pairs without a label are skipped
bool A3MicroPacket::parseBody(const char *body, size_t length) {
  clear();
  size_t start = 0;
  while (start <= length) {
    size_t end = start;
    while (end < length && body[end] != ',') end++;
    // one pair: body[start, end)
    size_t colon = start;
    while (colon < end && body[colon] != ':') colon++;
    String label = unescape(body + start, colon - start);
    String value = colon < end ? unescape(body + colon + 1, end - colon - 1) : String();
    label.trim();
    value.trim();
    if (label.length() > 0 && _count < A3MICRO_MAX_PAIRS) {
      _label[_count] = label;
      _value[_count] = value;
      _count++;
    }
    start = end + 1;
  }
  return _count > 0;
}

String A3MicroPacket::escape(const String &text) {
  String out;
  out.reserve(text.length());
  for (unsigned int i = 0; i < text.length(); i++) {
    char c = text[i];
    switch (c) {
      case ';': out += "%3B"; break;
      case ':': out += "%3A"; break;
      case ',': out += "%2C"; break;
      case '#': out += "%23"; break;
      case '%': out += "%25"; break;
      default: out += c;
    }
  }
  return out;
}

static int hexDigit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

String A3MicroPacket::unescape(const char *text, size_t length) {
  String out;
  out.reserve(length);
  for (size_t i = 0; i < length; i++) {
    if (text[i] == '%' && i + 2 < length) {
      int hi = hexDigit(text[i + 1]), lo = hexDigit(text[i + 2]);
      if (hi >= 0 && lo >= 0) {
        out += (char)(hi * 16 + lo);
        i += 2;
        continue;
      }
    }
    out += text[i];
  }
  return out;
}

/* ---------------------------------------------------------------------------------------------------------------
 * A3Micro: the link
 * ------------------------------------------------------------------------------------------------------------- */

A3Micro::A3Micro(Stream &serial)
  : _serial(&serial), _length(0), _inside(false)
#if defined(A3MICRO_BUILTIN_BLE)
  , _service(A3MICRO_SERVICE_UUID),
    _characteristic(A3MICRO_CHAR_UUID, BLERead | BLEWrite | BLEWriteWithoutResponse | BLENotify, 200),
    _qHead(0), _qTail(0)
#endif
{
  _tail[0] = _tail[1] = _tail[2] = 0;
}

#if defined(A3MICRO_BUILTIN_BLE)
A3Micro *A3Micro::_self = nullptr;

A3Micro::A3Micro()
  : _serial(nullptr), _length(0), _inside(false),
    _service(A3MICRO_SERVICE_UUID),
    _characteristic(A3MICRO_CHAR_UUID, BLERead | BLEWrite | BLEWriteWithoutResponse | BLENotify, 200),
    _qHead(0), _qTail(0) {
  _tail[0] = _tail[1] = _tail[2] = 0;
}

// ArduinoBLE calls this (from BLE.poll()) for each write by the app. The characteristic keeps only the newest
// value, so every write is copied into the queue straight away.
void A3Micro::written(BLEDevice central, BLECharacteristic characteristic) {
  (void)central;
  if (_self) _self->enqueue(characteristic.value(), (size_t)characteristic.valueLength());
}

void A3Micro::enqueue(const uint8_t *data, size_t length) {
  if (!data) return;
  for (size_t i = 0; i < length; i++) {
    size_t next = (_qHead + 1) % QUEUE_SIZE;
    if (next == _qTail) return;               // full: receive() isn't being called often enough
    _queue[_qHead] = (char)data[i];
    _qHead = next;
  }
}
#endif

bool A3Micro::begin(const char *name) {
  if (_serial) { (void)name; return true; }   // a serial module advertises by itself, under its own name
#if defined(A3MICRO_BUILTIN_BLE)
  if (!BLE.begin()) return false;
  BLE.setLocalName(name);
  BLE.setDeviceName(name);
  BLE.setAdvertisedService(_service);
  _service.addCharacteristic(_characteristic);
  BLE.addService(_service);
  _self = this;
  _characteristic.setEventHandler(BLEWritten, written);
  return BLE.advertise();
#else
  return false;
#endif
}

bool A3Micro::connected() {
  if (_serial) return true;
#if defined(A3MICRO_BUILTIN_BLE)
  BLEDevice app = BLE.central();
  return app && app.connected();
#else
  return false;
#endif
}

// Feeds one character into the message being assembled. True when it completes a message with pairs in it.
bool A3Micro::take(char c, A3MicroPacket &packet) {
  if (!_inside) {
    _tail[0] = _tail[1]; _tail[1] = _tail[2]; _tail[2] = c;
    if (_tail[0] == '#' && _tail[1] == '#' && _tail[2] == ';') {   // ##; : a message starts
      _inside = true;
      _length = 0;
      _tail[0] = _tail[1] = _tail[2] = 0;
    }
    return false;
  }
  if (_length >= A3MICRO_MAX_MESSAGE) {                            // too long: drop it, look for the next start
    _inside = false;
    _length = 0;
    return false;
  }
  _body[_length++] = c;
  if (_length >= 3 && _body[_length - 3] == ';' && _body[_length - 2] == '#' && _body[_length - 1] == '#') {   // ;## : the end
    _inside = false;
    size_t bodyLength = _length - 3;
    _body[bodyLength] = 0;
    _length = 0;
    return packet.parseBody(_body, bodyLength);
  }
  if (_length >= 3 && _body[_length - 3] == '#' && _body[_length - 2] == '#' && _body[_length - 1] == ';') {   // a new start: the last message never ended
    _length = 0;
  }
  return false;
}

bool A3Micro::receive(A3MicroPacket &packet) {
  if (_serial) {
    while (_serial->available() > 0) {
      if (take((char)_serial->read(), packet)) return true;   // the rest waits in the serial buffer
    }
    return false;
  }
#if defined(A3MICRO_BUILTIN_BLE)
  BLE.poll();
  while (_qTail != _qHead) {
    char c = _queue[_qTail];
    _qTail = (_qTail + 1) % QUEUE_SIZE;
    if (take(c, packet)) return true;
  }
#endif
  return false;
}

void A3Micro::sendText(const String &message) {
  if (_serial) { _serial->print(message); return; }
#if defined(A3MICRO_BUILTIN_BLE)
  BLE.poll();
  // In pieces of 20 bytes: what fits in one notification on every phone; the app joins them up again
  const char *p = message.c_str();
  size_t left = message.length();
  while (left > 0) {
    size_t n = left < 20 ? left : 20;
    _characteristic.writeValue((const uint8_t *)p, n);
    p += n;
    left -= n;
  }
#endif
}

void A3Micro::send(const A3MicroPacket &packet) {
  if (!packet.empty()) sendText(packet.toMessage());
}
void A3Micro::send(const String &label, const String &value) {
  A3MicroPacket p;
  p.add(label, value);
  send(p);
}
void A3Micro::send(const String &label, const char *value) { send(label, String(value)); }
void A3Micro::send(const String &label, long value) { send(label, String(value)); }
void A3Micro::send(const String &label, int value) { send(label, String(value)); }
void A3Micro::send(const String &label, double value, unsigned char decimals) { send(label, String(value, decimals)); }

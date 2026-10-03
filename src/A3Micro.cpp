/*
 * A3Micro library - src/A3Micro.cpp
 *
 * Description:
 * Source file implementing the `A3MicroMessage` and `A3MicroManager` classes. Provides
 * methods to receive, parse, and process BLE messages for control tasks on Arduino, such as
 * handling commands to operate devices. `A3MicroManager` supports both an HM-10 BLE module
 * on a serial stream and (on the UNO R4 WiFi) the board's built-in BLE radio.
 *
 * Developed for TwinSparks Development (www.twinsparksdevelopment.com)
 * Modified and written by R.E Espino of twinsparks.dev
 * Licensed under the MIT License. See LICENSE for details.
 */

#include "A3Micro.h"

// Constructor for A3MicroMessage
A3MicroMessage::A3MicroMessage()
  : id(""), value("") {}

// Checks if the message has a non-empty ID
bool A3MicroMessage::hasId() const {
  return id.length() > 0;
}

// Checks if the message has a non-empty value
bool A3MicroMessage::hasValue() const {
  return value.length() > 0;
}

// Converts the message to a readable string format
String A3MicroMessage::toString() const {
  return "id:" + id + " value:" + value;
}

// Parses a raw frame in the format [1][ID][2][VALUE][3] into a message.
// The end delimiter (3) may be absent when the transport strips it.
A3MicroMessage A3MicroMessage::parse(const uint8_t *buffer, size_t size) {
  A3MicroMessage msg;

  if (size > 0 && buffer[0] == 1) {
    size_t i = 1;
    // Extract the ID part (between 1 and 2 delimiters)
    while (i < size && buffer[i] != 2) {
      msg.id += (char)buffer[i++];
    }

    i++;  // Skip the delimiter (2)

    // Extract the Value part (between 2 and 3 delimiters)
    while (i < size && buffer[i] != 3) {
      msg.value += (char)buffer[i++];
    }
  }

  // Clean up the value string by standardizing it
  msg.value.toLowerCase();     // Convert all characters to lowercase
  msg.value.replace(" ", "");  // Remove any spaces

  return msg;
}

#if defined(A3MICRO_HAS_BUILTIN_BLE)
// UUIDs the A3Micro app uses to discover the board and exchange messages
static const char *A3MICRO_SERVICE_UUID = "19B10000-E8F2-537E-4F6C-D104768A1214";
static const char *A3MICRO_CHARACTERISTIC_UUID = "19B10001-E8F2-537E-4F6C-D104768A1214";
#endif

// HM-10 mode: communicates through the module's serial stream
A3MicroManager::A3MicroManager(Stream &s)
  : _s(&s), _frameLength(0), _inFrame(false)
#if defined(A3MICRO_HAS_BUILTIN_BLE)
    ,
    _service(A3MICRO_SERVICE_UUID),
    _characteristic(A3MICRO_CHARACTERISTIC_UUID, BLERead | BLEWrite | BLEWriteWithoutResponse | BLENotify, 100),
    _rxHead(0), _rxTail(0)
#endif
{
}

#if defined(A3MICRO_HAS_BUILTIN_BLE)
// Built-in BLE mode: communicates through the board's own radio
A3MicroManager::A3MicroManager()
  : _s(nullptr), _frameLength(0), _inFrame(false),
    _service(A3MICRO_SERVICE_UUID),
    _characteristic(A3MICRO_CHARACTERISTIC_UUID, BLERead | BLEWrite | BLEWriteWithoutResponse | BLENotify, 100),
    _rxHead(0), _rxTail(0) {}

A3MicroManager *A3MicroManager::_active = nullptr;

// Called by ArduinoBLE (inside BLE.poll()) for every write from the app. The
// characteristic only holds the latest value, so each write is queued here
// before the next one can overwrite it.
void A3MicroManager::onWritten(BLEDevice central, BLECharacteristic characteristic) {
  (void)central;
  if (_active) {
    _active->queueRx(characteristic.value(), (size_t)characteristic.valueLength());
  }
}

void A3MicroManager::queueRx(const uint8_t *data, size_t length) {
  if (!data) {
    return;
  }
  for (size_t i = 0; i < length; i++) {
    pushRx(data[i]);
  }
  // Each write carries one whole frame. A write that starts a frame without
  // ending it is still taken as a complete frame, as in earlier versions.
  if (length > 0 && data[0] == 1 && memchr(data, 3, length) == nullptr) {
    pushRx(3);
  }
}

void A3MicroManager::pushRx(uint8_t b) {
  size_t next = (_rxHead + 1) % RX_BUFFER_SIZE;
  if (next == _rxTail) {
    return;  // Full: the sketch isn't calling read(); drop the newest bytes
  }
  _rx[_rxHead] = b;
  _rxHead = next;
}
#endif

bool A3MicroManager::feed(uint8_t b, A3MicroMessage &msg) {
  if (b == 1) {
    // Start delimiter: begin a fresh frame, discarding any partial one
    _frameLength = 0;
    _frame[_frameLength++] = b;
    _inFrame = true;
    return false;
  }

  if (!_inFrame) {
    return false;  // Not inside a frame: ignore noise and module status text
  }

  if (b == 3) {
    // End delimiter: frame complete
    _inFrame = false;
    msg = A3MicroMessage::parse(_frame, _frameLength);
    _frameLength = 0;
    return true;
  }

  if (_frameLength >= FRAME_BUFFER_SIZE) {
    // Oversized frame with no terminator: drop it and wait for the next start
    _inFrame = false;
    _frameLength = 0;
    return false;
  }

  _frame[_frameLength++] = b;
  return false;
}

// Starts BLE advertising under the given name. In HM-10 mode the module
// advertises on its own, so there is nothing to do and this returns true.
bool A3MicroManager::begin(const char *deviceName) {
  if (_s) {
    (void)deviceName;  // Unused in HM-10 mode: the module keeps its own name
    return true;       // HM-10 handles advertising itself
  }

#if defined(A3MICRO_HAS_BUILTIN_BLE)
  if (!BLE.begin()) {
    return false;
  }

  BLE.setLocalName(deviceName);
  BLE.setDeviceName(deviceName);
  BLE.setAdvertisedService(_service);
  _active = this;
  _characteristic.setEventHandler(BLEWritten, onWritten);
  _service.addCharacteristic(_characteristic);
  BLE.addService(_service);
  BLE.advertise();
#endif

  return true;
}

// Reports whether the A3Micro app is connected. In HM-10 mode the module
// doesn't expose connection state, so this always returns true.
bool A3MicroManager::isConnected() {
  if (_s) {
    return true;
  }

#if defined(A3MICRO_HAS_BUILTIN_BLE)
  BLEDevice central = BLE.central();  // also services BLE events
  return central && central.connected();
#else
  return false;
#endif
}

// Reads a message from the app, parsing ID and value.
// Returns an empty message when no complete frame has arrived yet.
// Returns one message per call; any further messages already received wait
// for the next call, so none are lost.
A3MicroMessage A3MicroManager::read() {
  A3MicroMessage msg;

  if (_s) {
    // HM-10 mode: consume only the bytes already waiting, one at a time, and
    // assemble frames across calls. This never blocks, tolerates frames split
    // across BLE packets, and resynchronises immediately after a dropped byte
    // or a stray module status string such as "OK+CONN". Bytes after a
    // complete frame stay in the serial buffer for the next call.
    while (_s->available() > 0) {
      if (feed((uint8_t)_s->read(), msg)) {
        return msg;
      }
    }
    return A3MicroMessage();  // No complete frame yet
  }

#if defined(A3MICRO_HAS_BUILTIN_BLE)
  // Service the BLE stack so incoming writes and connection events are
  // processed even if the sketch never calls isConnected(). Every write is
  // queued by onWritten().
  BLE.poll();
  while (_rxTail != _rxHead) {
    uint8_t b = _rx[_rxTail];
    _rxTail = (_rxTail + 1) % RX_BUFFER_SIZE;
    if (feed(b, msg)) {
      return msg;
    }
  }
#endif

  return A3MicroMessage();
}

// Writes a message to the app using the protocol [1][ID][2][VALUE][3]
void A3MicroManager::write(const String &id, const String &value) {
  if (_s) {
    _s->write((uint8_t)1);
    _s->print(id);
    _s->write((uint8_t)2);
    _s->print(value);
    _s->write((uint8_t)3);
    return;
  }

#if defined(A3MICRO_HAS_BUILTIN_BLE)
  BLE.poll();  // Keep the BLE stack serviced in write-only sketches
  String frame;
  frame += (char)1;
  frame += id;
  frame += (char)2;
  frame += value;
  frame += (char)3;
  _characteristic.writeValue((const uint8_t *)frame.c_str(), frame.length());
#endif
}

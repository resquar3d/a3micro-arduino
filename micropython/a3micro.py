"""
a3micro.py - talk to the A3Micro Control app from a MicroPython board (ESP32, Raspberry Pi Pico W, ...)

Copyright (c) 2026 R.E Espino, TwinSparks Development (www.twinsparksdevelopment.com)
Released under the MIT License (see LICENSE).

THE A3MICRO PROTOCOL (the same as the Arduino library, A3Micro.h)
Every message, in both directions, is one line of text:

    ##;label1:value1,label2:value2,...,labelN:valueN;##

  ##   start          ;   between the markers and the pairs
  :    label:value    ,   between pairs          ##   end
A ; : , # or % inside a label or value travels as %3B %3A %2C %23 %25.

    a3 = A3Micro()              # the board's own Bluetooth LE radio
    a3 = A3Micro(uart)          # an HM-10 (or similar) on a machine.UART
    a3.begin("Rover")
    while True:
        packet = a3.receive()   # None until a complete message has arrived
        if packet:
            x = packet.get_int("j1x")
        a3.send("bat", 3.71)                         # ##;bat:3.71;##
        a3.send_packet({"bat": 3.71, "dist": 42})    # ##;bat:3.71,dist:42;##
"""

from micropython import const

try:
    import bluetooth
except ImportError:          # a board without Bluetooth: only a serial module (UART) can be used
    bluetooth = None

# The A3Micro BLE service and its characteristic (the same as A3Micro.h)
SERVICE_UUID = "A3C10000-9A58-4589-AD8F-6FCC3C2BF21A"
CHAR_UUID = "A3C10001-9A58-4589-AD8F-6FCC3C2BF21A"

MAX_MESSAGE = const(160)     # longest message accepted, in characters (between the markers)
_QUEUE_LIMIT = const(1024)   # characters kept waiting between receive() calls

_EV_CONNECT = const(1)
_EV_DISCONNECT = const(2)
_EV_WRITE = const(3)
_EV_MTU = const(21)
# Characteristic flags (numbers: older MicroPython builds don't name them)
_READ = const(0x0002)
_WRITE_NO_RESPONSE = const(0x0004)
_WRITE = const(0x0008)
_NOTIFY = const(0x0010)

_ESCAPES = {";": "%3B", ":": "%3A", ",": "%2C", "#": "%23", "%": "%25"}
_TRUE = ("1", "on", "true", "yes", "high")
_FALSE = ("0", "off", "false", "no", "low")


def escape(text):
    """; : , # % as %3B %3A %2C %23 %25"""
    return "".join(_ESCAPES.get(c, c) for c in str(text))


def unescape(text):
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c == "%" and i + 2 < n:
            try:
                out.append(chr(int(text[i + 1:i + 3], 16)))
                i += 3
                continue
            except ValueError:
                pass
        out.append(c)
        i += 1
    return "".join(out)


class A3MicroPacket:
    """One message: label:value pairs, in order."""

    def __init__(self, pairs=None):
        self.pairs = []
        if pairs:
            for label, value in (pairs.items() if isinstance(pairs, dict) else pairs):
                self.add(label, value)

    def add(self, label, value):
        label = str(label)
        if label:
            if isinstance(value, bool):
                value = "1" if value else "0"
            self.pairs.append((label, str(value)))
        return self

    def __len__(self):
        return len(self.pairs)

    def __iter__(self):
        return iter(self.pairs)

    def has(self, label):
        for l, _ in self.pairs:
            if l == label:
                return True
        return False

    def get(self, label, fallback=None):
        for l, v in reversed(self.pairs):    # the last value for a label wins
            if l == label:
                return v
        return fallback

    def get_int(self, label, fallback=0):
        v = self.get(label)
        try:
            return int(float(v)) if v else fallback
        except ValueError:
            return fallback

    def get_float(self, label, fallback=0.0):
        v = self.get(label)
        try:
            return float(v) if v else fallback
        except ValueError:
            return fallback

    def get_bool(self, label, fallback=False):
        v = self.get(label)
        if v is None:
            return fallback
        v = v.lower()
        if v in _TRUE:
            return True
        if v in _FALSE:
            return False
        return fallback

    def to_message(self):
        return "##;" + ",".join(escape(l) + ":" + escape(v) for l, v in self.pairs) + ";##"

    @staticmethod
    def parse_body(body):
        """A packet from what is between ##; and ;## (None when it holds no valid pair)."""
        packet = A3MicroPacket()
        for part in body.split(","):
            label, _, value = part.partition(":")
            label = unescape(label).strip()
            if label:
                packet.pairs.append((label, unescape(value).strip()))
        return packet if packet.pairs else None

    def __repr__(self):
        return "A3MicroPacket(" + repr(self.pairs) + ")"


class A3Micro:
    """The link to the app."""

    def __init__(self, uart=None, ble=None):
        self._uart = uart
        self._ble = None
        self._handle = None
        self._apps = {}           # connected app -> payload bytes per notification
        self._name = "A3Micro"
        self._queue = []          # characters received, not yet read
        self._inside = False      # between ##; and ;##
        self._body = []
        self._tail = ""           # last characters seen outside a message
        if uart is None:
            if bluetooth is None:
                raise OSError("This board has no Bluetooth: pass the UART of a serial BLE module")
            self._ble = ble or bluetooth.BLE()

    def begin(self, name="A3Micro"):
        """Starts advertising as `name` (own radio). A serial module advertises by itself.
        False when the radio doesn't start."""
        if self._ble is None:
            return True
        try:
            self._name = name
            self._ble.active(True)
            for option in ({"gap_name": name}, {"mtu": 256}):
                try:
                    self._ble.config(**option)
                except Exception:
                    pass
            self._ble.irq(self._event)
            flags = _READ | _WRITE | _WRITE_NO_RESPONSE | _NOTIFY
            ((self._handle,),) = self._ble.gatts_register_services(
                ((bluetooth.UUID(SERVICE_UUID), ((bluetooth.UUID(CHAR_UUID), flags),)),))
            # Append mode: writes that arrive between two reads are all kept
            self._ble.gatts_set_buffer(self._handle, 256, True)
            self._advertise()
            return True
        except Exception:
            return False

    def connected(self):
        """True while the app is connected (own radio). A serial module can't tell: always True."""
        return True if self._ble is None else len(self._apps) > 0

    def receive(self):
        """The next complete message as an A3MicroPacket, or None. Never waits."""
        if self._uart is not None:
            n = self._uart.any()
            if n:
                data = self._uart.read(n)
                if data:
                    self._queue.extend(chr(b) for b in data)
                    del self._queue[:-_QUEUE_LIMIT]
        while self._queue:
            packet = self._take(self._queue.pop(0))
            if packet:
                return packet
        return None

    def send(self, label, value):
        """One label:value pair: ##;label:value;##"""
        self.send_packet(A3MicroPacket().add(label, value))

    def send_packet(self, packet):
        """Several pairs at once: an A3MicroPacket, a dict or a list of (label, value)."""
        if not isinstance(packet, A3MicroPacket):
            packet = A3MicroPacket(packet)
        if len(packet):
            self._send_text(packet.to_message())

    # ---- inside ----
    def _take(self, c):
        if not self._inside:
            self._tail = (self._tail + c)[-3:]
            if self._tail == "##;":
                self._inside, self._body, self._tail = True, [], ""
            return None
        if len(self._body) >= MAX_MESSAGE:          # too long: drop it, look for the next start
            self._inside, self._body = False, []
            return None
        self._body.append(c)
        end = "".join(self._body[-3:])
        if end == ";##":
            self._inside = False
            body, self._body = "".join(self._body[:-3]), []
            return A3MicroPacket.parse_body(body)
        if end == "##;":                            # a new start: the last message never ended
            self._body = []
        return None

    def _send_text(self, text):
        data = text.encode()
        if self._uart is not None:
            self._uart.write(data)
            return
        if self._handle is None:
            return
        self._ble.gatts_write(self._handle, data)
        for app, size in tuple(self._apps.items()):
            try:
                for i in range(0, len(data), size):  # in pieces the app joins up again
                    self._ble.gatts_notify(app, self._handle, data[i:i + size])
            except Exception:
                pass                                 # the app went away mid-message

    def _advertise(self):
        # Flags + the 128-bit service fill 21 of the 31 advertising bytes: the name goes in the scan response
        uuid = bytes(bluetooth.UUID(SERVICE_UUID))
        adv = bytes((2, 0x01, 0x06)) + bytes((len(uuid) + 1, 0x07)) + uuid
        name = self._name.encode()[:29]
        self._ble.gap_advertise(100000, adv_data=adv, resp_data=bytes((len(name) + 1, 0x09)) + name)

    def _event(self, event, data):
        if event == _EV_CONNECT:
            self._apps[data[0]] = 20
        elif event == _EV_DISCONNECT:
            self._apps.pop(data[0], None)
            self._advertise()                        # so the app can connect again
        elif event == _EV_WRITE:
            app, handle = data
            if handle == self._handle:
                received = self._ble.gatts_read(handle)   # in append mode, reading also empties it
                if len(self._queue) < _QUEUE_LIMIT:
                    self._queue.extend(chr(b) for b in received)
        elif event == _EV_MTU:
            app, mtu = data
            if app in self._apps:
                self._apps[app] = max(20, mtu - 3)

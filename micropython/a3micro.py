"""
a3micro.py

Description:
MicroPython port of the A3Micro Arduino library. It sends and receives the same
[1][ID][2][VALUE][3] messages as A3Micro.cpp, with the same classes and methods,
so the A3Micro app and web controller work with MicroPython boards unchanged.

Two transports, like the Arduino library:
  manager = A3MicroManager()         # the board's built-in BLE radio (ESP32, Pico W, ...)
  manager = A3MicroManager(uart)     # an HM-10 module on a machine.UART

In built-in BLE mode the board advertises the same service and characteristic
UUIDs as the UNO R4 WiFi, so the app treats it exactly like an UNO R4 WiFi.

Developed for TwinSparks Development (www.twinsparksdevelopment.com)
Modified and written by R.E Espino of twinsparks.dev
Licensed under the MIT License. See LICENSE for details.
"""

from micropython import const

try:
    import bluetooth
except ImportError:  # board without BLE: only HM-10 (UART) mode is available
    bluetooth = None

# UUIDs the A3Micro app uses to discover the board and exchange messages.
# These must match A3Micro.cpp.
SERVICE_UUID = "19B10000-E8F2-537E-4F6C-D104768A1214"
CHARACTERISTIC_UUID = "19B10001-E8F2-537E-4F6C-D104768A1214"

_IRQ_CENTRAL_CONNECT = const(1)
_IRQ_CENTRAL_DISCONNECT = const(2)
_IRQ_GATTS_WRITE = const(3)
_IRQ_MTU_EXCHANGED = const(21)

_FLAG_READ = const(0x0002)
_FLAG_WRITE_NO_RESPONSE = const(0x0004)
_FLAG_WRITE = const(0x0008)
_FLAG_NOTIFY = const(0x0010)

_FRAME_BUFFER_SIZE = const(100)  # Same limit as A3Micro.h
_RX_LIMIT = const(512)           # Bytes kept waiting between read() calls
_ADV_INTERVAL_US = const(100000)


def _text(data):
    try:
        return bytes(data).decode()
    except Exception:  # not valid UTF-8: keep the bytes as characters, like the Arduino String
        return "".join(chr(b) for b in data)


class A3MicroMessage:
    """A message with an ID and a value, e.g. id "b0" and value "1"."""

    def __init__(self, id="", value=""):
        self.id = id        # Message ID, typically a command identifier
        self.value = value  # Message value, e.g. "1" or "0" for LED control

    def has_id(self):
        return len(self.id) > 0

    def has_value(self):
        return len(self.value) > 0

    def to_string(self):
        return "id:" + self.id + " value:" + self.value

    # Same names as the Arduino library
    hasId = has_id
    hasValue = has_value
    toString = to_string

    def __str__(self):
        return self.to_string()

    @staticmethod
    def parse(buffer):
        """Parses a raw [1][ID][2][VALUE][3] frame into a message.
        The end delimiter (3) may be absent. Like the Arduino library, the value
        is lowercased and stripped of spaces."""
        msg = A3MicroMessage()
        size = len(buffer)
        if size > 0 and buffer[0] == 1:
            i = 1
            start = i
            while i < size and buffer[i] != 2:
                i += 1
            msg.id = _text(buffer[start:i])
            i += 1  # Skip the delimiter (2)
            start = i
            while i < size and buffer[i] != 3:
                i += 1
            if start < size:
                msg.value = _text(buffer[start:i])
        msg.value = msg.value.lower().replace(" ", "")
        return msg


class A3MicroManager:
    """Manages BLE message reading and writing. Sketches use the same
    begin() / is_connected() / read() / write() calls with either transport."""

    def __init__(self, uart=None, ble=None):
        self._uart = uart          # HM-10 serial port, or None in built-in BLE mode
        self._ble = None
        self._handle = None        # Characteristic value handle
        self._conns = {}           # conn_handle -> negotiated MTU
        self._name = "A3Micro"
        self._rx = bytearray()     # Bytes received by the BLE IRQ, not yet processed
        self._pending = bytearray()
        self._frame = bytearray()  # Bytes of the frame currently being received
        self._in_frame = False
        if uart is None:
            if bluetooth is None:
                raise OSError("This board has no Bluetooth; pass an HM-10 UART instead")
            self._ble = ble or bluetooth.BLE()

    def begin(self, device_name="A3Micro"):
        """Starts BLE advertising under the given name (built-in BLE mode).
        In HM-10 mode the module advertises on its own, so this returns True.
        Returns False if the radio fails to start."""
        if self._uart is not None:
            return True
        try:
            ble = self._ble
            ble.active(True)
            self._name = device_name
            try:
                ble.config(gap_name=device_name)
            except Exception:
                pass
            try:
                ble.config(mtu=256)  # Lets long frames arrive in one write
            except Exception:
                pass
            ble.irq(self._irq)
            flags = _FLAG_READ | _FLAG_WRITE | _FLAG_WRITE_NO_RESPONSE | _FLAG_NOTIFY
            service = (bluetooth.UUID(SERVICE_UUID), ((bluetooth.UUID(CHARACTERISTIC_UUID), flags),))
            ((self._handle,),) = ble.gatts_register_services((service,))
            # Append mode: bytes from back-to-back writes are kept until read
            ble.gatts_set_buffer(self._handle, 256, True)
            self._advertise()
            return True
        except Exception:
            return False

    def is_connected(self):
        """True while the A3Micro app is connected (built-in BLE mode).
        In HM-10 mode the connection state isn't visible, so this is always True."""
        if self._uart is not None:
            return True
        return len(self._conns) > 0

    def read(self):
        """Reads and parses a message from the app. Never blocks: returns an
        empty message when no complete frame has arrived yet. Bytes after a
        complete frame are kept for the next call."""
        if self._uart is not None:
            n = self._uart.any()
            if n:
                data = self._uart.read(n)
                if data:
                    self._pending.extend(data)
        else:
            data = self._rx  # The IRQ extends whichever buffer self._rx points to
            self._rx = bytearray()
            self._pending.extend(data)
        if len(self._pending) > _RX_LIMIT:
            self._pending = self._pending[-_RX_LIMIT:]

        buf = self._pending
        for i in range(len(buf)):
            b = buf[i]
            if b == 1:
                # Start delimiter: begin a fresh frame, discarding any partial one
                self._frame = bytearray(b"\x01")
                self._in_frame = True
                continue
            if not self._in_frame:
                continue  # Not inside a frame: ignore noise and HM-10 status text
            if b == 3:
                # End delimiter: frame complete. Keep the rest for the next call.
                self._in_frame = False
                msg = A3MicroMessage.parse(self._frame)
                self._frame = bytearray()
                self._pending = buf[i + 1:]
                return msg
            if len(self._frame) >= _FRAME_BUFFER_SIZE:
                # Oversized frame with no terminator: drop it and wait for the next start
                self._in_frame = False
                self._frame = bytearray()
                continue
            self._frame.append(b)
        self._pending = bytearray()
        return A3MicroMessage()

    def write(self, id, value):
        """Writes a message to the app using the protocol [1][ID][2][VALUE][3]."""
        frame = b"\x01" + str(id).encode() + b"\x02" + str(value).encode() + b"\x03"
        if self._uart is not None:
            self._uart.write(frame)
            return
        if self._handle is None:
            return
        ble = self._ble
        ble.gatts_write(self._handle, frame)
        for conn, mtu in list(self._conns.items()):
            size = max(20, mtu - 3)  # The app reassembles frames split across notifications
            try:
                for i in range(0, len(frame), size):
                    ble.gatts_notify(conn, self._handle, frame[i:i + size])
            except Exception:
                pass  # The app disconnected mid-write

    # Same name as the Arduino library
    isConnected = is_connected

    def _advertise(self):
        # Flags + 128-bit service UUID fill 21 of the 31 advertising bytes, so the
        # name goes in the scan response, like ArduinoBLE on the UNO R4 WiFi.
        uuid = bytes(bluetooth.UUID(SERVICE_UUID))
        adv = bytes((2, 0x01, 0x06)) + bytes((len(uuid) + 1, 0x07)) + uuid
        name = self._name.encode()[:29]
        resp = bytes((len(name) + 1, 0x09)) + name
        self._ble.gap_advertise(_ADV_INTERVAL_US, adv_data=adv, resp_data=resp)

    def _irq(self, event, data):
        if event == _IRQ_CENTRAL_CONNECT:
            conn_handle, _, _ = data
            self._conns[conn_handle] = 23
        elif event == _IRQ_CENTRAL_DISCONNECT:
            conn_handle, _, _ = data
            self._conns.pop(conn_handle, None)
            self._advertise()  # Let the app reconnect
        elif event == _IRQ_GATTS_WRITE:
            conn_handle, attr_handle = data
            if attr_handle == self._handle:
                data = self._ble.gatts_read(self._handle)  # Also empties the append buffer
                if len(self._rx) < _RX_LIMIT:
                    self._rx.extend(data)
                    # Each write carries one whole frame. A write that starts a frame without
                    # ending it is still taken as a complete frame, like A3Micro.cpp.
                    if data and data[0] == 1 and 3 not in data:
                        self._rx.append(3)
        elif event == _IRQ_MTU_EXCHANGED:
            conn_handle, mtu = data
            if conn_handle in self._conns:
                self._conns[conn_handle] = mtu

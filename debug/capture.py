"""Investigate FBV serial protocol."""

from argparse import ArgumentParser
import serial
import time
import datetime
from threading import Thread, Event
from collections import deque

from fbv import FBVMessage, FBVStateMachine, filter_msgs

FILTER = []

first_ping = False
data_dump = bytes()


class RXThread(Thread):
    """Receive bytes."""

    def __init__(self, port, timeout=0.1, save_to=None, **kwargs):
        """Initialize."""
        super().__init__(**kwargs)
        self._port = serial.Serial(port, 31250, timeout=timeout)
        self._stop_flag = Event()
        self._stop_flag.clear()
        self._fsm = FBVStateMachine(self._msg_cb)
        self._tx_queue = deque()
        self._running = False
        self._first_ping = False
        self._start_time = None
        self._pingback = False
        self._anyresponse = False
        self._saveto = save_to
        if save_to is not None:
            with open(self._saveto, "wb") as dataf:
                dataf.write(bytes([0xAA, 0xAA]))

    @property
    def ackping(self):
        """Get pingback state."""
        return self._pingback

    @ackping.setter
    def ackping(self, value):
        """Set pingback state."""
        self._pingback = value

    def _msg_cb(self, msg, **kwargs):
        """Receive message."""
        self._anyresponse = True
        if msg.command == FBVMessage.COMMAND_FBV_PING:
            if msg.params[0] == 0x00:
                # respond
                self._log_evt("RX: {}".format(msg))
                if self._pingback is True:
                    self._write(
                        0xF0, 0x07, 0x80, 0x00, 0x02, 0x00, 0x01, 0x01, 0x00
                    )
                self._first_ping = True
                return

        ret = filter_msgs(msg, remove_types=FILTER)
        if ret:
            self._log_evt("RX: {}".format(ret[0]))
            self._append_to_file(bytes(ret[0].bytes))

    def _log_time(self):
        """Get log time."""
        now = datetime.datetime.now()
        if self._start_time is None:
            raise RuntimeError("not running")
        log_time = now - self._start_time

        return "{:02d}.{:03d}".format(
            log_time.seconds, log_time.microseconds // 1000
        )

    def _log_evt(self, text):
        """Log."""
        print("({}) {}".format(self._log_time(), text))

    def _write(self, *data):
        """Internal write function."""
        msg = FBVMessage.from_bytes(*data)
        self._log_evt("TX: {}".format(msg))
        msg_bytes = msg.bytes
        msg_bytes[0] = 0xFF
        self._append_to_file(bytes(msg_bytes))
        self._port.write(bytes(data))

    def _append_to_file(self, data):
        """Append to dump file."""
        if self._saveto is None:
            return

        with open(self._saveto, "ab") as dataf:
            dataf.write(data)

    def wait_firstping(self):
        """Block until first ping is received."""
        while self._first_ping is False:
            pass

    @property
    def firstping(self):
        """Get whether first ping was received."""
        return self._first_ping

    @property
    def anyresponse(self):
        """Get whether any response has been received."""
        return self._anyresponse

    def send(self, *data):
        """Send message."""
        if self._stop_flag.isSet() or self._running is False:
            raise RuntimeError("thread is not running")
        self._tx_queue.appendleft(data)

    def stop(self):
        """Stop."""
        self._stop_flag.set()

    def run(self):
        """Run."""

        self._start_time = datetime.datetime.now()
        self._running = True
        while True:
            if self._stop_flag.isSet():
                break

            data = self._port.read(1)
            self._fsm.recv_bytes(data)

            empty = False
            while empty is False:
                try:
                    msg = self._tx_queue.pop()
                    # send
                    self._write(*msg)
                except IndexError:
                    empty = True


def write_bytes(port, buffer, *data):
    """Write bytes."""
    msg = FBVMessage.from_bytes(*data)
    print("TX: {}".format(msg))
    sent = bytes(data)
    port.write(sent)
    # modify first byte for bookkeeping
    _data = list(data)
    _data[0] = 0xFF
    buffer += bytes(_data)


if __name__ == "__main__":

    def msg_callback(msg, **kwargs):
        """Message callback."""
        global data_dump
        global first_ping

    parser = ArgumentParser()
    parser.add_argument("--port", default="/dev/ttyUSB0")
    parser.add_argument("--timeout", default=20, type=int)
    parser.add_argument("--dump")
    parser.add_argument("--live", action="store_true")

    args = parser.parse_args()
    if args.live is True:
        timeout = 0.01
    else:
        timeout = args.timeout

    rx = RXThread(args.port, timeout, save_to=args.dump)
    rx.start()

    # ANNOUNCE
    # rx.send(0xF0, 0x02, 0x90, 0x00)
    while rx.firstping is False:
        if rx.anyresponse is False:
            rx.send(0xF0, 0x02, 0x90, 0x00)
        time.sleep(0.3)

    print("DEBUG: got first ping; continuing")
    # first ping received, continue
    rx.send(0xF0, 0x02, 0x30, 0x08)
    rx.ackping = True

    # wait a few seconds
    time.sleep(10)

    # rx.ackping = True

    # REQUEST
    state = 1
    btn = 0
    cmd = 0
    press_wait_count = 0
    between_press_wait_count = 0
    print("DEBUG: start main loop")
    while True:
        try:
            if between_press_wait_count == 0:
                if press_wait_count < 3:
                    press_wait_count += 1
                else:
                    rx.send(0xF0, 0x03, cmd, btn, state)
                    state ^= 1
                    if state == 1:
                        btn += 1
                        between_press_wait_count = 50
                    if btn == 256:
                        cmd += 1
                        btn = 0
                    if cmd == 256:
                        break
                    press_wait_count = 0
            else:
                between_press_wait_count -= 1
            time.sleep(0.1)
        except KeyboardInterrupt:
            break

    # stop
    rx.stop()
    rx.join()

    # if args.dump is not None:
    #     with open(args.dump, "wb") as dumpdata:
    #         dumpdata.write(data_dump)

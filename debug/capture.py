"""Investigate FBV serial protocol."""

from argparse import ArgumentParser
import serial
import time
import datetime
from threading import Thread, Event
from collections import deque

from fbv import FBVMessage, FBVStateMachine, filter_msgs

FILTER = [FBVMessage.COMMAND_FBV_SET_LED]

first_ping = False
data_dump = bytes()


class RXThread(Thread):
    """Receive bytes."""

    def __init__(self, port, timeout=0.1, **kwargs):
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

    def _msg_cb(self, msg, **kwargs):
        """Receive message."""
        if msg.command == FBVMessage.COMMAND_FBV_PING:
            if msg.params[0] == 0x00:
                # respond
                self._log_evt("RX: {}".format(msg))
                self._write(
                    0xF0, 0x07, 0x80, 0x00, 0x02, 0x00, 0x01, 0x01, 0x00
                )
                self._first_ping = True
                return

        ret = filter_msgs(msg, remove_types=FILTER)
        if ret:
            self._log_evt("RX: {}".format(ret[0]))

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
        self._port.write(bytes(data))

    def wait_firstping(self):
        """Block until first ping is received."""
        while self._first_ping is False:
            pass

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
        timeout = 0.15
    else:
        timeout = args.timeout

    rx = RXThread(args.port, timeout)
    rx.start()

    # ANNOUNCE
    rx.send(0xF0, 0x02, 0x90, 0x00)
    rx.send(0xF0, 0x02, 0x30, 0x08)

    # wait for first ping
    rx.wait_firstping()

    # wait a few seconds
    print("DEBUG: got first ping; waiting")
    time.sleep(3)

    # REQUEST
    state = 1
    press_wait_count = 0
    between_press_wait_count = 0
    print("DEBUG: start main loop")
    while True:
        try:
            if between_press_wait_count == 0:
                if press_wait_count < 5:
                    press_wait_count += 1
                else:
                    rx.send(0xF0, 0x03, 0x81, 0x41, state)
                    state ^= 1
                    if state == 1:
                        between_press_wait_count = 50
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

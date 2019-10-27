"""Investigate FBV serial protocol."""

from argparse import ArgumentParser
import serial
import time

from fbv import FBVMessage, FBVStateMachine, filter_msgs

FILTER = [FBVMessage.COMMAND_FBV_SET_LED]


def msg_callback(msg):
    """Message callback."""
    ret = filter_msgs(msg, remove_types=FILTER)
    if ret:
        print(ret[0])


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("--port", default="/dev/ttyUSB0")
    parser.add_argument("--timeout", default=20, type=int)
    parser.add_argument("--dump")
    parser.add_argument("--live", action="store_true")

    args = parser.parse_args()
    if args.live is True:
        timeout = 0.1
    else:
        timeout = args.timeout
    port = serial.Serial(args.port, 31250, timeout=timeout)

    if args.live is True:
        fsm = FBVStateMachine(msg_callback)

    # ANNOUNCE
    port.write(bytes([0xF0, 0x02, 0x90, 0x00]))
    data = port.read(100)
    time.sleep(0.1)

    # REQUEST
    port.write(bytes([0xF0, 0x03, 0x80, 0x00, 0x01]))
    if args.live is None or args.live is False:
        data += port.read(10000)
    else:
        fsm.recv_bytes(data)
        while True:
            fsm.recv_bytes(port.read(100))
            time.sleep(0.1)

    if args.dump is not None:
        with open(args.dump, "wb") as dumpdata:
            dumpdata.write(data)

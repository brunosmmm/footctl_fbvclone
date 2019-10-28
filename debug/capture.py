"""Investigate FBV serial protocol."""

from argparse import ArgumentParser
import serial
import time
import datetime

from fbv import FBVMessage, FBVStateMachine, filter_msgs

FILTER = [FBVMessage.COMMAND_FBV_SET_LED]

waiting_ack = False
ack_received = False
data_dump = bytes()


def write_bytes(port, buffer, *data):
    """Write bytes."""
    msg = FBVMessage.from_bytes(data)
    print("SENT: {}".format(msg))
    sent = bytes(data)
    port.write(sent)
    # modify first byte for bookkeeping
    sent[0] = 0xFF
    buffer += sent


if __name__ == "__main__":

    def msg_callback(msg, **kwargs):
        """Message callback."""
        global waiting_ack
        global ack_received
        global data_dump
        if (
            waiting_ack is True
            and msg.command == FBVMessage.COMMAND_FBV_REQ_DONE
        ):
            ack_received = True
            waiting_ack = False
        elif msg.command == FBVMessage.COMMAND_ACK_PING:
            if msg.params[0] == 0x00:
                # respond
                write_bytes(
                    port,
                    data_dump,
                    0xF0,
                    0x07,
                    0x80,
                    0x00,
                    0x02,
                    0x00,
                    0x01,
                    0x01,
                    0x00,
                )

        ret = filter_msgs(msg, remove_types=FILTER)
        if ret:
            print(ret[0])

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
    port = serial.Serial(args.port, 31250, timeout=timeout)

    if args.live is True:
        fsm = FBVStateMachine(msg_callback)

    # ANNOUNCE
    print("DEBUG: sending first packet")
    waiting_ack = True
    ack_received = False
    write_bytes(port, data_dump, 0xF0, 0x02, 0x90, 0x00)
    # data = port.read(100)
    # data_dump += data
    # fsm.recv_bytes(data)

    # wait
    # while ack_received is False:
    #     data = port.read(100)
    #     data_dump += data
    #     fsm.recv_bytes(data)

    print("DEBUG: sending second packet")
    waiting_ack = True
    ack_received = False
    write_bytes(port, data_dump, 0xF0, 0x02, 0x30, 0x08)
    # data = port.read(100)
    # data_dump += data
    # fsm.recv_bytes(data)

    # wait
    while ack_received is False:
        data = port.read(100)
        data_dump += data
        fsm.recv_bytes(data)

    # print("DEBUG: sending MOD button press")
    # port.write(bytes([0xF0, 0x03, 0x81, 0x41, 0x01]))
    # time.sleep(0.1)

    # print("DEBUG: sending MOD button release")
    # port.write(bytes([0xF0, 0x03, 0x81, 0x41, 0x00]))

    # REQUEST
    state = 1
    pre_wait = 10
    last_time = datetime.datetime.now()
    updown_time = None
    print("DEBUG: start main loop")
    if args.live is None or args.live is False:
        data_dump += port.read(10000)
    else:
        while True:
            data = port.read(100)
            data_dump += data
            fsm.recv_bytes(data)
            # time.sleep(0.1)
            # now = datetime.datetime.now()
            # delta = now - last_time
            # if delta.total_seconds() * 1000 > 300:
            #     port.write(bytes([0xF0, 0x02, 0x90, 0x01]))
            #     last_time = now
            # if pre_wait > 0:
            #     pre_wait -= 1
            # else:
            #     if updown_time is None:
            #         updown_time = now
            #     else:
            #         delta = now - updown_time
            #     if updown_time == now or delta.total_seconds() * 1000 > 150:
            #         print("DEBUG: send btn val {}".format(state))
            #         port.write(bytes([0xF0, 0x03, 0x81, 0x00, state]))
            #         state ^= 1
            #         updown_time = now
            #         if state == 1:
            #             pre_wait = 10

    if args.dump is not None:
        with open(args.dump, "wb") as dumpdata:
            dumpdata.write(data_dump)

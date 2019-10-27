"""Inspect FBV capture data dump."""

from argparse import ArgumentParser

from fbv import FBVMessage, FBVStateMachine, filter_msgs

received_messages = []
FILTER = []


def msg_callback(msg):
    """Message callback."""
    received_messages.append(msg)


if __name__ == "__main__":
    parser = ArgumentParser()
    parser.add_argument("path")

    args = parser.parse_args()

    with open(args.path, "rb") as dump:
        data = dump.read()

    fsm = FBVStateMachine(msg_callback)

    # read
    fsm.recv_bytes(data)

    # filter
    filtered = filter_msgs(received_messages, remove_types=FILTER)

    # print
    for msg in filtered:
        print(msg)

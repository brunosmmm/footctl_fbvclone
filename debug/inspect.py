"""Inspect FBV capture data dump."""

from argparse import ArgumentParser

from fbv import FBVMessage, FBVStateMachine, filter_msgs

received_messages = []
FILTER = []


def msg_callback(msg, **kwargs):
    """Message callback."""
    host_sent = kwargs.get("sent_by_host", False)
    received_messages.append((msg, host_sent))


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
    msg_only = [msg[0] for msg in received_messages]
    filtered = filter_msgs(msg_only, remove_types=FILTER)
    updated_filtered = []
    for msg, host_sent in received_messages:
        if msg not in filtered:
            continue
        updated_filtered.append((msg, host_sent))

    # print
    for msg, host_sent in updated_filtered:
        if host_sent is True:
            prefix = "SENT: "
        else:
            prefix = ""
        print("{}{}".format(prefix, msg))

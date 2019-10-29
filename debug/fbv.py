"""FBV messages."""


class FBVInvalidMessageError(Exception):
    """Invalid message."""


class FBVMessage:
    """FBV message."""

    COMMAND_FBV_SET_LED = 0x04
    COMMAND_FBV_SET_CH = 0x0C
    COMMAND_FBV_SET_NAME = 0x10
    COMMAND_FBV_PING = 0x01
    COMMAND_FBV_SET_BNKD = 0x0A
    COMMAND_FBV_SET_BNKU = 0x0B
    COMMAND_FBV_SET_FLAT = 0x20
    COMMAND_FBV_TUNER = 0x08
    COMMAND_FBV_ACK = 0x80
    COMMAND_FBV_BTN = 0x81
    COMMAND_FBV_CTL = 0x82

    COMMANDS = (COMMAND_FBV_SET_LED, COMMAND_FBV_SET_CH, COMMAND_FBV_SET_NAME)
    COMMAND_NAMES = {
        COMMAND_FBV_SET_LED: "SET LED",
        COMMAND_FBV_SET_CH: "SET CH",
        COMMAND_FBV_SET_NAME: "SET NAME",
        COMMAND_FBV_PING: "PING",
        COMMAND_FBV_SET_BNKU: "SET BANK DIGIT 2",
        COMMAND_FBV_SET_BNKD: "SET BANK DIGIT 1",
        COMMAND_FBV_SET_FLAT: "TUNER FLAT",
        COMMAND_FBV_TUNER: "TUNER NOTE",
        COMMAND_FBV_ACK: "ACK",
        COMMAND_FBV_BTN: "BTN",
        COMMAND_FBV_CTL: "CTL",
    }

    # weird LED 0x0 appears when speed of mod is a note symbol
    # it is always set to off. when it is a value in milliseconds,
    # the pedal stops sending it to off
    FBV_LED_TAP = 0x61
    FBV_LED_MOD = 0x41
    FBV_LED_DLY = 0x51
    FBV_LED_CHA = 0x20
    FBV_LED_CHB = 0x30
    FBV_LED_CHC = 0x40
    FBV_LED_CHD = 0x50
    FBV_LED_SB2 = 0x22
    FBV_LED_SB1 = 0x12
    FBV_LED_SB3 = 0x32
    FBV_LED_AMP = 0x01
    FBV_LED_REV = 0x21
    FBV_LED_WAH = 0x13
    LED_NAMES = {
        FBV_LED_TAP: "TAP",
        FBV_LED_MOD: "MOD",
        FBV_LED_DLY: "DELAY",
        FBV_LED_CHA: "CH A",
        FBV_LED_CHB: "CH B",
        FBV_LED_CHC: "CH C",
        FBV_LED_CHD: "CH D",
        FBV_LED_SB1: "STOMP",
        FBV_LED_SB2: "EQ",
        FBV_LED_SB3: "GATE",
        FBV_LED_AMP: "AMP",
        FBV_LED_REV: "REVERB",
        FBV_LED_WAH: "WAH",
    }

    def __init__(self, cmd, params):
        """Initialize."""
        if not isinstance(params, (list, tuple, bytes)):
            raise TypeError("params must be iterable or bytes")

        if isinstance(params, (list, tuple)):
            self._params = bytes(params)
        else:
            self._params = params

        if not isinstance(cmd, int):
            raise TypeError("command must be an int")

        self._cmd = cmd

    @property
    def command(self):
        """Get command."""
        return self._cmd

    @property
    def params(self):
        """Get parameters."""
        return self._params

    @property
    def bytes(self):
        """Get message bytes."""
        msg_bytes = [0xF0, len(self._params) + 1, self._cmd]
        msg_bytes += self._params
        return msg_bytes

    @staticmethod
    def _parse_msg(*items):
        """Parse message."""
        if len(items) < 3:
            raise FBVInvalidMessageError(
                "invalid message length, expected at least 3 bytes"
            )
        if items[0] != 0xF0:
            raise FBVInvalidMessageError("invalid byte sequence")

        msg_len = int(items[1])
        if len(items) - 2 != msg_len:
            raise FBVInvalidMessageError(
                "invalid message length, expected more {} bytes, got {}".format(
                    len(items) - 2, msg_len
                )
            )

        # parse command values?

    @staticmethod
    def from_bytes(*items):
        """Create message object from byte string."""
        FBVMessage._parse_msg(*items)
        return FBVMessage(items[2], items[3:])

    def __repr__(self):
        """Get representation."""
        ret = "[{:02d}] ".format(3 + len(self.params))
        if self.command in self.COMMAND_NAMES:
            ret += self.COMMAND_NAMES[self.command]
        else:
            ret += "CMD({})".format(hex(self.command))

        # specific format

        if self.command == self.COMMAND_FBV_SET_FLAT:
            if self.params[0] == 0:
                ret += " OFF"
            else:
                ret += " ON"
        elif self.command == self.COMMAND_FBV_SET_LED:
            if self.params[0] in self.LED_NAMES:
                ret += " {} to".format(self.LED_NAMES[self.params[0]])
            else:
                ret += " {}(?) to".format(hex(self.params[0]))
            if self.params[1] == 0:
                ret += " OFF"
            else:
                ret += " ON"
        elif self.command == self.COMMAND_FBV_SET_NAME:
            try:
                ret += " to '{}'".format(self.params[2:].decode("ascii"))
            except UnicodeDecodeError:
                ret += " to ???"
        elif self.command == self.COMMAND_FBV_PING:
            ret += " {}".format(hex(self.params[0]))
        elif self.command == self.COMMAND_FBV_ACK:
            pass
        elif self.command in (
            self.COMMAND_FBV_SET_BNKD,
            self.COMMAND_FBV_SET_BNKU,
        ):
            if self.params[0] == 0:
                ret += " to MANUAL"
            else:
                ret += " to {}".format(chr(self.params[0]))
        elif self.command == self.COMMAND_FBV_SET_CH:
            ret += " {}".format(chr(self.params[0]))
        elif self.command == self.COMMAND_FBV_TUNER:
            ret += " {}".format(chr(self.params[3]))
        else:
            ret += " payload = {}".format([hex(b) for b in self.params])

        return ret


class FBVStateMachine:
    """Receiver state machine."""

    STATE_SYSEX_BYTE = 1
    STATE_LEN = 2
    STATE_CMD = 3
    STATE_PARAMS = 4

    def __init__(self, msg_callback=None):
        """Initialize."""
        self._state = self.STATE_SYSEX_BYTE
        self._cur_msg = []
        self._sent_by_host = False
        self._pending_bytes = 0
        self._msg_cb = msg_callback

    @property
    def state(self):
        """Get state."""
        return self._state

    def recv_byte(self, item):
        """Receive a byte."""
        if not isinstance(item, (int, bytes)):
            raise TypeError("item must be an int or bytes")
        if isinstance(item, bytes) and len(item) > 1:
            raise ValueError("only one byte is allowed at a time")
        if isinstance(item, bytes):
            item = ord(item)
        if self._state == self.STATE_SYSEX_BYTE:
            self._sent_by_host = False
            if item not in (0xF0, 0xFF):
                # ignore
                return
            if item == 0xFF:
                self._sent_by_host = True
            self._cur_msg.append(0xF0)
            self._state = self.STATE_LEN
        elif self._state == self.STATE_LEN:
            self._cur_msg.append(item)
            self._state = self.STATE_CMD
            self._pending_bytes = item
        elif self._state == self.STATE_CMD:
            self._cur_msg.append(item)
            self._state = self.STATE_PARAMS
            self._pending_bytes -= 1
        elif self._state == self.STATE_PARAMS:
            self._pending_bytes -= 1
            self._cur_msg.append(item)
            if self._pending_bytes == 0:
                # done
                self._state = self.STATE_SYSEX_BYTE
                if callable(self._msg_cb):
                    self._msg_cb(
                        FBVMessage.from_bytes(*self._cur_msg),
                        sent_by_host=self._sent_by_host,
                    )
                self._cur_msg = []
                self._pending_bytes = 0

    def recv_bytes(self, *items):
        """Receive bytes."""
        for item in items:
            if isinstance(item, bytes):
                for byte in item:
                    self.recv_byte(byte)
            elif isinstance(item, int):
                self.recv_byte(item)
            else:
                raise TypeError("items must be either bytes or integers")


def filter_msgs(msg_list, remove_types=None, keep_types=None):
    """Filter messages."""
    if not isinstance(msg_list, (tuple, list)):
        msg_list = [msg_list]

    if remove_types is None and keep_types is None:
        return msg_list

    if remove_types is not None and keep_types is not None:
        raise ValueError("use either remove_types or keep_types argument")

    if remove_types is not None and not isinstance(remove_types, (tuple, list)):
        remove_types = [remove_types]
    if keep_types is not None and not isinstance(keep_types, (tuple, list)):
        keep_types = [keep_types]

    if remove_types is not None:
        for item in remove_types:
            if not isinstance(item, int):
                raise TypeError("types must be integer")
    elif keep_types is not None:
        for item in remove_types:
            if not isinstance(item, int):
                raise TypeError("types must be integer")

    ret = []
    for msg in msg_list:
        if keep_types is not None:
            if msg.command not in keep_types:
                continue
        else:
            if msg.command in remove_types:
                continue

        ret.append(msg)

    return ret

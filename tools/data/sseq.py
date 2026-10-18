"""Disassemble Nitro sound sequences (SSEQ) into text, and assemble the text back, byte for byte.

A sequence's text is one command a line, in the order of the sequence's data, with labels where tracks start and where
jumps and calls go:

    track_00:
        alloc_tracks 0x0003
        open_track 1, track_01
        tempo 120
        program 3
        note Cs4, 100, 48
        rest 48
        jump track_00
    track_01:
        ...

A note's key is a name, C, Cs, D, Ds, E, F, Fs, G, Gs, A, As or B and an octave, where middle C (60) is C4 and a
negative octave is written m1, as Cm1 for key 0; a number works too. A command after `if` only runs when the last
comparison held, after `random` its last argument is a random number from the two given instead, and after `from_var`
it is the variable given. `.byte` keeps data that no track reaches. A number of wait ticks is written as the game has
it, and `rest 5 @2` gives a length the game stored in more bytes than it needs.
"""
import re
import struct

KEYS = ("C", "Cs", "D", "Ds", "E", "F", "Fs", "G", "Gs", "A", "As", "B")
# Commands by byte: name, and their arguments' kinds: u8, s8, s16, u16, var (a variable-length number), key, label
# (a 24-bit offset), track (u8)
COMMANDS = {
    0x80: ("rest", ("var",)), 0x81: ("program", ("var",)),
    0x93: ("open_track", ("track", "label")), 0x94: ("jump", ("label",)), 0x95: ("call", ("label",)),
    0xB0: ("set_var", ("u8", "s16")), 0xB1: ("add_var", ("u8", "s16")), 0xB2: ("sub_var", ("u8", "s16")),
    0xB3: ("mul_var", ("u8", "s16")), 0xB4: ("div_var", ("u8", "s16")), 0xB5: ("shift_var", ("u8", "s16")),
    0xB6: ("random_var", ("u8", "s16")), 0xB7: ("var_b7", ("u8", "s16")), 0xB8: ("cmp_eq", ("u8", "s16")),
    0xB9: ("cmp_ge", ("u8", "s16")), 0xBA: ("cmp_gt", ("u8", "s16")), 0xBB: ("cmp_le", ("u8", "s16")),
    0xBC: ("cmp_lt", ("u8", "s16")), 0xBD: ("cmp_ne", ("u8", "s16")),
    0xC0: ("pan", ("u8",)), 0xC1: ("volume", ("u8",)), 0xC2: ("master_volume", ("u8",)),
    0xC3: ("transpose", ("s8",)), 0xC4: ("pitch_bend", ("s8",)), 0xC5: ("bend_range", ("u8",)),
    0xC6: ("priority", ("u8",)), 0xC7: ("note_wait", ("u8",)), 0xC8: ("tie", ("u8",)),
    0xC9: ("portamento_key", ("key",)), 0xCA: ("mod_depth", ("u8",)), 0xCB: ("mod_speed", ("u8",)),
    0xCC: ("mod_type", ("u8",)), 0xCD: ("mod_range", ("u8",)), 0xCE: ("portamento", ("u8",)),
    0xCF: ("portamento_time", ("u8",)), 0xD0: ("attack", ("u8",)), 0xD1: ("decay", ("u8",)),
    0xD2: ("sustain", ("u8",)), 0xD3: ("release", ("u8",)), 0xD4: ("loop_start", ("u8",)),
    0xD5: ("expression", ("u8",)), 0xD6: ("print_var", ("u8",)),
    0xE0: ("mod_delay", ("s16",)), 0xE1: ("tempo", ("s16",)), 0xE3: ("sweep_pitch", ("s16",)),
    0xFC: ("loop_end", ()), 0xFD: ("return", ()), 0xFE: ("alloc_tracks", ("u16",)), 0xFF: ("end", ()),
}
NOTE = ("note", ("key", "u8", "var"))
BY_NAME = {name: (byte, kinds) for byte, (name, kinds) in COMMANDS.items()}
PREFIXES = {0xA0: "random", 0xA1: "from_var", 0xA2: "if"}
# Commands after which a track doesn't go on to the next one
STOPS = {0x94, 0xFD, 0xFF}
HEADER = 0x1C


class SequenceError(Exception):
    pass


def key_name(key: int) -> str:
    octave = key // 12 - 1
    return KEYS[key % 12] + (f"m{-octave}" if octave < 0 else str(octave))


def parse_key(text: str) -> int:
    match = re.fullmatch(r"([A-G]s?)(m?)(\d+)", text)
    if not match:
        return int(text, 0)
    octave = -int(match.group(3)) if match.group(2) else int(match.group(3))
    return (octave + 1) * 12 + KEYS.index(match.group(1))


def read_var(data: bytes, position: int) -> tuple[int, int, int]:
    """Returns a variable-length number, its length in bytes and the position after it."""
    value, start = 0, position
    while True:
        byte = data[position]
        position += 1
        value = value << 7 | byte & 0x7F
        if not byte & 0x80:
            return value, position - start, position


def var_bytes(value: int, length: int = 0) -> bytes:
    if not 0 <= value <= 0x0FFFFFFF:
        raise SequenceError(f"{value} is outside a wait's 0 to {0x0FFFFFFF}")
    if length > 4:
        raise SequenceError("a wait is at most 4 bytes")
    groups = [value & 0x7F]
    value >>= 7
    while value:
        groups.append(value & 0x7F)
        value >>= 7
    groups += [0] * (length - len(groups))
    groups.reverse()
    return bytes(group | 0x80 for group in groups[:-1]) + bytes(groups[-1:])


def decode(data: bytes, position: int) -> tuple[int, str | None, list, int]:
    """Decodes the command at a position: its byte, its prefix, its arguments as (kind, value, length) and the
    position after it."""
    prefix = None
    byte = data[position]
    if byte in PREFIXES:
        prefix = PREFIXES[byte]
        position += 1
        byte = data[position]
    position += 1
    if byte < 0x80:
        kinds = NOTE[1][1:]
    elif byte in COMMANDS:
        kinds = COMMANDS[byte][1]
    else:
        raise SequenceError(f"unknown command {byte:#04x}")
    if prefix in ("random", "from_var"):
        kinds = kinds[:-1]
    args = []
    for kind in kinds:
        if kind == "var":
            value, length, position = read_var(data, position)
            args.append((kind, value, length))
        elif kind in ("u8", "key", "track"):
            args.append((kind, data[position], 1))
            position += 1
        elif kind == "s8":
            args.append((kind, struct.unpack_from("<b", data, position)[0], 1))
            position += 1
        elif kind == "s16":
            args.append((kind, struct.unpack_from("<h", data, position)[0], 2))
            position += 2
        elif kind == "u16":
            args.append((kind, struct.unpack_from("<H", data, position)[0], 2))
            position += 2
        else:
            args.append((kind, int.from_bytes(data[position:position + 3], "little"), 3))
            position += 3
    if prefix == "random":
        args += [("s16", struct.unpack_from("<h", data, position)[0], 2),
                 ("s16", struct.unpack_from("<h", data, position + 2)[0], 2)]
        position += 4
    elif prefix == "from_var":
        args.append(("u8", data[position], 1))
        position += 1
    if position > len(data):
        raise SequenceError("a command runs past the end")
    return byte, prefix, args, position


def disassemble(sseq: bytes) -> str:
    """Returns the text of a sequence."""
    if sseq[:4] != b"SSEQ" or sseq[16:20] != b"DATA" or struct.unpack_from("<I", sseq, 24)[0] != HEADER:
        raise SequenceError("not a sound sequence")
    data = sseq[HEADER:]
    labels = {0: "track_00"}
    pending = [0]
    commands = {}
    while pending:
        position = pending.pop()
        while position < len(data) and position not in commands:
            try:
                byte, prefix, args, following = decode(data, position)
            except (SequenceError, IndexError, struct.error):
                break
            commands[position] = (byte, prefix, args, following)
            for kind, value, _ in args:
                if kind == "label":
                    if value not in labels:
                        track = next((v for k, v, _ in args if k == "track"), None)
                        name = f"track_{track:02d}" if track is not None else f"label_{value:04x}"
                        labels[value] = name if name not in labels.values() else f"{name}_{value:04x}"
                    pending.append(value)
            if byte in STOPS and prefix != "if":
                break
            position = following
    starts = sorted(commands)
    for start, following in zip(starts, starts[1:]):
        if commands[start][3] > following:
            raise SequenceError(f"commands overlap at {following:#x}")
    lines = []
    position = 0
    while position < len(data):
        if position in labels:
            lines.append(f"{labels[position]}:")
        if position in commands:
            byte, prefix, args, following = commands[position]
            name = "note" if byte < 0x80 else COMMANDS[byte][0]
            text = []
            if byte < 0x80:
                text.append(key_name(byte))
            for kind, value, length in args:
                if kind == "label":
                    text.append(labels[value])
                elif kind == "key":
                    text.append(key_name(value))
                elif kind == "u16":
                    text.append(f"{value:#06x}")
                elif kind == "var" and length != len(var_bytes(value)):
                    text.append(f"{value} @{length}")
                else:
                    text.append(str(value))
            line = f"    {prefix + ' ' if prefix else ''}{name}"
            lines.append(line + (" " + ", ".join(text) if text else ""))
            position = following
            continue
        end = position + 1
        while end < len(data) and end not in commands and end not in labels:
            end += 1
        chunk = data[position:end]
        for offset in range(0, len(chunk), 16):
            lines.append("    .byte " + ", ".join(f"{b:#04x}" for b in chunk[offset:offset + 16]))
        position = end
    return "\n".join(lines) + "\n"


def validate(statements: list, at: str):
    """Checks that every track, from track 0 and each open_track, ends with end or a jump, closes the loops it opens,
    and returns only from a call, and that open_track only starts the tracks alloc_tracks gives."""
    commands = [(number, what, args) for number, what, args in statements]
    labels = {args: index for index, (_, what, args) in enumerate(commands) if what == "label"}
    tracks = 0xFFFF
    for number, what, args in commands:
        if what != "label" and what[1] == "alloc_tracks":
            tracks = int(args[0], 0)
            break

    def walk(start: int, in_call: bool):
        pending = [(start, 0)]
        seen = set()
        while pending:
            index, depth = pending.pop()
            while True:
                if index >= len(commands):
                    raise SequenceError(f"{at}line {commands[-1][0]}: a track runs past the end without end or jump")
                if (index, depth) in seen:
                    break
                seen.add((index, depth))
                number, what, args = commands[index]
                if what == "label":
                    index += 1
                    continue
                prefix, name = what
                if name == ".byte":
                    raise SequenceError(f"{at}line {number}: a track runs into .byte data without end or jump")
                if name == "open_track" and not tracks >> int(args[0], 0) & 1:
                    raise SequenceError(f"{at}line {number}: track {args[0]} isn't one alloc_tracks gives")
                if name == "loop_start":
                    depth += 1
                elif name == "loop_end" and prefix != "if":
                    if not depth:
                        raise SequenceError(f"{at}line {number}: loop_end without a loop_start")
                    depth -= 1
                elif name == "return" and not in_call:
                    raise SequenceError(f"{at}line {number}: return outside a call")
                elif name == "jump":
                    pending.append((labels[args[0]], depth))
                    if prefix != "if":
                        break
                elif name in ("end", "return") and prefix != "if":
                    break
                index += 1

    starts = [0] + [labels[args[1]] for _, what, args in commands if what != "label" and what[1] == "open_track"]
    for start in starts:
        walk(start, False)
    for _, what, args in commands:
        if what != "label" and what[1] == "call":
            walk(labels[args[0]], True)


def assemble(text: str, where: str = "") -> bytes:
    """Returns the sequence of a text."""
    at = f"{where}: " if where else ""
    statements = []
    for number, raw in enumerate(text.splitlines(), 1):
        line = raw.split(";", 1)[0].strip()
        if not line:
            continue
        if line.endswith(":"):
            statements.append((number, "label", line[:-1]))
            continue
        words = line.split(None, 1)
        prefix = None
        if words[0] in ("random", "from_var", "if"):
            if len(words) < 2:
                raise SequenceError(f"{at}line {number}: {words[0]} needs a command after it")
            prefix = words[0]
            words = words[1].split(None, 1)
        args = [arg.strip() for arg in words[1].split(",")] if len(words) > 1 else []
        statements.append((number, (prefix, words[0]), args))

    def encode(number, prefix, name, args, labels):
        try:
            if name == ".byte":
                values = [int(arg, 0) for arg in args]
                if any(not 0 <= value <= 0xFF for value in values):
                    raise SequenceError(".byte takes values from 0 to 255")
                return bytes(values)
            if name == "note":
                if not args:
                    raise SequenceError("note takes a key, a velocity and a length")
                byte, kinds = parse_key(args[0]), NOTE[1][1:]
                args = args[1:]
                if not 0 <= byte < 0x80:
                    raise SequenceError(f"key {byte} is outside 0 to 127")
            elif name in BY_NAME:
                byte, kinds = BY_NAME[name]
            else:
                raise SequenceError(f"unknown command {name}")
            if prefix in ("random", "from_var"):
                kinds = kinds[:-1] + (("s16", "s16") if prefix == "random" else ("u8",))
            if len(args) != len(kinds):
                raise SequenceError(f"{name} takes {len(kinds)} arguments, not {len(args)}")
            out = bytearray()
            if prefix:
                out.append({value: key for key, value in PREFIXES.items()}[prefix])
            out.append(byte)
            for kind, arg in zip(kinds, args):
                if kind == "var":
                    value, _, length = arg.partition("@")
                    out += var_bytes(int(value, 0), int(length or 0))
                elif kind == "key":
                    out.append(parse_key(arg))
                elif kind in ("u8", "track"):
                    out += struct.pack("<B", int(arg, 0))
                elif kind == "s8":
                    out += struct.pack("<b", int(arg, 0))
                elif kind == "s16":
                    out += struct.pack("<h", int(arg, 0))
                elif kind == "u16":
                    out += struct.pack("<H", int(arg, 0))
                else:
                    if labels is not None and arg not in labels:
                        raise SequenceError(f"unknown label {arg}")
                    out += (labels or {}).get(arg, 0).to_bytes(3, "little")
            return bytes(out)
        except (ValueError, struct.error, SequenceError) as error:
            raise SequenceError(f"{at}line {number}: {error}") from None

    labels: dict[str, int] = {}
    position = 0
    for number, what, args in statements:
        if what == "label":
            if args in labels:
                raise SequenceError(f"{at}line {number}: label {args} is defined twice")
            labels[args] = position
        else:
            position += len(encode(number, what[0], what[1], args, None))
    data = b"".join(encode(number, what[0], what[1], args, labels)
                    for number, what, args in statements if what != "label")
    validate(statements, at)
    data += bytes(-(HEADER + len(data)) % 4)
    size = HEADER + len(data)
    return (struct.pack("<4sHHIHH", b"SSEQ", 0xFEFF, 0x0100, size, 0x10, 1)
            + struct.pack("<4sII", b"DATA", size - 0x10, HEADER) + data)

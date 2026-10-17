"""Read Nitro sound banks (SBNK) into JSON-ready instruments, and write them back, byte for byte.

A bank's instruments are a list in the order of their data, each with its program number, the number a sequence's
`program` picks it by, or a list of numbers where programs share it:

    {"program": 0, "type": "wave", "wave_archive": "WAVE_MUS_WB_X", "wave": 3, "base_key": "C4",
     "attack": 127, "decay": 127, "sustain": 127, "release": 115, "pan": 64}

A "wave" plays a wave of one of the bank's wave archives, a "psg" a square wave of a duty from 0 to 7 and a "noise"
noise, each from its base key, with its envelope and pan. A "drums" set has an instrument for each key from its
first_key, and a "split" an instrument for each region of keys up to its up_to. An "unused" entry keeps data that no
program uses, as hex.
"""
import struct

from sseq import key_name, parse_key

DATA = 0x3C
TYPES = {1: "wave", 2: "psg", 3: "noise", 16: "drums", 17: "split"}
SIMPLE = struct.Struct("<HHBBBBBB")
REGIONS = 8


class BankError(Exception):
    pass


def simple_json(kind: str, data: bytes, archives: list) -> dict:
    first, second, key, attack, decay, sustain, release, pan = SIMPLE.unpack(data)
    result: dict = {"type": kind}
    if kind == "wave":
        if second >= len(archives) or archives[second] is None:
            raise BankError(f"an instrument plays wave archive {second}, which the bank doesn't have")
        result.update({"wave_archive": archives[second], "wave": first})
    elif kind == "psg":
        if second:
            raise BankError("a PSG instrument has a second value")
        result["duty"] = first
    elif first or second:
        raise BankError("a noise instrument has values")
    result.update({"base_key": key_name(key), "attack": attack, "decay": decay, "sustain": sustain,
                   "release": release, "pan": pan})
    return result


def simple_bytes(instrument: dict, archives: list) -> tuple[int, bytes]:
    kind = instrument["type"]
    code = {name: number for number, name in TYPES.items()}[kind]
    if kind == "wave":
        if instrument["wave_archive"] not in archives:
            raise BankError(f"{instrument['wave_archive']} isn't one of the bank's wave archives")
        first, second = instrument["wave"], archives.index(instrument["wave_archive"])
    else:
        first, second = instrument.get("duty", 0), 0
    return code, SIMPLE.pack(first, second, parse_key(str(instrument["base_key"])), instrument["attack"],
                             instrument["decay"], instrument["sustain"], instrument["release"], instrument["pan"])


def instrument_size(data: bytes, kind: int, offset: int) -> int:
    if kind in (1, 2, 3):
        return SIMPLE.size
    if kind == 16:
        return 2 + 12 * (data[offset + 1] - data[offset] + 1)
    return REGIONS + 12 * next((index for index, key in enumerate(data[offset:offset + REGIONS]) if not key), REGIONS)


def read(sbnk: bytes, archives: list) -> list:
    """Returns a bank's instruments, with archives, the names of its four wave archives, naming the waves'."""
    if sbnk[:4] != b"SBNK" or sbnk[16:20] != b"DATA":
        raise BankError("not a sound bank")
    count = struct.unpack_from("<I", sbnk, 0x38)[0]
    records = [struct.unpack_from("<BHB", sbnk, DATA + 4 * index) for index in range(count)]
    programs: dict[int, list[int]] = {}
    kinds: dict[int, int] = {}
    for program, (kind, offset, reserved) in enumerate(records):
        if reserved or (kind == 0) != (offset == 0) or kind not in (0, *TYPES):
            raise BankError(f"program {program}'s record can't be written again")
        if kind:
            programs.setdefault(offset, []).append(program)
            kinds[offset] = kind
    instruments = []
    position = DATA + 4 * count
    for offset in sorted(programs):
        if offset > position:
            instruments.append({"unused": sbnk[position:offset].hex()})
        elif offset < position:
            raise BankError("instruments overlap")
        kind = kinds[offset]
        numbers = programs[offset]
        entry: dict = {"program": numbers[0] if len(numbers) == 1 else numbers}
        if kind in (1, 2, 3):
            entry.update(simple_json(TYPES[kind], sbnk[offset:offset + SIMPLE.size], archives))
        elif kind == 16:
            first, last = sbnk[offset], sbnk[offset + 1]
            keys = []
            for index in range(last - first + 1):
                start = offset + 2 + 12 * index
                sub, pad = sbnk[start], sbnk[start + 1]
                if sub not in (1, 2, 3) or pad:
                    raise BankError("a drum's instrument can't be written again")
                keys.append(simple_json(TYPES[sub], sbnk[start + 2:start + 12], archives))
            entry.update({"type": "drums", "first_key": key_name(first), "keys": keys})
        else:
            uppers = sbnk[offset:offset + REGIONS]
            used = instrument_size(sbnk, kind, offset) - REGIONS
            if any(uppers[used // 12:]):
                raise BankError("a split's regions go on after an empty one")
            regions = []
            for index in range(used // 12):
                start = offset + REGIONS + 12 * index
                sub, pad = sbnk[start], sbnk[start + 1]
                if sub not in (1, 2, 3) or pad:
                    raise BankError("a split's instrument can't be written again")
                regions.append({"up_to": key_name(uppers[index]),
                                **simple_json(TYPES[sub], sbnk[start + 2:start + 12], archives)})
            entry.update({"type": "split", "regions": regions})
        instruments.append(entry)
        position = offset + instrument_size(sbnk, kind, offset)
    tail = sbnk[position:]
    if tail != bytes(-position % 4):
        instruments.append({"unused": tail.hex()})
    if count != max((max(n) if isinstance(n := entry.get("program"), list) else n
                     for entry in instruments if "program" in entry), default=-1) + 1:
        instruments.append({"programs": count})
    return instruments


def write(instruments: list, archives: list) -> bytes:
    """Returns the bank of instruments."""
    programs = [entry["program"] for entry in instruments if "program" in entry]
    numbers = [number for program in programs for number in (program if isinstance(program, list) else [program])]
    if len(set(numbers)) != len(numbers):
        raise BankError("a program number is given twice")
    count = next((entry["programs"] for entry in instruments if "programs" in entry), max(numbers, default=-1) + 1)
    records = [(0, 0)] * count
    body = bytearray()
    position = DATA + 4 * count
    for entry in instruments:
        if "unused" in entry:
            body += bytes.fromhex(entry["unused"])
            continue
        if "programs" in entry:
            continue
        offset = position + len(body)
        kind = entry["type"]
        if kind in ("wave", "psg", "noise"):
            code, data = simple_bytes(entry, archives)
        elif kind == "drums":
            first = parse_key(str(entry["first_key"]))
            code, data = 16, bytes([first, first + len(entry["keys"]) - 1])
            for key in entry["keys"]:
                sub, sub_data = simple_bytes(key, archives)
                data += bytes([sub, 0]) + sub_data
        elif kind == "split":
            regions = entry["regions"]
            if len(regions) > REGIONS:
                raise BankError(f"a split has at most {REGIONS} regions")
            code = 17
            data = bytes(parse_key(str(region["up_to"])) for region in regions) + bytes(REGIONS - len(regions))
            for region in regions:
                sub, sub_data = simple_bytes(region, archives)
                data += bytes([sub, 0]) + sub_data
        else:
            raise BankError(f"unknown instrument type {kind}")
        program = entry["program"]
        for number in program if isinstance(program, list) else [program]:
            if number >= count:
                raise BankError(f"program {number} is past the bank's {count}")
            records[number] = (code, offset)
        body += data
    # The data ends aligned to 4 bytes, unless it ends with unused bytes, which keep the ROM's own end
    entries = [entry for entry in instruments if "programs" not in entry]
    if not entries or "unused" not in entries[-1]:
        body += bytes(-(position + len(body)) % 4)
    data = bytes(32) + struct.pack("<I", count) + b"".join(struct.pack("<BHB", kind, offset, 0)
                                                           for kind, offset in records) + body
    block = struct.pack("<4sI", b"DATA", 8 + len(data)) + data
    return struct.pack("<4sHHIHH", b"SBNK", 0xFEFF, 0x0100, 0x10 + len(block), 0x10, 1) + block

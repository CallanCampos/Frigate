#!/usr/bin/env python3
"""Compares a run of Frigate's program with a run of the melonDS oracle (port/tools/oracle/README.md).

A run is a directory: frame_NNNNNN.png for the frames it captured (both screens, top above bottom, 256x384),
frames.tsv with a row for each frame it ran (pbw-oracle and the port's --run-dir write one for every frame),
memory.txt with the memory it dumped, and run.txt. Frames pair up by --offset (oracle frame = port frame + offset, 0
by default) or by --align: counter (the oracle frame where the game's own frame count is the port frame's less the
count lag: 0, else what --lag declares from a port frame on; the port frames of --clock, where the game waits on a
clock, pair by their pictures as sequence does), first-change (the offset between the first frames that aren't blank,
which needs every frame up to them in both runs), image (the offset at which most of the port's frames show exactly an
oracle frame's screens) or sequence (each port frame, in order, with an oracle frame from the last pair's on that
shows the same pictures at the palettes' depth, whatever the timing).

    compare.py screens ORACLE PORT [--offset N | --align counter|first-change|image|sequence] [--clock LIST]
                       [--lag FRAME=N,...] [--frames LIST] [--known-oracle LIST] [--report DIR] [--depth 6|5]
                       [--tolerance FRACTION] [--pixel-tolerance STEPS] [--brief]
    compare.py memory ORACLE PORT [--offset N | --align ...] [--clock LIST] [--lag FRAME=N,...] [--frames LIST]
                      [--labels A,B,...]
    compare.py align ORACLE PORT
    compare.py hash PNG...

Each screen compares on its own, at the DS's colour depth (6 bits a channel, value >> 2), or with --depth 5 at its
palettes' (value >> 3), which leaves out differences in the output's low bit. A screen that shows 3D in the oracle's
frame (frames.tsv's top_3d and bottom_3d) passes when few of its pixels differ by more than a few steps after allowing
each pixel a one-pixel shift (rasterizers differ at polygon edges); a screen without 3D must be exact. Without a
picture on one side, frames.tsv's hashes decide a screen without 3D at 6 bits; any other screen is not compared.
With counter or sequence, every oracle frame between the frames that two consecutive port frames pair with must show
the screens of one of the two, or be one --known-oracle lists: a picture the port never shows fails.
--report writes, for each frame that isn't exact, an image of the oracle's screens, the port's and the differences
side by side; --brief lists, and draws, only the frames that don't match.

Exit statuses: 0 when every pair matches, 1 when something differs or couldn't be compared, 2 for bad arguments or
runs it can't read. Standard library only.
"""

from __future__ import annotations

import argparse
import collections
import hashlib
import os
import struct
import sys
import zlib
from dataclasses import dataclass, field

SCREEN_WIDTH = 256
SCREEN_HEIGHT = 192
SCREEN_PIXELS = SCREEN_WIDTH * SCREEN_HEIGHT
SCREEN_NAMES = ("top", "bottom")
FRAME_FILE = "frame_{:06d}.png"

# A screen with 3D passes when at most this fraction of its pixels differ by more than PIXEL_TOLERANCE steps of 6 bits
# from every pixel around the same place in the other picture
DEFAULT_TOLERANCE = 0.005
DEFAULT_PIXEL_TOLERANCE = 2


class RunError(Exception):
    pass


# PNG files


def read_png(path: str) -> tuple[int, int, bytes]:
    """Returns (width, height, rgb): 8-bit greyscale, RGB, palette, greyscale+alpha or RGBA, not interlaced; alpha is
    dropped"""
    with open(path, "rb") as f:
        data = f.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise RunError(f"{path}: not a PNG file")
    at = 8
    idat = []
    palette = b""
    width = height = depth = colour = interlace = None
    while at < len(data):
        (length,) = struct.unpack(">I", data[at : at + 4])
        kind = data[at + 4 : at + 8]
        body = data[at + 8 : at + 8 + length]
        at += 12 + length
        if kind == b"IHDR":
            width, height, depth, colour, _, _, interlace = struct.unpack(">IIBBBBB", body)
        elif kind == b"PLTE":
            palette = body
        elif kind == b"IDAT":
            idat.append(body)
        elif kind == b"IEND":
            break
    if width is None or depth != 8 or interlace != 0 or colour not in (0, 2, 3, 4, 6):
        raise RunError(f"{path}: only 8-bit, non-interlaced PNG files are read")
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[colour]
    stride = width * channels
    raw = zlib.decompress(b"".join(idat))
    if len(raw) != (stride + 1) * height:
        raise RunError(f"{path}: truncated image data")
    rows = []
    previous = bytearray(stride)
    for y in range(height):
        start = y * (stride + 1)
        kind = raw[start]
        row = bytearray(raw[start + 1 : start + 1 + stride])
        if kind == 1:
            for i in range(channels, stride):
                row[i] = (row[i] + row[i - channels]) & 0xFF
        elif kind == 2:
            for i in range(stride):
                row[i] = (row[i] + previous[i]) & 0xFF
        elif kind == 3:
            for i in range(stride):
                left = row[i - channels] if i >= channels else 0
                row[i] = (row[i] + ((left + previous[i]) >> 1)) & 0xFF
        elif kind == 4:
            for i in range(stride):
                a = row[i - channels] if i >= channels else 0
                b = previous[i]
                c = previous[i - channels] if i >= channels else 0
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                predictor = a if pa <= pb and pa <= pc else b if pb <= pc else c
                row[i] = (row[i] + predictor) & 0xFF
        elif kind != 0:
            raise RunError(f"{path}: unknown PNG filter {kind}")
        rows.append(bytes(row))
        previous = row
    pixels = b"".join(rows)
    if colour == 2:
        return width, height, pixels
    out = bytearray(width * height * 3)
    if colour == 6:
        out[0::3] = pixels[0::4]
        out[1::3] = pixels[1::4]
        out[2::3] = pixels[2::4]
    elif colour == 0 or colour == 4:
        grey = pixels[0::channels]
        out[0::3] = grey
        out[1::3] = grey
        out[2::3] = grey
    else:
        for i, index in enumerate(pixels):
            out[i * 3 : i * 3 + 3] = palette[index * 3 : index * 3 + 3]
    return width, height, bytes(out)


def write_png(path: str, width: int, height: int, rgb: bytes) -> None:
    def chunk(kind: bytes, body: bytes) -> bytes:
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)

    stride = width * 3
    raw = b"".join(b"\x00" + rgb[y * stride : (y + 1) * stride] for y in range(height))
    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 6)))
        f.write(chunk(b"IEND", b""))


def screen_hashes(rgb: bytes) -> tuple[str, str]:
    """The SHA-1 of each screen's RGB bytes, as pbw-oracle's frames.tsv gives them"""
    half = SCREEN_WIDTH * SCREEN_HEIGHT * 3
    return hashlib.sha1(rgb[:half]).hexdigest(), hashlib.sha1(rgb[half:]).hexdigest()


# A screen is blank when all but this many of its pixels have one colour, as pbw-oracle decides it
BLANK_SLACK = SCREEN_WIDTH


def is_blank(screen: bytes) -> bool:
    counts = collections.Counter(zip(screen[0::3], screen[1::3], screen[2::3]))
    return counts.most_common(1)[0][1] + BLANK_SLACK >= len(screen) // 3


# Runs


@dataclass
class Frame:
    frame: int
    vblank: int | None
    counter: int | None
    game_frame: int | None
    keyread: int | None
    top_3d: bool | None
    bottom_3d: bool | None
    top: str
    bottom: str
    top_blank: bool
    bottom_blank: bool


@dataclass
class Run:
    path: str
    frames: dict[int, Frame] = field(default_factory=dict)
    info: dict[str, str] = field(default_factory=dict)

    def png(self, frame: int) -> str | None:
        path = os.path.join(self.path, FRAME_FILE.format(frame))
        return path if os.path.exists(path) else None

    def captured(self) -> list[int]:
        out = []
        for name in os.listdir(self.path):
            if name.startswith("frame_") and name.endswith(".png"):
                try:
                    out.append(int(name[6:-4]))
                except ValueError:
                    pass
        return sorted(out)


def optional_int(text: str) -> int | None:
    return None if text == "-" else int(text)


# frames.tsv's columns: version 2, and version 1, whose one 3d column (engine A showed BG0 as the 3D layer) didn't say
# which screen, so that it reads as both
FRAMES_COLUMNS = ["frame", "vblank", "counter", "game_frame", "keyread", "top_3d", "bottom_3d", "top", "bottom",
                  "top_blank", "bottom_blank"]
FRAMES_COLUMNS_1 = ["frame", "vblank", "counter", "keyread", "3d", "top", "bottom", "top_blank", "bottom_blank"]


def optional_bool(text: str) -> bool | None:
    value = optional_int(text)
    return None if value is None else bool(value)


def load_run(path: str) -> Run:
    if not os.path.isdir(path):
        raise RunError(f"{path} isn't a run directory")
    run = Run(path)
    tsv = os.path.join(path, "frames.tsv")
    if os.path.exists(tsv):
        with open(tsv) as f:
            lines = [line.rstrip("\n") for line in f if not line.startswith("#")]
        header = lines[0].split("\t") if lines else []
        if header not in (FRAMES_COLUMNS, FRAMES_COLUMNS_1):
            raise RunError(f"{tsv}: unexpected columns {header}")
        for line in lines[1:]:
            row = line.split("\t")
            if len(row) != len(header):
                raise RunError(f"{tsv}: bad row {line!r}")
            cells = dict(zip(header, row))
            if header == FRAMES_COLUMNS_1:
                cells["game_frame"] = "-"
                cells["top_3d"] = cells["bottom_3d"] = cells["3d"]
            run.frames[int(cells["frame"])] = Frame(
                int(cells["frame"]),
                optional_int(cells["vblank"]),
                optional_int(cells["counter"]),
                optional_int(cells["game_frame"]),
                optional_int(cells["keyread"]),
                optional_bool(cells["top_3d"]),
                optional_bool(cells["bottom_3d"]),
                cells["top"],
                cells["bottom"],
                cells["top_blank"] == "1",
                cells["bottom_blank"] == "1",
            )
    info = os.path.join(path, "run.txt")
    if os.path.exists(info):
        with open(info) as f:
            for line in f:
                if ":" in line and not line.startswith("#"):
                    key, value = line.split(":", 1)
                    run.info[key.strip()] = value.strip()
    return run


def write_frames_tsv(path: str, frames: list[Frame]) -> None:
    def cell(value) -> str:
        if value is None:
            return "-"
        if isinstance(value, bool):
            return "1" if value else "0"
        return str(value)

    with open(path, "w") as f:
        f.write("#pbwframes 2\n")
        f.write("\t".join(FRAMES_COLUMNS) + "\n")
        for fr in frames:
            cells = [fr.frame, fr.vblank, fr.counter, fr.game_frame, fr.keyread, fr.top_3d, fr.bottom_3d, fr.top,
                     fr.bottom, fr.top_blank, fr.bottom_blank]
            f.write("\t".join(cell(c) for c in cells) + "\n")


def frame_from_png(frame: int, path: str) -> Frame:
    width, height, rgb = read_png(path)
    if (width, height) != (SCREEN_WIDTH, SCREEN_HEIGHT * 2):
        raise RunError(f"{path}: {width}x{height}, not both screens ({SCREEN_WIDTH}x{SCREEN_HEIGHT * 2})")
    top, bottom = screen_hashes(rgb)
    half = len(rgb) // 2
    return Frame(frame, None, None, None, None, None, None, top, bottom, is_blank(rgb[:half]), is_blank(rgb[half:]))


def load_memory(run: Run) -> dict[tuple[int, str], tuple[int, bytes]]:
    path = os.path.join(run.path, "memory.txt")
    out = {}
    if not os.path.exists(path):
        return out
    with open(path) as f:
        for number, line in enumerate(f, 1):
            if line.startswith("#") or not line.strip():
                continue
            cells = line.split()
            if len(cells) != 5:
                raise RunError(f"{path}:{number}: expected FRAME LABEL ADDRESS SIZE HEX")
            frame, label, address, size, data = cells
            data_bytes = bytes.fromhex(data)
            if len(data_bytes) != int(size):
                raise RunError(f"{path}:{number}: {size} bytes announced, {len(data_bytes)} given")
            out[(int(frame), label)] = (int(address, 16), data_bytes)
    return out


# Frame lists


def parse_frames(text: str) -> list[int] | None:
    """"1,30,60-90,100-200/10"; None for "all" """
    if text == "all":
        return None
    out = []
    for item in text.split(","):
        step = 1
        if "/" in item:
            item, step_text = item.split("/", 1)
            step = int(step_text)
        if "-" in item:
            first, last = (int(x) for x in item.split("-", 1))
        else:
            first = last = int(item)
        if first < 1 or last < first or step < 1:
            raise ValueError(item)
        out.extend(range(first, last + 1, step))
    return sorted(set(out))


# Alignment


def first_change(run: Run, complete: bool = False) -> int | None:
    """The first frame where either screen isn't blank. With complete, a RunError when the run lacks a frame before it:
    in a run of chosen frames (oracle.py port's), the first that isn't blank needn't be the first the program showed"""
    for number in sorted(run.frames):
        frame = run.frames[number]
        if not (frame.top_blank and frame.bottom_blank):
            missing = [f for f in range(1, number) if f not in run.frames]
            if complete and missing:
                raise RunError(f"{run.path}: its first frame that isn't blank is {number}, but it lacks frames "
                               f"{describe_ranges(missing)} before it, which could be the first; first-change needs "
                               "every frame up to it (a run of chosen frames, as oracle.py port writes, can't give it)")
            return number
    return None


def counter_offsets(oracle: Run, port: Run) -> dict[int, int]:
    """For each port frame with a counter, the offset to the oracle's first frame where the counter has its value,
    when the port frame is also the first with it"""
    first_oracle: dict[int, int] = {}
    for number in sorted(oracle.frames):
        value = oracle.frames[number].counter
        if value is not None and value not in first_oracle:
            first_oracle[value] = number
    out = {}
    seen = set()
    for number in sorted(port.frames):
        value = port.frames[number].counter
        if value is None or value in seen:
            continue
        seen.add(value)
        if value in first_oracle:
            out[number] = first_oracle[value] - number
    return out


def image_offsets(oracle: Run, port: Run, window: int | None = None) -> dict[int, list[int]]:
    """For each port frame, the offsets to the oracle frames (within the window, if one is given) that show exactly
    the same screens, nearest first"""
    by_hash: dict[tuple[str, str], list[int]] = {}
    for number, frame in oracle.frames.items():
        by_hash.setdefault((frame.top, frame.bottom), []).append(number)
    out = {}
    for number, frame in sorted(port.frames.items()):
        matches = [o - number for o in by_hash.get((frame.top, frame.bottom), [])
                   if window is None or abs(o - number) <= window]
        out[number] = sorted(matches, key=abs)
    return out


def picture_keys(run: Run, depth: int | None) -> dict[int, tuple]:
    """Each frame's screens as a key: the SHA-1 at the depth of the PNG when there is one (and a depth), else
    frames.tsv's hashes"""
    keys: dict[int, tuple] = {number: (f.top, f.bottom) for number, f in run.frames.items()}
    if depth is not None:
        for number in run.captured():
            rgb = read_png(run.png(number))[2]
            keys[number] = (hashlib.sha1(to_depth(rgb, depth)).hexdigest(),)
    return keys


# The depth sequence alignment matches pictures at: the palettes' colours, so that a difference in the low bit of the
# DS's 6-bit output (which compare then reports) doesn't break the pairing
SEQUENCE_DEPTH = 5


@dataclass
class Pairing:
    """How a port run's frames pair with an oracle run's: the oracle frame of each port frame (mapping), notes on some
    port frames, the port frames where the counts were lined up again by declaration after a clock (breaks: what the
    oracle showed between such a frame's pair and the one before isn't checked) and a line for each stretch of port
    frames saying how it was paired (timing)"""
    mapping: dict[int, int] = field(default_factory=dict)
    notes: dict[int, str] = field(default_factory=dict)
    breaks: set[int] = field(default_factory=set)
    timing: list[str] = field(default_factory=list)


def parse_lags(text: str | None) -> dict[int, int]:
    """"1109=0,3241=-1": from each port frame on, the port's count less the oracle's"""
    lags: dict[int, int] = {}
    for item in (text or "").split(","):
        if not item:
            continue
        frame, _, lag = item.partition("=")
        if not lag:
            raise ValueError(f"{item!r} isn't FRAME=LAG")
        lags[int(frame)] = int(lag)
    return lags


class Aligner:
    """What both alignments look frames up by: each frame's pictures as a key (at SEQUENCE_DEPTH when both runs have a
    PNG file of every frame they list, else by frames.tsv's hashes, which a run with pictures of some frames only has
    for the others), the oracle frames that show each, and the oracle frames with each count"""

    def __init__(self, oracle: Run, port: Run):
        self.oracle, self.port = oracle, port
        oracle_pictures, port_pictures = set(oracle.captured()), set(port.captured())
        pictured = oracle_pictures and set(oracle.frames) <= oracle_pictures and set(port.frames) <= port_pictures
        depth = SEQUENCE_DEPTH if pictured else None
        self.oracle_keys = picture_keys(oracle, depth)
        self.port_keys = picture_keys(port, depth)
        self.by_key: dict[tuple, list[int]] = {}
        for number in sorted(self.oracle_keys):
            self.by_key.setdefault(self.oracle_keys[number], []).append(number)
        self.with_counter: dict[int, list[int]] = {}
        for number in sorted(oracle.frames):
            value = oracle.frames[number].counter
            if value is not None:
                self.with_counter.setdefault(value, []).append(number)

    @staticmethod
    def counter(run: Run, number: int) -> int | None:
        frame = run.frames.get(number)
        return None if frame is None else frame.counter

    def expected(self, number: int, anchor: tuple[int, int]) -> int:
        """The oracle frame where a port frame is expected: as many game frames after the anchor's oracle frame as it
        is after the anchor's port frame (the first oracle frame where the count reaches the value) when both runs
        have the counter, else as many frames"""
        port_anchor, oracle_anchor = anchor
        port_count, port_anchor_count = self.counter(self.port, number), self.counter(self.port, port_anchor)
        oracle_anchor_count = self.counter(self.oracle, oracle_anchor)
        if None not in (port_count, port_anchor_count, oracle_anchor_count):
            wanted = oracle_anchor_count + port_count - port_anchor_count
            # The first frame where the count has the value, after the anchor and before the count starts again
            for candidate in self.with_counter.get(wanted, []):
                if candidate > oracle_anchor:
                    return candidate
        return oracle_anchor + number - port_anchor

    def anchor(self, number: int, last: tuple[int, int]) -> tuple[int, int]:
        """What a port frame is expected from: the last pair while the port's picture stays the same; once it changes,
        the end of the last pair's picture on each side, the last port frame and the last oracle frame that showed it.
        The DS spends more frames than the port on a still picture while it loads or waits on a clock, so what comes
        after a still starts where the still ends on each"""
        if self.port_keys.get(number) == self.port_keys[last[0]]:
            return last
        end = last[1]
        while self.oracle_keys.get(end + 1) == self.oracle_keys[last[1]]:
            end += 1
        return (last[0], end)

    def by_picture(self, number: int, last: tuple[int, int] | None) -> tuple[int, bool]:
        """A port frame's oracle frame by its pictures, and whether it was extrapolated: of the oracle frames from the
        last pair's on that show the same screens, the nearest to where the port frame is expected; with none (a
        screen with 3D, or a difference), where it is expected. Nearest, not first: a picture the oracle showed
        earlier (a flash of white, a black screen) isn't taken for a later one when frames in between had no match"""
        target = None if last is None else self.expected(number, self.anchor(number, last))
        later = [o for o in self.by_key.get(self.port_keys[number], []) if last is None or o >= last[1]]
        if later:
            return (later[0] if target is None else min(later, key=lambda o: (abs(o - target), o))), False
        return (number if target is None else target), True

    def by_counter(self, number: int, lag: int, last: tuple[int, int] | None) -> int | None:
        """A port frame's oracle frame by the count: the oracle's frames whose count is the port frame's less the lag,
        the run of them from the last pair's on, at the port frame's place among the port's frames with its count (a
        pass that takes several frames on both; the oracle's last when its pass took fewer)"""
        count = self.counter(self.port, number)
        frames = None if count is None else self.with_counter.get(count - lag)
        if not frames:
            return None
        place = 0
        while self.counter(self.port, number - 1 - place) == count:
            place += 1
        runs: list[list[int]] = []
        for frame in frames:
            if runs and frame == runs[-1][-1] + 1:
                runs[-1].append(frame)
            else:
                runs.append([frame])
        floor = 0 if last is None else last[1]
        run = next((r for r in runs if r[-1] >= floor), runs[-1])
        return run[min(place, len(run) - 1)]


def sequence_mapping(oracle: Run, port: Run) -> tuple[dict[int, int], dict[int, str]]:
    """Pairs each port frame, in order, by its pictures (Aligner.by_picture): with an oracle frame from the last
    pair's on that shows the same screens, the nearest to where it is expected; a port frame that none shows (a screen
    with 3D, or a difference) pairs with where it is expected, and is noted "extrapolated". It shows that the port
    shows the oracle's pictures in the same order, whatever the timing"""
    aligner = Aligner(oracle, port)
    mapping: dict[int, int] = {}
    notes: dict[int, str] = {}
    last = None
    for number in sorted(aligner.port_keys):
        choice, extrapolated = aligner.by_picture(number, last)
        mapping[number] = choice
        if extrapolated:
            notes[number] = "extrapolated"
        else:
            last = (number, choice)
    return mapping, notes


def counter_mapping(oracle: Run, port: Run, clock: set[int] | None = None,
                    lags: dict[int, int] | None = None) -> Pairing:
    """Pairs each port frame with the oracle frame where the game's count, less the count lag, is the port frame's
    (Aligner.by_counter): the timing the game keeps while it counts passes of its main loop, which a port that shows
    a picture late or early, or for more or fewer passes, fails. The lag is 0 from power-on, where both programs start
    the count, and what lags declares from each port frame on. Port frames of clock, where the game waits on a clock
    and the DS makes fewer, longer passes than the port, pair by their pictures instead (Aligner.by_picture); after
    them, the lag is the declared one (a press that ends the wait reaches both programs on the same pass, so the counts
    are the same again; the frame is a break), else the one where the frame is expected from the end of the last
    picture on each side"""
    clock = clock or set()
    lags = lags or {}
    aligner = Aligner(oracle, port)
    pairing = Pairing()
    stretches: list[list] = []  # [first port frame, last port frame, how]

    def stretch(number: int, how: str) -> None:
        if stretches and stretches[-1][2] == how and stretches[-1][1] == number - 1:
            stretches[-1][1] = number
        else:
            stretches.append([number, number, how])

    lag = 0
    how_lag = "0 from power-on"
    last = None
    in_clock = False
    for number in sorted(aligner.port_keys):
        if number in clock:
            choice, extrapolated = aligner.by_picture(number, last)
            pairing.mapping[number] = choice
            if extrapolated:
                pairing.notes[number] = "extrapolated"
            else:
                last = (number, choice)
            in_clock = True
            stretch(number, "by pictures (the game waits on a clock)")
            continue
        restart = False
        if number in lags:
            lag = lags[number]
            how_lag = f"{lag}, declared"
            restart = in_clock and last is not None
        elif in_clock and last is not None:
            expected = aligner.expected(number, aligner.anchor(number, last))
            count, oracle_count = aligner.counter(port, number), aligner.counter(oracle, expected)
            if None not in (count, oracle_count):
                lag = count - oracle_count
                how_lag = f"{lag}, from where the frame is expected after the end of the last picture on each"
        in_clock = False
        choice = aligner.by_counter(number, lag, last)
        if restart and choice is not None:
            pairing.breaks.add(number)
            skipped = describe_ranges(list(range(last[1] + 1, choice))) or "none"
            how_lag += (f" where the counts line up again after the clock (oracle frames since the last pair: "
                        f"{skipped}, not checked)")
        stretch(number, f"by the counter, lag {how_lag}")
        if choice is None:
            count = aligner.counter(port, number)
            pairing.notes[number] = "no counter" if count is None else f"no oracle frame with count {count - lag}"
            continue
        pairing.mapping[number] = choice
        last = (number, choice)
    for first, final, how in stretches:
        frames = f"port frame {first}" if first == final else f"port frames {first}-{final}"
        pairing.timing.append(f"{frames}: {how}")
    return pairing


def unshown_frames(oracle: Run, mapping: dict[int, int], within: tuple[int, int], breaks: set[int] | None = None,
                   allowed: set[int] | None = None) -> list[int]:
    """The oracle frames that show a picture no port frame shows: between the oracle frames of two consecutive port
    frames (both within the given range of port frames, the second not a break), those whose screens are neither
    pair's. Frames the DS repeats while a pass takes longer, or while a movie frame stays, show one of the two;
    a picture the port skips, or a frame the DS tore with a register written partway through it, shows neither.
    allowed are oracle frames known to have no counterpart"""
    keys = {number: (frame.top, frame.bottom) for number, frame in oracle.frames.items()}
    breaks = breaks or set()
    allowed = allowed or set()
    first, final = within
    out = set()
    for number in range(first, final):
        after = number + 1
        if number not in mapping or after not in mapping or after in breaks:
            continue
        a, b = mapping[number], mapping[after]
        for frame in range(a + 1, b):
            if frame in keys and frame not in allowed and keys[frame] not in (keys.get(a), keys.get(b)):
                out.add(frame)
    return sorted(out)


def frame_mapping(oracle: Run, port: Run, args):
    """A function from a port frame to the oracle frame it pairs with (None when it has none)"""
    if args.align == "sequence":
        mapping, _ = sequence_mapping(oracle, port)
        return mapping.get
    if args.align == "counter":
        return counter_pairing(oracle, port, args).mapping.get
    if args.align == "first-change":
        a, b = first_change(oracle, complete=True), first_change(port, complete=True)
        if a is None or b is None:
            raise RunError("a run has no frame whose screens aren't blank")
        return lambda frame: frame + a - b
    if args.align == "image":
        found = [o[0] for o in image_offsets(oracle, port).values() if o]
        if not found:
            raise RunError("no port frame shows the same screens as an oracle frame nearby")
        offset = max(sorted(set(found), key=abs), key=found.count)
        return lambda frame: frame + offset
    return lambda frame: frame + args.offset


def counter_pairing(oracle: Run, port: Run, args) -> Pairing:
    """counter_mapping with the command line's --clock and --lag"""
    if not any(frame.counter is not None for frame in port.frames.values()):
        raise RunError("no counter values to align on: both runs need frames.tsv's counter column")
    clock = set(parse_frames(args.clock) or []) if args.clock else set()
    return counter_mapping(oracle, port, clock, parse_lags(args.lag))


def resolve_pairs(oracle: Run, port: Run, args) -> tuple[list[tuple[int, int]], Pairing | None]:
    """(port frame, oracle frame) pairs to compare, and for the counter and sequence alignments, which pair every
    frame, the whole pairing"""
    port_frames = sorted(set(port.frames) | set(port.captured()))
    if args.frames:
        wanted = parse_frames(args.frames)
        if wanted is not None:
            port_frames = [f for f in port_frames if f in set(wanted)]
    pairing = None
    if args.align == "sequence":
        mapping, notes = sequence_mapping(oracle, port)
        pairing = Pairing(mapping, notes, set(), ["every port frame: by pictures, whatever the timing"])
    elif args.align == "counter":
        pairing = counter_pairing(oracle, port, args)
    mapping = pairing.mapping.get if pairing else frame_mapping(oracle, port, args)
    pairs = []
    for frame in port_frames:
        target = mapping(frame)
        if target is not None and target >= 1:
            pairs.append((frame, target))
    return pairs, pairing


# Screens


@dataclass
class ScreenCheck:
    """One screen of a pair: "exact", "pass" (3D within the tolerance), "fail", or "unknown" (not compared)"""
    three_d: bool
    verdict: str
    differing: int = 0
    beyond: int = 0
    reason: str = ""


@dataclass
class ScreenResult:
    port_frame: int
    oracle_frame: int
    three_d: tuple[bool, bool]
    screens: list[ScreenCheck]
    note: str = ""
    failure: str = ""  # why the pair couldn't be compared at all

    @property
    def verdict(self) -> str:
        if self.failure:
            return "fail"
        verdicts = [screen.verdict for screen in self.screens]
        return next((v for v in ("fail", "unknown", "pass") if v in verdicts), "exact")


DEPTH_TABLES = {depth: bytes(v >> (8 - depth) for v in range(256)) for depth in (5, 6)}


def to_depth(rgb: bytes, depth: int) -> bytes:
    """Each channel at the given depth: 6 bits is the DS's output, 5 bits its palettes' colours"""
    return rgb.translate(DEPTH_TABLES[depth])


def compare_screen(oracle: bytes, port: bytes, pixel_tolerance: int, depth: int = 6) -> tuple[list[int], list[int]]:
    """For one screen's RGB bytes: the pixels that differ at the depth, and those of them that differ by more than
    pixel_tolerance from every pixel of the other picture within one pixel of the same place"""
    a = to_depth(oracle, depth)
    b = to_depth(port, depth)
    differing = []
    beyond = []

    def close(image: bytes, x: int, y: int, ref: bytes) -> bool:
        for ny in (y - 1, y, y + 1):
            if ny < 0 or ny >= SCREEN_HEIGHT:
                continue
            for nx in (x - 1, x, x + 1):
                if nx < 0 or nx >= SCREEN_WIDTH:
                    continue
                n = (ny * SCREEN_WIDTH + nx) * 3
                if max(abs(image[n + c] - ref[c]) for c in range(3)) <= pixel_tolerance:
                    return True
        return False

    for index in range(SCREEN_PIXELS):
        at = index * 3
        if a[at : at + 3] == b[at : at + 3]:
            continue
        differing.append(index)
        x, y = index % SCREEN_WIDTH, index // SCREEN_WIDTH
        if not (close(a, x, y, b[at : at + 3]) and close(b, x, y, a[at : at + 3])):
            beyond.append(index)
    return differing, beyond


def write_report(path: str, oracle: bytes, port: bytes, differing: list[int], beyond: list[int]) -> None:
    """The oracle's screens, the port's, and the differences (pixel indexes over both screens, top first): the
    oracle's picture dimmed, with tolerated differences in yellow and the others in red"""
    width = SCREEN_WIDTH
    height = SCREEN_HEIGHT * 2
    diff = bytearray(v // 4 for v in oracle)
    for index in differing:
        diff[index * 3 : index * 3 + 3] = b"\xff\xd0\x00"
    for index in beyond:
        diff[index * 3 : index * 3 + 3] = b"\xff\x00\x00"
    out = bytearray()
    for y in range(height):
        row = slice(y * width * 3, (y + 1) * width * 3)
        out += oracle[row] + port[row] + diff[row]
    write_png(path, width * 3, height, bytes(out))


def check_by_hashes(oracle_hash: str, port_hash: str, three_d: bool, depth: int) -> ScreenCheck:
    """A screen without a picture on one side: frames.tsv's hashes are of the 8-bit output, the DS's 6 bits widened,
    so that they decide a screen at 6 bits; they can't say whether a difference is within the 3D tolerance or below
    the palettes' depth"""
    if oracle_hash == port_hash:
        return ScreenCheck(three_d, "exact")
    if depth == 6 and not three_d:
        return ScreenCheck(three_d, "fail", reason="its hashes differ")
    why = "a screen with 3D" if three_d else f"--depth {depth}"
    return ScreenCheck(three_d, "unknown", reason=f"no picture, and its hashes differ: {why} needs pictures")


def describe_screen(check: ScreenCheck) -> str:
    if check.verdict == "exact":
        return "exact"
    if check.reason:
        return check.reason
    if check.three_d:
        return f"{check.differing} pixels differ, {check.beyond} beyond tolerance"
    return f"{check.differing} pixels differ"


def command_screens(args) -> int:
    oracle = load_run(args.oracle)
    port = load_run(args.port)
    pairs, pairing = resolve_pairs(oracle, port, args)
    if not pairs:
        print("no frames to compare", file=sys.stderr)
        return 2
    notes = pairing.notes if pairing else {}
    unshown: list[int] = []
    if pairing:
        for line in pairing.timing:
            print(line)
        compared = [frame for frame, _ in pairs]
        known = set(parse_frames(args.known_oracle) or []) if args.known_oracle else set()
        unshown = unshown_frames(oracle, pairing.mapping, (compared[0], compared[-1]), pairing.breaks, known)
    if args.report:
        os.makedirs(args.report, exist_ok=True)
    screen_bytes = SCREEN_PIXELS * 3
    results = []
    for port_frame, oracle_frame in pairs:
        port_png = port.png(port_frame)
        oracle_png = oracle.png(oracle_frame)
        info = oracle.frames.get(oracle_frame)
        three_d = (bool(info and info.top_3d), bool(info and info.bottom_3d))
        note = notes.get(port_frame, "")
        if port_png is None or oracle_png is None:
            mine = port.frames.get(port_frame)
            if mine is None or info is None:
                results.append(ScreenResult(port_frame, oracle_frame, three_d, [], note, "no picture on one side"))
                continue
            checks = [check_by_hashes(info.top, mine.top, three_d[0], args.depth),
                      check_by_hashes(info.bottom, mine.bottom, three_d[1], args.depth)]
            results.append(ScreenResult(port_frame, oracle_frame, three_d, checks,
                                        ", ".join(n for n in (note, "hashes only") if n)))
            continue
        _, _, a = read_png(oracle_png)
        _, _, b = read_png(port_png)
        if len(a) != len(b):
            results.append(ScreenResult(port_frame, oracle_frame, three_d, [], note, "sizes differ"))
            continue
        checks = []
        all_differing: list[int] = []
        all_beyond: list[int] = []
        for screen in range(2):
            oracle_screen = a[screen * screen_bytes : (screen + 1) * screen_bytes]
            port_screen = b[screen * screen_bytes : (screen + 1) * screen_bytes]
            if to_depth(oracle_screen, args.depth) == to_depth(port_screen, args.depth):
                checks.append(ScreenCheck(three_d[screen], "exact"))
                continue
            differing, beyond = compare_screen(oracle_screen, port_screen, args.pixel_tolerance, args.depth)
            # The tolerance is a fraction of this screen's pixels, and only a screen with 3D has one
            passed = three_d[screen] and len(beyond) <= args.tolerance * SCREEN_PIXELS
            checks.append(ScreenCheck(three_d[screen], "pass" if passed else "fail", len(differing), len(beyond)))
            all_differing += [index + screen * SCREEN_PIXELS for index in differing]
            all_beyond += [index + screen * SCREEN_PIXELS for index in beyond]
        results.append(ScreenResult(port_frame, oracle_frame, three_d, checks, note))
        if args.report and all_differing and not (args.brief and results[-1].verdict == "pass"):
            write_report(os.path.join(args.report, f"diff_port{port_frame:06d}_oracle{oracle_frame:06d}.png"), a, b,
                         all_differing, all_beyond)

    labels = {"exact": "exact", "pass": "pass", "fail": "FAIL", "unknown": "not compared"}
    print("port   oracle  3D      result")
    for r in results:
        if args.brief and r.verdict in ("exact", "pass"):
            continue
        kind = {(False, False): "-", (True, False): "top", (False, True): "bottom", (True, True): "both"}[r.three_d]
        verdict = labels[r.verdict]
        if r.failure:
            verdict += f" ({r.failure})"
        elif r.verdict != "exact":
            verdict += " (" + "; ".join(f"{name} {describe_screen(check)}"
                                        for name, check in zip(SCREEN_NAMES, r.screens)) + ")"
        if r.note:
            verdict += f" [{r.note}]"
        print(f"{r.port_frame:6d} {r.oracle_frame:6d}  {kind:6s}  {verdict}")
    for first, final in frame_ranges(unshown):
        counts = sorted({oracle.frames[f].counter for f in range(first, final + 1)} - {None})
        where = f"oracle frame {first}" if first == final else f"oracle frames {first}-{final}"
        if counts:
            where += f" (count {counts[0]})" if len(counts) == 1 else f" (counts {counts[0]}-{counts[-1]})"
        print(f"FAIL: {where}: no port frame shows {'its' if first == final else 'their'} pictures")
    failed = sum(1 for r in results if r.verdict == "fail")
    unknown = sum(1 for r in results if r.verdict == "unknown")
    exact = sum(1 for r in results if r.verdict == "exact")
    print(f"{len(results) - failed - unknown} of {len(results)} frames match ({exact} exact, "
          f"{len(results) - failed - unknown - exact} with 3D within the tolerance)" +
          (f"; {unknown} not compared" if unknown else "") +
          (f"; {len(unshown)} oracle frames show pictures no port frame shows" if unshown else ""))
    return 1 if failed or unknown or unshown else 0


# Memory


def command_memory(args) -> int:
    oracle = load_run(args.oracle)
    port = load_run(args.port)
    oracle_memory = load_memory(oracle)
    port_memory = load_memory(port)
    labels = set(args.labels.split(",")) if args.labels else None
    wanted = parse_frames(args.frames) if args.frames else None
    mapping = frame_mapping(oracle, port, args)
    compared = differing = 0
    for (frame, label), (_, data) in sorted(port_memory.items()):
        if (labels is not None and label not in labels) or (wanted is not None and frame not in wanted):
            continue
        oracle_frame = mapping(frame)
        compared += 1
        if oracle_frame is None or (oracle_frame, label) not in oracle_memory:
            print(f"{label} at port frame {frame}: no oracle dump at frame {oracle_frame}")
            differing += 1
            continue
        _, expected = oracle_memory[(oracle_frame, label)]
        if expected == data:
            continue
        differing += 1
        if len(expected) != len(data):
            print(f"{label} at port frame {frame} / oracle {oracle_frame}: {len(data)} bytes, oracle {len(expected)}")
            continue
        changed = [i for i in range(len(data)) if data[i] != expected[i]]
        shown = ", ".join(f"+0x{i:x}: {expected[i]:02x}->{data[i]:02x}" for i in changed[:8])
        more = f" and {len(changed) - 8} more" if len(changed) > 8 else ""
        print(f"{label} at port frame {frame} / oracle {oracle_frame}: {len(changed)} bytes differ ({shown}{more})")
    if compared == 0:
        print("no dumps to compare", file=sys.stderr)
        return 2
    print(f"{compared} dumps compared, {differing} differ")
    return 1 if differing else 0


# Alignment report


def command_align(args) -> int:
    oracle = load_run(args.oracle)
    port = load_run(args.port)
    firsts = []
    for name, run in (("oracle", oracle), ("port", port)):
        try:
            firsts.append(first_change(run, complete=True))
        except RunError:
            print(f"the {name}'s run lacks frames before its first that isn't blank, {first_change(run)}: "
                  "first-change can't use it")
            firsts.append(None)
    a, b = firsts
    print(f"first frame that isn't blank: oracle {a}, port {b}" +
          (f" (offset {a - b})" if a is not None and b is not None else ""))
    counters = counter_offsets(oracle, port)
    if counters:
        values = sorted(set(counters.values()))
        print(f"counter offsets: {values[:10]}{' ...' if len(values) > 10 else ''}")
    else:
        print("counter offsets: none (the port's run has no counter column)")
    print("port frames and the oracle frames that show the same screens:")
    for frame, offsets in image_offsets(oracle, port).items():
        print(f"  port {frame}: {describe_ranges(sorted(frame + o for o in offsets)) or 'none'}")
    return 0


def frame_ranges(frames: list[int]) -> list[tuple[int, int]]:
    """[(249, 251), (300, 300)] for sorted frame numbers"""
    ranges: list[list[int]] = []
    for frame in frames:
        if ranges and frame == ranges[-1][1] + 1:
            ranges[-1][1] = frame
        else:
            ranges.append([frame, frame])
    return [(a, b) for a, b in ranges]


def describe_ranges(frames: list[int], limit: int = 6) -> str:
    """"249-251, 300, 400-420" for sorted frame numbers"""
    ranges = frame_ranges(frames)
    text = ", ".join(f"{a}" if a == b else f"{a}-{b}" for a, b in ranges[:limit])
    return text + (f" and {len(ranges) - limit} more" if len(ranges) > limit else "")


def command_hash(args) -> int:
    for path in args.png:
        _, _, rgb = read_png(path)
        top, bottom = screen_hashes(rgb)
        print(f"{top}\t{bottom}\t{path}")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)

    def pairing(p):
        p.add_argument("oracle", help="the oracle's run directory")
        p.add_argument("port", help="the port's run directory")
        p.add_argument("--offset", type=int, default=0, help="oracle frame = port frame + OFFSET (default 0)")
        p.add_argument("--align", choices=["counter", "first-change", "image", "sequence"],
                       help="find the pairing instead")
        p.add_argument("--clock", help="--align counter: port frames where the game waits on a clock, which pair by "
                       "their pictures (a frame list)")
        p.add_argument("--lag", help="--align counter: FRAME=LAG,...: from port frame FRAME on, the port's count is "
                       "the oracle's + LAG")
        p.add_argument("--frames", help="only these port frames: N, A-B, A-B/STEP, comma-separated")

    screens = commands.add_parser("screens", help="compare the screens")
    pairing(screens)
    screens.add_argument("--report", help="write difference images here")
    screens.add_argument("--brief", action="store_true",
                         help="list, and with --report draw, only the frames that don't match")
    screens.add_argument("--tolerance", type=float, default=DEFAULT_TOLERANCE,
                         help="screens with 3D: the fraction of the screen's pixels allowed beyond the pixel tolerance")
    screens.add_argument("--pixel-tolerance", type=int, default=DEFAULT_PIXEL_TOLERANCE,
                         help="screens with 3D: the steps a pixel may differ by")
    screens.add_argument("--depth", type=int, choices=[5, 6], default=6,
                         help="bits a channel to compare: 6, the DS's output (default), or 5, its palettes' colours")
    screens.add_argument("--known-oracle", help="oracle frames known to have no counterpart on the port (a frame "
                         "the DS tore), which the check for pictures no port frame shows leaves out")
    memory = commands.add_parser("memory", help="compare the memory dumps by label")
    pairing(memory)
    memory.add_argument("--labels", help="only these labels, comma-separated")
    align = commands.add_parser("align", help="report how the runs' frames line up")
    align.add_argument("oracle")
    align.add_argument("port")
    hashes = commands.add_parser("hash", help="print the screen hashes of PNG files as frames.tsv has them")
    hashes.add_argument("png", nargs="+")

    args = parser.parse_args(argv)
    if (getattr(args, "clock", None) or getattr(args, "lag", None)) and args.align != "counter":
        parser.error("--clock and --lag go with --align counter")
    try:
        if args.command == "screens":
            return command_screens(args)
        if args.command == "memory":
            return command_memory(args)
        if args.command == "align":
            return command_align(args)
        return command_hash(args)
    except (RunError, OSError, ValueError, zlib.error) as error:
        print(f"compare.py: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())

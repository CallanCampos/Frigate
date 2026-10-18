#!/usr/bin/env python3
"""The port's sound against the melonDS oracle (port/tools/oracle/README.md, "Sound").

Both programs write two things for it:

- A sound trace (--sound-trace, sndtrace.h): the ARM7 sound driver's state after each run of its thread. The driver
  moves its sequences on at its periodic runs only, every 2728 ticks of 64 cycles, and the two programs make those runs
  at different moments of a frame (the DS's driver starts during the boot that the port doesn't take, and the DS's
  passes take longer), so states pair by the sequence's own clock: a play of a player (from its start to its stop) on
  the oracle pairs with the same play on the port, and their records with the same count of periodic runs since the
  start. At each pair the player, its tracks (by their place in the player), their channels (in their lists' order)
  and the sound unit's registers of those channels are compared field by field. Frame by frame, where the game counts
  passes, what the ARM9 sees of the driver as each frame ends (the shared work's players and channels, the locks, the
  alarms, the tick counts, the registers of the channels the ARM9 drives itself) is compared on the frames that
  compare.py pairs.
- A WAV file of the mixed output (--audio-dump): 16-bit stereo, one sample each 1024 cycles. A segment of the port's
  frames is measured against the oracle's by envelope: each frame's RMS (560,190 cycles, 547.06 samples), lined up at
  the lag (oracle frame = port frame + lag) where the two correlate best, and the RMS, peak and samples at full scale
  of the whole segment. A segment whose sound doesn't depend on when the driver's runs fall (a stream, which the ARM9
  writes and the channels play at their timer) is compared sample by sample.

    sound.py state ORACLE_TRACE PORT_TRACE [--player N] [--before-count N] [--show N]
    sound.py frames ORACLE_TRACE PORT_TRACE ORACLE_RUN PORT_RUN [--clock LIST] [--lag FRAME=N,...] [--show N]
    sound.py audio ORACLE_WAV PORT_WAV --segments NAME:FIRST-LAST:LOW~HIGH[:exact],...
    sound.py dump TRACE INDEX

oracle.py's sound and sound-check commands run them on the comparison sequences, and write and check the reference
(compare_runs, write_reference, check_reference). Standard library only.
"""

from __future__ import annotations

import argparse
import array
import hashlib
import json
import math
import os
import struct
import sys
from dataclasses import dataclass, field

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import compare  # noqa: E402

# sndtrace.h
MAGIC = b"pbwsnd 1\n"
RECORD_SIZE = 0x14C0
OFF_CYCLE = 0x00
OFF_FRAME = 0x08
OFF_COUNTER = 0x0C
OFF_SEED = 0x10
OFF_PERIODS = 0x14
OFF_LOCK = 0x18
OFF_WEAK_LOCK = 0x1C
OFF_CNT = 0x20
OFF_WORK = 0x80
OFF_SHARED = 0x1200
OFF_START_MIX = 0x1480
WORK_ADDRESS = 0x03809FBC
CHANNELS = 16
CHANNEL_SIZE = 0x54
PLAYERS_OFF = 0x540
PLAYER_SIZE = 0x24
TRACKS_OFF = 0x780
TRACK_SIZE = 0x40
ALARMS_OFF = 0xF80
ALARM_SIZE = 0x40
PLAYER_COUNT = 16
TRACK_COUNT = 32
ALARM_COUNT = 8
TRACK_NONE = 0xFF
PLAYER_ACTIVE = 0x01
CNT_START = 0x80000000

# The DS's clocks: 2130 cycles a line, 263 lines a frame; a mixer sample each 1024 cycles
CYCLES_PER_FRAME = 2130 * 263
CYCLES_PER_SAMPLE = 1024
SAMPLES_PER_FRAME = CYCLES_PER_FRAME / CYCLES_PER_SAMPLE
FULL_SCALE = 0x7FFF

# How the outputs must agree, with the oracle and with the reference: each segment's envelope correlation at its lag,
# its RMS and peak within a fraction, its samples at full scale within a fraction or a few (the notes the two start a
# mixer sample apart clip on different samples); the frames of the envelope's blocks; the exact segments' probe
ENVELOPE_CORRELATION = 0.97
LEVEL_TOLERANCE = 0.02
CLIPPED_TOLERANCE = 0.10
CLIPPED_SLACK = 32
ENVELOPE_BLOCK = 4
PROBE_SAMPLES = 512
# A note start the oracle sees within this many mixer samples of the port's, against the most common offset, is the
# same note (one further is a start the oracle's sampler can't see: a channel stopped and started between two samples)
NOTE_WINDOW = 16


# Traces


@dataclass
class Record:
    data: bytes

    def u16(self, offset: int) -> int:
        return struct.unpack_from("<H", self.data, offset)[0]

    def u32(self, offset: int) -> int:
        return struct.unpack_from("<I", self.data, offset)[0]

    @property
    def cycle(self) -> int:
        return struct.unpack_from("<Q", self.data, OFF_CYCLE)[0]

    @property
    def frame(self) -> int:
        return self.u32(OFF_FRAME)

    @property
    def counter(self) -> int:
        return self.u32(OFF_COUNTER)

    @property
    def seed(self) -> int:
        return self.u32(OFF_SEED)

    @property
    def periods(self) -> int:
        return self.u32(OFF_PERIODS)

    def cnt(self, channel: int) -> int:
        return self.u32(OFF_CNT + channel * 4)

    def start_mix(self, channel: int) -> int:
        return self.u32(OFF_START_MIX + channel * 4)

    def work(self, offset: int, size: int) -> bytes:
        return self.data[OFF_WORK + offset:OFF_WORK + offset + size]

    def player_raw(self, number: int) -> bytes:
        return self.work(PLAYERS_OFF + number * PLAYER_SIZE, PLAYER_SIZE)

    def track_raw(self, number: int) -> bytes:
        return self.work(TRACKS_OFF + number * TRACK_SIZE, TRACK_SIZE)

    def channel_raw(self, number: int) -> bytes:
        return self.work(number * CHANNEL_SIZE, CHANNEL_SIZE)

    def alarm_raw(self, number: int) -> bytes:
        return self.work(ALARMS_OFF + number * ALARM_SIZE, ALARM_SIZE)

    def player_active(self, number: int) -> bool:
        return bool(self.data[OFF_WORK + PLAYERS_OFF + number * PLAYER_SIZE] & PLAYER_ACTIVE)

    @property
    def locks(self) -> tuple[int, int]:
        return self.u32(OFF_LOCK), self.u32(OFF_WEAK_LOCK)

    # The shared work
    @property
    def player_status(self) -> int:
        return self.u32(OFF_SHARED + 4)

    @property
    def channel_status(self) -> int:
        return self.u16(OFF_SHARED + 8)

    def tick_counter(self, player: int) -> int:
        return self.u32(OFF_SHARED + 0x20 + player * 0x24 + 0x20)

    def variables(self, player: int) -> tuple[int, ...]:
        return struct.unpack_from("<16h", self.data, OFF_SHARED + 0x20 + player * 0x24)

    @property
    def global_variables(self) -> tuple[int, ...]:
        return struct.unpack_from("<16h", self.data, OFF_SHARED + 0x260)


def load_trace(path: str) -> list[Record]:
    records = []
    with open(path, "rb") as f:
        if f.read(len(MAGIC)) != MAGIC:
            raise ValueError(f"{path} isn't a sound trace (pbwsnd 1)")
        while True:
            data = f.read(RECORD_SIZE)
            if len(data) < RECORD_SIZE:
                break
            records.append(Record(data))
    return records


# A player's state


PLAYER_FIELDS = (
    ("flags", 0x00, "B"),
    ("prio", 0x04, "B"),
    ("volume", 0x05, "B"),
    ("extFader", 0x06, "h"),
    ("tempo", 0x18, "H"),
    ("tempoRatio", 0x1A, "H"),
    ("tempoCounter", 0x1C, "H"),
)
TRACK_FIELDS = (
    ("flags", 0x00, "B"),
    ("panRange", 0x01, "B"),
    ("prgNo", 0x02, "H"),
    ("volume", 0x04, "B"),
    ("volume2", 0x05, "B"),
    ("pitchBend", 0x06, "b"),
    ("bendRange", 0x07, "B"),
    ("pan", 0x08, "b"),
    ("extPan", 0x09, "b"),
    ("extFader", 0x0A, "h"),
    ("extPitch", 0x0C, "h"),
    ("attack", 0x0E, "B"),
    ("decay", 0x0F, "B"),
    ("sustain", 0x10, "B"),
    ("release", 0x11, "B"),
    ("prio", 0x12, "B"),
    ("transpose", 0x13, "b"),
    ("portaKey", 0x14, "B"),
    ("portaTime", 0x15, "B"),
    ("sweepPitch", 0x16, "h"),
    ("mod", 0x18, "6s"),
    ("channelMask", 0x1E, "H"),
    ("wait", 0x20, "i"),
    ("depth", 0x3B, "B"),
)
CHANNEL_FIELDS = (
    ("type", 0x01, "B"),
    ("envStatus", 0x02, "B"),
    ("flags", 0x03, "B"),
    ("panRange", 0x04, "B"),
    ("originalKey", 0x05, "B"),
    ("userDecay2", 0x06, "h"),
    ("key", 0x08, "B"),
    ("velocity", 0x09, "B"),
    ("initPan", 0x0A, "b"),
    ("userPan", 0x0B, "b"),
    ("userDecay", 0x0C, "h"),
    ("userPitch", 0x0E, "h"),
    ("envDecay", 0x10, "i"),
    ("sweepCounter", 0x14, "i"),
    ("sweepLength", 0x18, "i"),
    ("attack", 0x1C, "B"),
    ("sustain", 0x1D, "B"),
    ("decay", 0x1E, "H"),
    ("release", 0x20, "H"),
    ("prio", 0x22, "B"),
    ("pan", 0x23, "B"),
    ("volume", 0x24, "H"),
    ("timer", 0x26, "H"),
    ("lfo", 0x28, "10s"),
    ("sweepPitch", 0x32, "h"),
    ("length", 0x34, "i"),
    ("wave", 0x38, "12s"),
)


def unpack_fields(raw: bytes, fields) -> dict:
    return {name: struct.unpack_from("<" + fmt, raw, offset)[0] for name, offset, fmt in fields}


def link_index(address: int, base: int, size: int, count: int) -> int | None:
    """The index of the work's element that a link points to: None for 0, -1 for anything else"""
    if address == 0:
        return None
    offset = address - WORK_ADDRESS - base
    if offset < 0 or offset % size != 0 or offset // size >= count:
        return -1
    return offset // size


def channel_list(record: Record, first: int) -> list[int]:
    """The channels of a list from its first link, in order"""
    out = []
    number = link_index(first, 0, CHANNEL_SIZE, CHANNELS)
    while number is not None and number >= 0 and number not in out:
        out.append(number)
        number = link_index(struct.unpack_from("<I", record.channel_raw(number), 0x50)[0], 0, CHANNEL_SIZE,
                            CHANNELS)
    return out


def channel_state(record: Record, number: int, positions: dict[int, int]) -> dict:
    raw = record.channel_raw(number)
    state = unpack_fields(raw, CHANNEL_FIELDS)
    state["number"] = number
    # A square wave's duty where a sound's samples are (the DS keeps them in one field); those are addresses
    if state["type"] == 1:
        state["duty"] = struct.unpack_from("<I", raw, 0x44)[0]
    callback, data = struct.unpack_from("<II", raw, 0x48)
    state["callback"] = callback != 0
    track = link_index(data, TRACKS_OFF, TRACK_SIZE, TRACK_COUNT)
    state["track"] = positions.get(track, track) if track is not None else None
    state["cnt"] = record.cnt(number)
    return state


def player_state(record: Record, number: int) -> dict:
    """The player's fields; its tracks by their place in the player (the driver's track numbers depend on what the
    other players took), each with its place in its sequence (cur - base) and call stack, and its channels in its
    list's order; the shared work's tick count, variables and status bit for the player"""
    raw = record.player_raw(number)
    state = {f"player.{name}": value for name, value in unpack_fields(raw, PLAYER_FIELDS).items()}
    state["player.tickCounter"] = record.tick_counter(number)
    state["player.variables"] = record.variables(number)
    state["player.status"] = bool(record.player_status >> number & 1)
    track_numbers = list(raw[0x08:0x18])
    positions = {t: j for j, t in enumerate(track_numbers) if t != TRACK_NONE}
    for j, t in enumerate(track_numbers):
        if t == TRACK_NONE or t >= TRACK_COUNT:
            continue
        traw = record.track_raw(t)
        prefix = f"track{j}."
        for name, value in unpack_fields(traw, TRACK_FIELDS).items():
            state[prefix + name] = value
        base, cur = struct.unpack_from("<II", traw, 0x24)
        stack = struct.unpack_from("<III", traw, 0x2C)
        depth = min(traw[0x3B], 3)
        state[prefix + "position"] = (cur - base) & 0xFFFFFFFF
        state[prefix + "callStack"] = tuple((s - base) & 0xFFFFFFFF for s in stack[:depth])
        state[prefix + "loopCount"] = tuple(traw[0x38:0x38 + depth])
        channels = channel_list(record, struct.unpack_from("<I", traw, 0x3C)[0])
        state[prefix + "channelCount"] = len(channels)
        for i, c in enumerate(channels):
            for name, value in channel_state(record, c, positions).items():
                state[f"{prefix}ch{i}.{name}"] = value
    return state


def state_digest(state: dict) -> str:
    return json.dumps(state, sort_keys=True, default=lambda v: v.hex() if isinstance(v, bytes) else str(v))


# Plays


@dataclass
class Play:
    player: int
    start: int  # the index of its first record
    end: int  # the index of its last
    # (periodic runs since the start, record index): the last record with each count
    runs: list[tuple[int, int]] = field(default_factory=list)


def find_plays(records: list[Record], player: int) -> list[Play]:
    """A player's plays: from a record where it is active after one where it wasn't, or where its tick count went back
    (a start over an active player), to its last active record. A run counts as many periodic runs as the record
    covers, so that the oracle's records of two runs together (the ARM7 busy between them) keep the count"""
    plays: list[Play] = []
    current: Play | None = None
    count = 0
    last_ticks = 0
    for index, record in enumerate(records):
        active = record.player_active(player)
        ticks = record.tick_counter(player)
        if active and (current is None or ticks < last_ticks):
            if current is not None:
                plays.append(current)
            current = Play(player, index, index)
            count = 0
        if not active:
            if current is not None:
                plays.append(current)
                current = None
            continue
        count += record.periods
        current.end = index
        if current.runs and current.runs[-1][0] == count:
            current.runs[-1] = (count, index)
        else:
            current.runs.append((count, index))
        last_ticks = ticks
    if current is not None:
        plays.append(current)
    return plays


def paired_runs(oracle: list[Record], port: list[Record], a: Play, b: Play,
                before_count: int | None) -> tuple[list[tuple[int, int, int]], int]:
    """The runs of two plays that pair, (count, oracle record, port record), while the game's count is below
    before_count on both; and how many pairs that left out"""
    port_runs = dict(b.runs)
    pairs = []
    left = 0
    for count, oracle_index in a.runs:
        port_index = port_runs.get(count)
        if port_index is None:
            continue
        if before_count is not None and max(oracle[oracle_index].counter, port[port_index].counter) >= before_count:
            left += 1
            continue
        pairs.append((count, oracle_index, port_index))
    return pairs, left


@dataclass
class PlayResult:
    player: int
    play: int
    oracle_frames: tuple[int, int]
    port_frames: tuple[int, int]
    pairs: int = 0
    equal: int = 0
    fields: int = 0
    fields_equal: int = 0
    # field name -> [runs, first run, oracle value, port value]
    differences: dict[str, list] = field(default_factory=dict)
    first_unequal: int | None = None
    left: int = 0
    only: str = ""  # "oracle" or "port" for a play the other doesn't have
    # The play's notes: the mixer sample that took each one's start on the port, and the oracle's for the same note
    starts: dict[int, int] = field(default_factory=dict)


def note_starts(oracle_record: Record, port_record: Record, player: int, starts: dict[int, int]) -> None:
    """The start samples of the notes on the player's channels, paired by the track's place and the channel's"""
    def channels(record: Record) -> dict[tuple[int, int], int]:
        out = {}
        for j, t in enumerate(record.player_raw(player)[0x08:0x18]):
            if t >= TRACK_COUNT:
                continue
            first = struct.unpack_from("<I", record.track_raw(t), 0x3C)[0]
            for i, c in enumerate(channel_list(record, first)):
                out[(j, i)] = record.start_mix(c)
        return out

    left = channels(oracle_record)
    for key, port_start in channels(port_record).items():
        if key in left and port_start not in starts:
            starts[port_start] = left[key]


def compare_plays(oracle: list[Record], port: list[Record], before_count: int | None = None,
                  players=None) -> list[PlayResult]:
    results = []
    for player in players if players is not None else range(PLAYER_COUNT):
        a_plays = find_plays(oracle, player)
        b_plays = find_plays(port, player)
        for index in range(max(len(a_plays), len(b_plays))):
            if index >= len(a_plays) or index >= len(b_plays):
                side, records, play = (("oracle", oracle, a_plays[index]) if index < len(a_plays)
                                       else ("port", port, b_plays[index]))
                if before_count is not None and records[play.start].counter >= before_count:
                    continue
                result = PlayResult(player, index, (0, 0), (0, 0), only=side)
                frames = (records[play.start].frame, records[play.end].frame)
                result.oracle_frames, result.port_frames = (frames, (0, 0)) if side == "oracle" else ((0, 0), frames)
                results.append(result)
                continue
            a, b = a_plays[index], b_plays[index]
            result = PlayResult(player, index, (oracle[a.start].frame, oracle[a.end].frame),
                                (port[b.start].frame, port[b.end].frame))
            pairs, result.left = paired_runs(oracle, port, a, b, before_count)
            for count, oracle_index, port_index in pairs:
                note_starts(oracle[oracle_index], port[port_index], player, result.starts)
                left = player_state(oracle[oracle_index], player)
                right = player_state(port[port_index], player)
                result.pairs += 1
                same = True
                for name in sorted(set(left) | set(right)):
                    result.fields += 1
                    lv = left.get(name, "-")
                    rv = right.get(name, "-")
                    if lv == rv:
                        result.fields_equal += 1
                        continue
                    same = False
                    entry = result.differences.setdefault(name, [0, count, lv, rv])
                    entry[0] += 1
                if same:
                    result.equal += 1
                elif result.first_unequal is None:
                    result.first_unequal = count
            results.append(result)
    return results


def describe_starts(starts: dict[int, int]) -> str:
    """How the oracle's note starts sit against the port's: from the earliest of the most common group, how many come
    that many mixer samples later"""
    offsets = [o - p for p, o in starts.items()]
    if not offsets:
        return "none"
    common = max(set(offsets), key=offsets.count)
    near = [d for d in offsets if abs(d - common) <= NOTE_WINDOW]
    base = min(near)
    spread: dict[int, int] = {}
    for d in near:
        spread[d - base] = spread.get(d - base, 0) + 1
    shown = ", ".join(f"{n} at +{d}" for d, n in sorted(spread.items()))
    unseen = len(offsets) - len(near)
    return (f"{len(near)} paired, by the mixer sample that took their start against the earliest: {shown}"
            + (f"; {unseen} restarts the oracle's sampler can't see" if unseen else ""))


def describe_play(r: PlayResult, show: int) -> list[str]:
    if r.only:
        frames = r.oracle_frames if r.only == "oracle" else r.port_frames
        return [f"player {r.player} play {r.play}: only on the {r.only} (frames {frames[0]}-{frames[1]})"]
    lines = [f"player {r.player} play {r.play}: oracle frames {r.oracle_frames[0]}-{r.oracle_frames[1]}, port "
             f"{r.port_frames[0]}-{r.port_frames[1]}: {r.pairs} runs paired, {r.equal} equal; fields "
             f"{r.fields_equal} of {r.fields} equal" + (f", the first unequal at run {r.first_unequal}"
                                                       if r.first_unequal is not None else "")
             + (f"; {r.left} runs after the count limit left out" if r.left else "")]
    if r.starts:
        lines.append(f"    notes: {describe_starts(r.starts)}")
    shown = sorted(r.differences.items(), key=lambda item: (item[1][1], item[0]))
    for name, (count, first, lv, rv) in shown[:show]:
        lines.append(f"    {name}: {count} runs, the first at run {first}: oracle {lv}, port {rv}")
    if len(shown) > show:
        lines.append(f"    ... {len(shown) - show} more fields differ")
    return lines


# Frame by frame


def status_of(record: Record) -> dict:
    """What the ARM9 sees of the driver at a glance: the shared work's players and channels, the locks, the alarms
    that are on with their setting, the active players' tick counts, the global variables, and the registers of the
    channels the ARM9 locked (a stream's, a cry's), which it drives itself"""
    alarms = []
    for a in range(ALARM_COUNT):
        raw = record.alarm_raw(a)
        if raw[0]:
            alarms.append((a, raw[1], struct.unpack_from("<QQ", raw, 4)))
    lock, weak = record.locks
    return {
        "players": record.player_status,
        "channels": record.channel_status,
        "locks": (lock, weak),
        "alarms": tuple(alarms),
        "ticks": tuple((p, record.tick_counter(p)) for p in range(PLAYER_COUNT) if record.player_status >> p & 1),
        "globals": record.global_variables,
        "locked": tuple((c, record.cnt(c)) for c in range(CHANNELS) if (lock | weak) >> c & 1),
    }


def frame_states(records: list[Record]) -> dict[int, int]:
    """Each frame's last record, which is the driver as the frame's VBlank comes; a frame with no run of its own has
    the one before's"""
    ends: dict[int, int] = {}
    for index, record in enumerate(records):
        ends[record.frame] = index
    out = {}
    last = None
    for frame in range(1, max(ends, default=0) + 1):
        last = ends.get(frame, last)
        if last is not None:
            out[frame] = last
    return out


@dataclass
class FrameResult:
    compared: int = 0
    equal: int = 0
    run_apart: int = 0
    known: int = 0
    unequal: list = field(default_factory=list)  # (port frame, oracle frame, {field: (oracle, port)})
    # The pairs equal at their frame ends: (port frame, oracle frame, oracle record)
    pairs: list = field(default_factory=list)


def frame_pairs(oracle_run: compare.Run, port_run: compare.Run, clock: str | None,
                lags: str | None) -> list[tuple[int, int]]:
    """The frames where the game counts passes, paired as compare.py --align counter pairs them. The movies' frames
    pair by their pictures, which the DS shows late against its clock, and the sound isn't timed by them: they aren't
    compared here (their plays are, by the sequences' own clock)"""
    clock_frames = set(compare.parse_frames(clock) or []) if clock else set()
    pairing = compare.counter_mapping(oracle_run, port_run, clock_frames, compare.parse_lags(lags))
    return sorted((p, o) for p, o in pairing.mapping.items() if p not in clock_frames)


def compare_frames(oracle: list[Record], port: list[Record], pairs: list[tuple[int, int]],
                   known: set[int] | None = None) -> FrameResult:
    """The status at each pair's frame ends: equal; equal a run apart, where the run that changed it falls just
    before the VBlank on one program and just after on the other; or not. known are port frames left out"""
    result = FrameResult()
    known = known or set()
    oracle_states = frame_states(oracle)
    port_states = frame_states(port)
    for port_frame, oracle_frame in pairs:
        port_index = port_states.get(port_frame)
        oracle_index = oracle_states.get(oracle_frame)
        if port_index is None or oracle_index is None:
            continue
        if port_frame in known:
            result.known += 1
            continue
        result.compared += 1
        a = status_of(port[port_index])
        b = status_of(oracle[oracle_index])
        if a == b:
            result.equal += 1
            result.pairs.append((port_frame, oracle_frame, oracle_index))
            continue
        near_oracle = [status_of(oracle[i]) for i in (oracle_index - 1, oracle_index + 1) if 0 <= i < len(oracle)]
        near_port = [status_of(port[i]) for i in (port_index - 1, port_index + 1) if 0 <= i < len(port)]
        if a in near_oracle or b in near_port:
            result.run_apart += 1
            continue
        result.unequal.append((port_frame, oracle_frame, {n: (b[n], a[n]) for n in a if a[n] != b[n]}))
    return result


def status_hash(record: Record) -> str:
    return hashlib.sha1(json.dumps(status_of(record), sort_keys=True).encode()).hexdigest()[:16]


# Audio


def load_wav(path: str) -> tuple[int, array.array]:
    """A 16-bit stereo WAV file's rate and samples, left and right interleaved"""
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError(f"{path} isn't a WAV file")
    at = 12
    rate = channels = bits = None
    samples = None
    while at + 8 <= len(data):
        tag = data[at:at + 4]
        size = struct.unpack_from("<I", data, at + 4)[0]
        body = data[at + 8:at + 8 + size]
        if tag == b"fmt ":
            _, channels, rate, _, _, bits = struct.unpack_from("<HHIIHH", body, 0)
        elif tag == b"data":
            samples = array.array("h")
            samples.frombytes(body[:len(body) // 4 * 4])
            if sys.byteorder != "little":
                samples.byteswap()
        at += 8 + size + (size & 1)
    if channels != 2 or bits != 16 or samples is None:
        raise ValueError(f"{path} isn't 16-bit stereo")
    return rate, samples


def frame_samples(frame: int) -> tuple[int, int]:
    """Frame N's stereo samples in a dump, from 1: those the mixer made during it"""
    return int(round((frame - 1) * SAMPLES_PER_FRAME)), int(round(frame * SAMPLES_PER_FRAME))


@dataclass
class Level:
    rms: float = 0.0
    peak: int = 0
    clipped: int = 0  # samples (left or right) at full scale

    def as_text(self) -> str:
        return f"{self.rms:.1f},{self.peak},{self.clipped}"

    @staticmethod
    def from_text(text: str) -> "Level":
        rms, peak, clipped = text.split(",")
        return Level(float(rms), int(peak), int(clipped))


def level(samples: array.array, first: int, last: int) -> Level:
    """The RMS, peak and samples at full scale of the frames first to last"""
    start, _ = frame_samples(first)
    _, end = frame_samples(last)
    window = samples[start * 2:end * 2]
    if not window:
        return Level()
    total = sum(v * v for v in window)
    return Level(math.sqrt(total / len(window)), max(abs(v) for v in window),
                 sum(1 for v in window if v >= FULL_SCALE or v <= -FULL_SCALE))


def envelope(samples: array.array, first: int, last: int, block: int = 1) -> list[float]:
    """The RMS of each block of frames from first to last"""
    out = []
    for frame in range(first, last + 1, block):
        start, _ = frame_samples(frame)
        _, end = frame_samples(min(frame + block - 1, last))
        window = samples[start * 2:end * 2]
        out.append(math.sqrt(sum(v * v for v in window) / len(window)) if window else 0.0)
    return out


def correlation(a: list[float], b: list[float]) -> float:
    n = min(len(a), len(b))
    if n < 2:
        return 0.0
    ma = sum(a[:n]) / n
    mb = sum(b[:n]) / n
    sab = sum((a[i] - ma) * (b[i] - mb) for i in range(n))
    saa = sum((a[i] - ma) ** 2 for i in range(n))
    sbb = sum((b[i] - mb) ** 2 for i in range(n))
    if saa == 0 or sbb == 0:
        return 1.0 if saa == sbb else 0.0
    return sab / math.sqrt(saa * sbb)


@dataclass(frozen=True)
class Segment:
    """Port frames first to last of a sequence, the oracle's at a lag (oracle frame = port frame + lag) from low to
    high; exact when the oracle's samples must be the port's to the bit"""
    name: str
    first: int
    last: int
    low: int
    high: int
    exact: bool = False


def parse_segments(text: str) -> list[Segment]:
    """NAME:FIRST-LAST:LOW~HIGH[:exact],..."""
    out = []
    for item in text.split(","):
        parts = item.split(":")
        first, last = (int(x) for x in parts[1].split("-"))
        low, high = (int(x) for x in parts[2].split("~"))
        out.append(Segment(parts[0], first, last, low, high, len(parts) > 3 and parts[3] == "exact"))
    return out


@dataclass
class SegmentResult:
    segment: Segment
    lag: int = 0
    correlation: float = 0.0
    oracle: Level = field(default_factory=Level)
    port: Level = field(default_factory=Level)
    blocks: list[float] = field(default_factory=list)  # the oracle's envelope at the lag, by blocks of frames
    offset: int | None = None  # exact: oracle sample = port sample + offset
    identical: int = 0
    samples: int = 0
    failures: list[str] = field(default_factory=list)


def find_offset(oracle: array.array, port: array.array, port_start: int, guess: int, reach: int) -> int | None:
    """The offset (oracle sample = port sample + offset) at which the port's PROBE_SAMPLES from port_start are the
    oracle's to the bit, near guess"""
    probe = port[port_start * 2:(port_start + PROBE_SAMPLES) * 2]
    if len(probe) < PROBE_SAMPLES * 2 or not any(probe):
        return None
    for delta in range(reach + 1):
        for offset in (guess + delta, guess - delta) if delta else (guess,):
            start = (port_start + offset) * 2
            if start >= 0 and oracle[start:start + len(probe)] == probe:
                return offset
    return None


def loudest_probe(samples: array.array, first: int, last: int) -> int:
    """The start (a stereo sample) of the loudest probe's window in the frames first to last, at a frame's start"""
    best = (-1, frame_samples(first)[0])
    for frame in range(first, last + 1, 8):
        start = frame_samples(frame)[0]
        energy = sum(abs(v) for v in samples[start * 2:(start + PROBE_SAMPLES) * 2])
        if energy > best[0]:
            best = (energy, start)
    return best[1]


def within(have: float, want: float, fraction: float, slack: float = 0.0) -> bool:
    return abs(have - want) <= max(abs(want) * fraction, slack)


def compare_segment(oracle: array.array, port: array.array, segment: Segment) -> SegmentResult:
    result = SegmentResult(segment)
    mine = envelope(port, segment.first, segment.last)
    # The oracle's frames over every lag, once
    first = max(1, segment.first + segment.low)
    theirs = envelope(oracle, first, segment.last + segment.high)
    best = (segment.low, -2.0)
    for lag in range(segment.low, segment.high + 1):
        at = segment.first + lag - first
        window = theirs[at:at + len(mine)] if at >= 0 else []
        if len(window) == len(mine):
            c = correlation(window, mine)
            if c > best[1]:
                best = (lag, c)
    result.lag, result.correlation = best
    result.oracle = level(oracle, segment.first + result.lag, segment.last + result.lag)
    result.port = level(port, segment.first, segment.last)
    result.blocks = envelope(oracle, segment.first + result.lag, segment.last + result.lag, ENVELOPE_BLOCK)
    if result.correlation < ENVELOPE_CORRELATION:
        result.failures.append(f"the envelopes correlate {result.correlation:.4f} at best (lag {result.lag})")
    check_levels(result.oracle, result.port, result.failures)
    if segment.exact:
        probe = loudest_probe(port, segment.first, segment.last)
        guess = int(round(result.lag * SAMPLES_PER_FRAME))
        result.offset = find_offset(oracle, port, probe, guess, int(2 * SAMPLES_PER_FRAME))
        start, _ = frame_samples(segment.first)
        _, end = frame_samples(segment.last)
        result.samples = (end - start) * 2
        if result.offset is None:
            result.failures.append("no offset gives the oracle's samples at the loudest point")
        else:
            theirs = oracle[(start + result.offset) * 2:(end + result.offset) * 2]
            result.identical = sum(1 for a, b in zip(port[start * 2:end * 2], theirs) if a == b)
            if result.identical != result.samples:
                result.failures.append(f"{result.samples - result.identical} of {result.samples} samples aren't the "
                                       "oracle's")
    return result


def check_levels(want: Level, have: Level, failures: list[str]) -> None:
    if not within(have.rms, want.rms, LEVEL_TOLERANCE):
        failures.append(f"RMS {have.rms:.1f}, not {want.rms:.1f}")
    if not within(have.peak, want.peak, LEVEL_TOLERANCE):
        failures.append(f"peak {have.peak}, not {want.peak}")
    if not within(have.clipped, want.clipped, CLIPPED_TOLERANCE, CLIPPED_SLACK):
        failures.append(f"{have.clipped} samples at full scale, not {want.clipped}")


def describe_segment(r: SegmentResult) -> str:
    s = r.segment
    text = (f"{s.name}, port frames {s.first}-{s.last} (oracle = port + {r.lag}): envelope correlation "
            f"{r.correlation:.4f}; RMS {r.oracle.rms:.1f} and {r.port.rms:.1f}, peak {r.oracle.peak} and "
            f"{r.port.peak}, samples at full scale {r.oracle.clipped} and {r.port.clipped} (oracle and port)")
    if s.exact:
        text += (f"; {r.identical} of {r.samples} samples the oracle's" if r.offset is not None
                 else "; no sample offset found")
    return text


# The comparison of two runs, and the reference that keeps it


@dataclass(frozen=True)
class SoundSpec:
    """How a comparison sequence's sound is compared: its plays while the game's count is below before_count (a press
    that reaches the two programs at different points of a sequence ends the plays' comparison), its audio segments,
    and by version the port frames whose status differs by construction, with why"""
    before_count: int | None = None
    segments: tuple[Segment, ...] = ()
    known: dict = field(default_factory=dict)


@dataclass
class Comparison:
    plays: list[PlayResult]
    frames: FrameResult
    segments: list[SegmentResult]
    pairs: list[tuple[int, int]]
    failures: list[str] = field(default_factory=list)


def run_files(directory: str) -> tuple[str, str]:
    return os.path.join(directory, "sound.trace"), os.path.join(directory, "audio.wav")


def compare_runs(oracle_dir: str, port_dir: str, spec: SoundSpec, version: str, clock: str | None,
                 lags: str | None, show: int = 8) -> Comparison:
    """Both runs' traces, statuses and sound, as the sequence's spec says; prints what it found"""
    oracle_trace, oracle_wav = run_files(oracle_dir)
    port_trace, port_wav = run_files(port_dir)
    oracle = load_trace(oracle_trace)
    port = load_trace(port_trace)
    plays = compare_plays(oracle, port, spec.before_count)
    pairs = frame_pairs(compare.load_run(oracle_dir), compare.load_run(port_dir), clock, lags)
    known = known_frames(spec, version)
    frames = compare_frames(oracle, port, pairs, set(known))
    _, oracle_samples = load_wav(oracle_wav)
    _, port_samples = load_wav(port_wav)
    segments = [compare_segment(oracle_samples, port_samples, s) for s in spec.segments]
    comparison = Comparison(plays, frames, segments, pairs)

    print("The driver's state, run by run, by the sequences' own clock:")
    for r in plays:
        for line in describe_play(r, show):
            print(f"  {line}")
        if r.only:
            comparison.failures.append(f"player {r.player}'s play {r.play} is only on the {r.only}")
        elif r.equal != r.pairs or r.pairs == 0:
            comparison.failures.append(f"player {r.player}'s play {r.play}: {r.pairs - r.equal} of {r.pairs} runs "
                                       "differ")
    total = sum(r.pairs for r in plays)
    print(f"  {total} runs paired, {sum(r.equal for r in plays)} equal; "
          f"{sum(r.fields_equal for r in plays)} of {sum(r.fields for r in plays)} fields")
    print(f"What the ARM9 sees of the driver as each frame ends, where the game counts passes: {frames.compared} "
          f"frames compared, {frames.equal} equal, {frames.run_apart} equal a run apart, {len(frames.unequal)} "
          f"unequal; {frames.known} left out")
    for frames_text, why in spec.known.get(version, {}).items():
        print(f"  left out, port frames {frames_text}: {why}")
    for port_frame, oracle_frame, fields in frames.unequal[:show]:
        print(f"  port frame {port_frame} (oracle {oracle_frame}): "
              + "; ".join(f"{n}: oracle {v[0]}, port {v[1]}" for n, v in fields.items()))
        comparison.failures.append(f"port frame {port_frame}'s status isn't oracle frame {oracle_frame}'s")
    if len(frames.unequal) > show:
        comparison.failures.append(f"{len(frames.unequal) - show} more frames' status differs")
    print("The sound:")
    for r in segments:
        print(f"  {describe_segment(r)}")
        comparison.failures += [f"{r.segment.name}: {f}" for f in r.failures]
    return comparison


def known_frames(spec: SoundSpec, version: str) -> list[int]:
    out = []
    for frames_text in spec.known.get(version, {}):
        out += compare.parse_frames(frames_text) or []
    return out


def run_list(counts: list[int]) -> str:
    """[0, 1, 2, 5] as "0-2,5" """
    return ",".join(f"{a}" if a == b else f"{a}-{b}" for a, b in compare.frame_ranges(counts)) if counts else "-"


def chunk_digest(records: list[Record], player: int, indices: list[int]) -> str:
    digest = hashlib.sha1()
    for index in indices:
        digest.update(state_digest(player_state(records[index], player)).encode())
        digest.update(b"\n")
    return digest.hexdigest()


# A play's runs per row of the reference, and the frames between the status rows kept where it doesn't change
PLAY_CHUNK = 64
STATUS_STEP = 8
REFERENCE_MAGIC = "#pbwsound 1"


def write_reference(path: str, comparison: Comparison, oracle_dir: str, port_dir: str, spec: SoundSpec,
                    version: str, description: str) -> int:
    """Writes the reference from a comparison that passed: the oracle's values only (hashes and small statistics),
    at the port's frames and runs the comparison paired them with. Returns its rows"""
    oracle_trace, oracle_wav = run_files(oracle_dir)
    port_trace, port_wav = run_files(port_dir)
    oracle = load_trace(oracle_trace)
    port = load_trace(port_trace)
    _, oracle_samples = load_wav(oracle_wav)
    _, port_samples = load_wav(port_wav)
    rows = []
    # Plays: the oracle's states, PLAY_CHUNK paired runs to a row
    for r in comparison.plays:
        a = find_plays(oracle, r.player)[r.play]
        b = find_plays(port, r.player)[r.play]
        pairs, _ = paired_runs(oracle, port, a, b, spec.before_count)
        for at in range(0, len(pairs), PLAY_CHUNK):
            chunk = pairs[at:at + PLAY_CHUNK]
            counts = [count for count, _, _ in chunk]
            rows.append(("play", r.player, r.play, run_list(counts),
                         chunk_digest(oracle, r.player, [o for _, o, _ in chunk])))
    # Statuses: of the frames equal at their ends (not those a run apart, which the moment of a run decides), every
    # STATUS_STEP-th and each where the oracle's changes
    last = None
    for port_frame, oracle_frame, index in comparison.frames.pairs:
        digest = status_hash(oracle[index])
        if digest != last or port_frame % STATUS_STEP == 0:
            rows.append(("status", port_frame, oracle_frame, "-", digest))
        last = digest
    # Segments: the lag and levels, the envelope by blocks, and for an exact one the hash of its samples
    for r in comparison.segments:
        s = r.segment
        rows.append(("segment", s.name, f"{s.first}-{s.last}", r.lag, r.oracle.as_text()))
        for i, value in enumerate(r.blocks):
            rows.append(("envelope", s.name, s.first + i * ENVELOPE_BLOCK, "-", f"{value:.1f}"))
        if s.exact and r.offset is not None:
            start, _ = frame_samples(s.first)
            _, end = frame_samples(s.last)
            probe = loudest_probe(port_samples, s.first, s.last)
            rows.append(("probe", s.name, probe, r.offset, sample_hash(oracle_samples, probe + r.offset,
                                                                      PROBE_SAMPLES)))
            rows.append(("exact", s.name, f"{start}-{end}", r.offset,
                         sample_hash(oracle_samples, start + r.offset, end - start)))
    with open(path, "w") as f:
        f.write(f"{REFERENCE_MAGIC}\n")
        for line in description.splitlines():
            f.write(f"# {line}\n")
        for frames_text, why in spec.known.get(version, {}).items():
            f.write(f"# Status left out, port frames {frames_text}: {why}\n")
        f.write("kind\ta\tb\tc\tvalue\n")
        for row in rows:
            f.write("\t".join(str(cell) for cell in row) + "\n")
    return len(rows)


def sample_hash(samples: array.array, start: int, count: int) -> str:
    """The SHA-1 of count stereo samples from start, as the WAV file has them (little-endian)"""
    window = samples[max(start, 0) * 2:(start + count) * 2]
    if sys.byteorder != "little":
        window = array.array("h", window)
        window.byteswap()
    return hashlib.sha1(window.tobytes()).hexdigest()


def load_reference(path: str) -> list[list[str]]:
    rows = []
    with open(path) as f:
        lines = f.read().splitlines()
    if not lines or lines[0] != REFERENCE_MAGIC:
        raise compare.RunError(f"{path} isn't a sound reference ({REFERENCE_MAGIC})")
    header = None
    for number, line in enumerate(lines[1:], 2):
        if not line or line.startswith("#"):
            continue
        cells = line.split("\t")
        if header is None:
            header = cells
            continue
        if len(cells) != 5:
            raise compare.RunError(f"{path}:{number}: expected 5 columns")
        rows.append(cells)
    return rows


def check_reference(path: str, port_dir: str) -> tuple[list[str], dict[str, int]]:
    """The port's run against the reference: what fails, and how much was checked"""
    port_trace, port_wav = run_files(port_dir)
    port = load_trace(port_trace)
    _, samples = load_wav(port_wav)
    rows = load_reference(path)
    failures: list[str] = []
    counts = {"runs": 0, "frames": 0, "segments": 0, "blocks": 0, "exact": 0}
    plays: dict[int, list[Play]] = {}
    port_states = frame_states(port)
    segments: dict[str, tuple[int, int, int, Level]] = {}
    blocks: dict[str, list[tuple[int, float]]] = {}
    probes: dict[str, tuple[int, int, str]] = {}
    for kind, a, b, c, value in rows:
        if kind == "play":
            player, play = int(a), int(b)
            found = plays.setdefault(player, find_plays(port, player))
            if play >= len(found):
                failures.append(f"player {player} has no play {play} on the port")
                continue
            by_count = dict(found[play].runs)
            counts_wanted = compare.parse_frames(c) if c != "-" else []
            missing = [n for n in counts_wanted if n not in by_count]
            if missing:
                failures.append(f"player {player}'s play {play} has no runs {run_list(missing)} on the port")
                continue
            counts["runs"] += len(counts_wanted)
            if chunk_digest(port, player, [by_count[n] for n in counts_wanted]) != value:
                failures.append(f"player {player}'s play {play}: runs {c} aren't the oracle's")
        elif kind == "status":
            port_frame = int(a)
            index = port_states.get(port_frame)
            counts["frames"] += 1
            if index is None:
                failures.append(f"port frame {port_frame} has no driver state")
            elif status_hash(port[index]) != value:
                failures.append(f"port frame {port_frame}'s status isn't oracle frame {b}'s")
        elif kind == "segment":
            first, last = (int(x) for x in b.split("-"))
            segments[a] = (first, last, int(c), Level.from_text(value))
        elif kind == "envelope":
            blocks.setdefault(a, []).append((int(b), float(value)))
        elif kind == "probe":
            probes[a] = (int(b), int(c), value)
        elif kind == "exact":
            start, end = (int(x) for x in b.split("-"))
            offset = int(c)
            probe = probes.get(a)
            # The port's own timing may move a stream by a few samples: the probe, the oracle's samples at the loudest
            # point, finds where they are now (shift: the port's samples are shift later than when the reference was
            # written)
            found = None
            if probe is not None:
                reach = int(2 * SAMPLES_PER_FRAME)
                for delta in range(reach + 1):
                    for shift in (delta, -delta) if delta else (0,):
                        if sample_hash(samples, probe[0] + shift, PROBE_SAMPLES) == probe[2]:
                            found = shift
                            break
                    if found is not None:
                        break
            counts["exact"] += 1
            if found is None:
                failures.append(f"{a}: the oracle's samples at its loudest point aren't the port's anywhere near")
            elif sample_hash(samples, start + found, end - start) != value:
                failures.append(f"{a}: the port's samples {start + found}-{end + found} aren't the oracle's "
                                f"(oracle samples from {start + offset})")
    for name, (first, last, lag, want) in segments.items():
        counts["segments"] += 1
        have = level(samples, first, last)
        segment_failures: list[str] = []
        check_levels(want, have, segment_failures)
        theirs = [v for _, v in sorted(blocks.get(name, []))]
        mine = envelope(samples, first, last, ENVELOPE_BLOCK)
        counts["blocks"] += len(theirs)
        c = correlation(theirs, mine)
        if c < ENVELOPE_CORRELATION:
            segment_failures.append(f"the envelopes correlate {c:.4f}")
        failures += [f"{name}: {f}" for f in segment_failures]
    return failures, counts


# The command line


def command_state(args) -> int:
    oracle = load_trace(args.oracle)
    port = load_trace(args.port)
    results = compare_plays(oracle, port, args.before_count, [args.player] if args.player is not None else None)
    for r in results:
        for line in describe_play(r, args.show):
            print(line)
    pairs = sum(r.pairs for r in results)
    equal = sum(r.equal for r in results)
    print(f"total: {pairs} runs paired, {equal} equal; fields {sum(r.fields_equal for r in results)} of "
          f"{sum(r.fields for r in results)} equal")
    return 0 if equal == pairs and not any(r.only for r in results) else 1


def command_frames(args) -> int:
    oracle = load_trace(args.oracle)
    port = load_trace(args.port)
    pairs = frame_pairs(compare.load_run(args.oracle_run), compare.load_run(args.port_run), args.clock, args.lag)
    result = compare_frames(oracle, port, pairs)
    print(f"{result.compared} frames compared on the game's count: {result.equal} equal, {result.run_apart} equal a "
          f"run apart, {len(result.unequal)} unequal")
    for port_frame, oracle_frame, fields in result.unequal[:args.show]:
        print(f"    port frame {port_frame} (oracle {oracle_frame}): "
              + "; ".join(f"{n}: oracle {v[0]}, port {v[1]}" for n, v in fields.items()))
    return 0 if not result.unequal else 1


def command_audio(args) -> int:
    _, oracle = load_wav(args.oracle)
    _, port = load_wav(args.port)
    failed = False
    for segment in parse_segments(args.segments):
        r = compare_segment(oracle, port, segment)
        print(describe_segment(r))
        for failure in r.failures:
            print(f"    {failure}")
        failed = failed or bool(r.failures)
    return 1 if failed else 0


def command_dump(args) -> int:
    record = load_trace(args.trace)[args.index]
    print(f"record {args.index}: cycle {record.cycle}, frame {record.frame}, count {record.counter}, random "
          f"{record.seed:#010x}, periodic runs {record.periods}, players {record.player_status:#x}, channels "
          f"{record.channel_status:#06x}, locks {record.locks[0]:#x} and {record.locks[1]:#x}")
    for player in range(PLAYER_COUNT):
        if record.player_active(player):
            print(f"player {player}: {state_digest(player_state(record, player))}")
    for channel in range(CHANNELS):
        raw = record.channel_raw(channel)
        if raw[3] & 1 or record.cnt(channel) & CNT_START:
            print(f"channel {channel}: {unpack_fields(raw, CHANNEL_FIELDS)}, SOUNDCNT {record.cnt(channel):#010x}, "
                  f"started at mixer sample {record.start_mix(channel)}")
    for alarm in range(ALARM_COUNT):
        raw = record.alarm_raw(alarm)
        if raw[0]:
            tick, period = struct.unpack_from("<QQ", raw, 4)
            print(f"alarm {alarm}: ID {raw[1]}, words to send {raw[2]}, tick {tick}, period {period}")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    state = commands.add_parser("state", help="compare two sound traces play by play")
    state.add_argument("oracle")
    state.add_argument("port")
    state.add_argument("--player", type=int)
    state.add_argument("--before-count", type=int, help="compare plays while the game's count is below this")
    state.add_argument("--show", type=int, default=12)
    frames = commands.add_parser("frames", help="compare two sound traces' status frame by frame")
    frames.add_argument("oracle")
    frames.add_argument("port")
    frames.add_argument("oracle_run")
    frames.add_argument("port_run")
    frames.add_argument("--clock")
    frames.add_argument("--lag")
    frames.add_argument("--show", type=int, default=20)
    audio = commands.add_parser("audio", help="compare two WAV dumps by segment")
    audio.add_argument("oracle")
    audio.add_argument("port")
    audio.add_argument("--segments", required=True)
    dump = commands.add_parser("dump", help="print a trace's record")
    dump.add_argument("trace")
    dump.add_argument("index", type=int)
    args = parser.parse_args(argv)
    try:
        return {"state": command_state, "frames": command_frames, "audio": command_audio,
                "dump": command_dump}[args.command](args)
    except (compare.RunError, OSError, ValueError) as error:
        print(f"sound.py: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())

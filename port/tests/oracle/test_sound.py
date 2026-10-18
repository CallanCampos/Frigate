#!/usr/bin/env python3
"""Tests of sound.py (port/tools/oracle), the sound comparison, on made-up sound traces (sndtrace.h's records) and WAV
dumps: plays and their pairing by the sequence's own clock, field differences, the count limit, note starts, the
status frame by frame with its run apart and its known frames, envelopes, lags and samples to the bit, the reference's
round trip and what makes its check fail, and oracle.py's sound specs and skips. They need neither melonDS nor a ROM.
Standard library only.
"""

from __future__ import annotations

import array
import io
import math
import os
import shutil
import struct
import sys
import tempfile
import unittest

TOOLS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "tools", "oracle")
sys.path.insert(0, os.path.abspath(TOOLS))

import compare  # noqa: E402
import oracle  # noqa: E402
import sound  # noqa: E402

PERIOD = 2728 * 64


def record(cycle: int, frame: int, counter: int = 0, periods: int = 1, players=None, tracks=None, channels=None,
           player_status: int = 0, ticks=None, locks=(0, 0), cnt=None, start_mix=None, alarms=None) -> bytes:
    """A sound trace's record: players {number: (flags, tempo, tempoCounter, [track numbers])}, tracks
    {number: (wait, position, [channel numbers])}, channels {number: (envStatus, volume, timer)}"""
    data = bytearray(sound.RECORD_SIZE)
    struct.pack_into("<QIIII", data, 0, cycle, frame, counter, 0x12345678, periods)
    struct.pack_into("<II", data, sound.OFF_LOCK, *locks)
    for c, value in (cnt or {}).items():
        struct.pack_into("<I", data, sound.OFF_CNT + c * 4, value)
    for c, value in (start_mix or {}).items():
        struct.pack_into("<I", data, sound.OFF_START_MIX + c * 4, value)
    work = sound.OFF_WORK
    for number, (flags, tempo, tempo_counter, track_numbers) in (players or {}).items():
        at = work + sound.PLAYERS_OFF + number * sound.PLAYER_SIZE
        data[at] = flags
        data[at + 1] = number
        data[at + 5] = 127
        numbers = list(track_numbers) + [sound.TRACK_NONE] * (16 - len(track_numbers))
        data[at + 8:at + 24] = bytes(numbers)
        struct.pack_into("<HHH", data, at + 0x18, tempo, 256, tempo_counter)
        struct.pack_into("<I", data, at + 0x20, 0x02200000)
    for number, (wait, position, channel_numbers) in (tracks or {}).items():
        at = work + sound.TRACKS_OFF + number * sound.TRACK_SIZE
        data[at] = 1
        data[at + 4] = 127
        struct.pack_into("<i", data, at + 0x20, wait)
        struct.pack_into("<II", data, at + 0x24, 0x02300000, 0x02300000 + position)
        first = sound.WORK_ADDRESS + channel_numbers[0] * sound.CHANNEL_SIZE if channel_numbers else 0
        struct.pack_into("<I", data, at + 0x3C, first)
        for i, c in enumerate(channel_numbers):
            following = channel_numbers[i + 1:i + 2]
            link = sound.WORK_ADDRESS + following[0] * sound.CHANNEL_SIZE if following else 0
            struct.pack_into("<I", data, work + c * sound.CHANNEL_SIZE + 0x50, link)
            struct.pack_into("<I", data, work + c * sound.CHANNEL_SIZE + 0x4C,
                             sound.WORK_ADDRESS + sound.TRACKS_OFF + number * sound.TRACK_SIZE)
    for number, (env, volume, timer) in (channels or {}).items():
        at = work + number * sound.CHANNEL_SIZE
        data[at] = number
        data[at + 2] = env
        data[at + 3] = 1
        struct.pack_into("<HH", data, at + 0x24, volume, timer)
    struct.pack_into("<IIH", data, sound.OFF_SHARED, 0, player_status, 0)
    for player, value in (ticks or {}).items():
        struct.pack_into("<I", data, sound.OFF_SHARED + 0x20 + player * 0x24 + 0x20, value)
    for number, (identifier, tick, period) in (alarms or {}).items():
        at = work + sound.ALARMS_OFF + number * sound.ALARM_SIZE
        data[at] = 1
        data[at + 1] = identifier
        struct.pack_into("<QQ", data, at + 4, tick, period)
    return bytes(data)


def write_trace(path: str, records: list[bytes]) -> None:
    with open(path, "wb") as f:
        f.write(sound.MAGIC)
        for data in records:
            f.write(data)


def write_wav(path: str, samples: list[int]) -> None:
    data = array.array("h", samples)
    if sys.byteorder != "little":
        data.byteswap()
    body = data.tobytes()
    with open(path, "wb") as f:
        f.write(b"RIFF" + struct.pack("<I", 36 + len(body)) + b"WAVEfmt ")
        f.write(struct.pack("<IHHIIHH", 16, 1, 2, 32728, 32728 * 4, 4, 16))
        f.write(b"data" + struct.pack("<I", len(body)) + body)


def play_records(start_frame: int, runs: int, offset_cycles: int = 0, wait_change: dict | None = None,
                 counter_base: int = 0, start_mix_shift: int = 0) -> list[bytes]:
    """A player 0 playing a track 3 with a channel 2: one periodic run a record, three a frame, from start_frame. The
    track's wait counts down each run, its position moves on every 4 runs, and a note starts every 8"""
    out = [record(offset_cycles, start_frame - 1)]
    for k in range(1, runs + 1):
        wait = 100 - k
        if wait_change and k in wait_change:
            wait = wait_change[k]
        frame = start_frame + (k - 1) // 3
        out.append(record(offset_cycles + k * PERIOD, frame, counter=counter_base + frame,
                          players={0: (3, 120, (k * 120) % 240, [3])}, tracks={3: (wait, k // 4 * 3, [2])},
                          channels={2: (2, 100 + k % 5, 0xFC00)}, player_status=1, ticks={0: k // 2},
                          start_mix={2: 1000 + k // 8 * 1400 + start_mix_shift}))
    out.append(record(offset_cycles + (runs + 1) * PERIOD, start_frame + runs // 3 + 1))
    return out


def tone(frames: int, start: int, amplitude: int, lead: int = 0) -> list[int]:
    """Stereo samples: silence for lead samples, then a tone from frame start that grows and falls"""
    count = int(frames * sound.SAMPLES_PER_FRAME) + 1
    out = [0] * (2 * lead)
    for n in range(count):
        frame = n / sound.SAMPLES_PER_FRAME
        if frame < start:
            value = 0
        else:
            envelope = amplitude * (0.3 + 0.7 * abs(math.sin((frame - start) / 7.0)))
            value = int(envelope * math.sin(n * 0.21)) + (n % 5)
        out += [value, -value]
    return out


class Directories:
    def __init__(self, test: unittest.TestCase):
        self.root = tempfile.mkdtemp(prefix="sound-test-")
        test.addCleanup(shutil.rmtree, self.root, True)
        self.oracle = os.path.join(self.root, "oracle")
        self.port = os.path.join(self.root, "port")
        os.makedirs(self.oracle)
        os.makedirs(self.port)

    def frames(self, run: str, count: int, counter_offset: int = 0) -> None:
        rows = [compare.Frame(n, n, n + counter_offset, None, None, False, False, "a", "b", False, False)
                for n in range(1, count + 1)]
        compare.write_frames_tsv(os.path.join(run, "frames.tsv"), rows)


def quiet(function, *args):
    saved = sys.stdout, sys.stderr
    sys.stdout = sys.stderr = io.StringIO()
    try:
        return function(*args)
    finally:
        sys.stdout, sys.stderr = saved


class PlayTests(unittest.TestCase):
    def setUp(self):
        self.root = tempfile.mkdtemp(prefix="sound-test-")
        self.addCleanup(shutil.rmtree, self.root, True)

    def trace(self, name: str, records: list[bytes]) -> list[sound.Record]:
        path = os.path.join(self.root, name)
        write_trace(path, records)
        return sound.load_trace(path)

    def test_a_play_counts_its_periodic_runs(self):
        records = self.trace("a", play_records(10, 30))
        plays = sound.find_plays(records, 0)
        self.assertEqual(len(plays), 1)
        self.assertEqual(plays[0].runs[0], (1, 1))
        self.assertEqual(plays[0].runs[-1], (30, 30))
        self.assertEqual(sound.find_plays(records, 1), [])

    def test_records_of_two_runs_keep_the_count(self):
        records = play_records(10, 6)
        # The oracle's ARM7 busy over runs 3 and 4: one record with two periodic runs
        merged = records[:3] + [record(4 * PERIOD, 11, 11, 2, players={0: (3, 120, 0, [3])},
                                       tracks={3: (96, 3, [2])}, player_status=1, ticks={0: 2})] + records[5:]
        plays = sound.find_plays(self.trace("a", merged), 0)
        self.assertEqual([count for count, _ in plays[0].runs], [1, 2, 4, 5, 6])

    def test_a_start_over_an_active_player_is_a_new_play(self):
        first = play_records(10, 6)
        second = play_records(20, 6, offset_cycles=8 * PERIOD)
        plays = sound.find_plays(self.trace("a", first[:-1] + second[1:]), 0)
        self.assertEqual(len(plays), 2)

    def test_the_same_plays_at_other_moments_are_equal(self):
        oracle = self.trace("o", play_records(600, 60, offset_cycles=12345678, counter_base=-250))
        port = self.trace("p", play_records(350, 60, offset_cycles=4194304))
        results = sound.compare_plays(oracle, port)
        self.assertEqual(len(results), 1)
        self.assertEqual((results[0].pairs, results[0].equal), (60, 60))
        self.assertEqual(results[0].fields, results[0].fields_equal)

    def test_a_difference_names_its_field_and_run(self):
        oracle = self.trace("o", play_records(600, 40))
        port = self.trace("p", play_records(350, 40, wait_change={17: 5}))
        result = sound.compare_plays(oracle, port)[0]
        self.assertEqual(result.equal, 39)
        self.assertEqual(result.first_unequal, 17)
        self.assertEqual(list(result.differences), ["track0.wait"])
        self.assertEqual(result.differences["track0.wait"][2:], [83, 5])

    def test_a_play_on_one_side_only(self):
        oracle = self.trace("o", play_records(600, 10))
        port = self.trace("p", [record(0, 1), record(PERIOD, 2)])
        result = sound.compare_plays(oracle, port)[0]
        self.assertEqual(result.only, "oracle")

    def test_the_count_limit_leaves_runs_out(self):
        oracle = self.trace("o", play_records(600, 30, counter_base=-500, wait_change={25: 1}))
        port = self.trace("p", play_records(350, 30, counter_base=-250))
        # Both reach count 107 at run 22 (frame 607 on the oracle, 357 on the port): the difference at run 25 is past it
        result = sound.compare_plays(oracle, port, before_count=107)[0]
        self.assertEqual(result.pairs + result.left, 30)
        self.assertEqual(result.equal, result.pairs)
        self.assertGreater(result.left, 0)

    def test_note_starts_pair_and_spread(self):
        oracle = self.trace("o", play_records(600, 40, start_mix_shift=200000))
        port = self.trace("p", play_records(350, 40))
        result = sound.compare_plays(oracle, port)[0]
        self.assertEqual(set(o - p for p, o in result.starts.items()), {200000})
        self.assertIn("6 at +0", sound.describe_starts(result.starts))


class FrameTests(unittest.TestCase):
    def setUp(self):
        self.root = tempfile.mkdtemp(prefix="sound-test-")
        self.addCleanup(shutil.rmtree, self.root, True)

    def trace(self, name: str, records: list[bytes]) -> list[sound.Record]:
        path = os.path.join(self.root, name)
        write_trace(path, records)
        return sound.load_trace(path)

    def statuses(self, changes: dict[int, int], frames: int = 10) -> list[bytes]:
        """Three runs a frame; the shared work's player status from the given runs on"""
        out = []
        status = 0
        for k in range(frames * 3):
            status = changes.get(k, status)
            out.append(record(k * PERIOD, k // 3 + 1, player_status=status, locks=(0x41, 0), cnt={0: 0xA800007F}))
        return out

    def test_frames_compare_at_their_ends(self):
        oracle = self.trace("o", self.statuses({7: 1}))
        port = self.trace("p", self.statuses({7: 1}))
        result = sound.compare_frames(oracle, port, [(f, f) for f in range(1, 11)])
        self.assertEqual((result.compared, result.equal), (10, 10))

    def test_a_change_a_run_apart(self):
        # The change comes in the last run of frame 3 on one, the first of frame 4 on the other
        oracle = self.trace("o", self.statuses({8: 1}))
        port = self.trace("p", self.statuses({9: 1}))
        result = sound.compare_frames(oracle, port, [(f, f) for f in range(1, 11)])
        self.assertEqual((result.equal, result.run_apart, len(result.unequal)), (9, 1, 0))

    def test_a_difference_and_a_known_frame(self):
        oracle = self.trace("o", self.statuses({3: 1}))
        port = self.trace("p", self.statuses({12: 1}))
        result = sound.compare_frames(oracle, port, [(f, f) for f in range(1, 11)], known={2})
        self.assertEqual(result.known, 1)
        self.assertEqual([frame for frame, _, _ in result.unequal], [3])
        # The player's status, and with it its tick count among the active players'
        self.assertEqual(list(result.unequal[0][2]), ["players", "ticks"])


class AudioTests(unittest.TestCase):
    def test_a_lag_and_samples_to_the_bit(self):
        port = array.array("h", tone(200, 20, 9000))
        lead = 1234
        oracle = array.array("h", [0] * (2 * lead)) + port
        segment = sound.Segment("s", 30, 150, 0, 6, exact=True)
        result = sound.compare_segment(oracle, port, segment)
        self.assertEqual(result.lag, round(lead / sound.SAMPLES_PER_FRAME))
        self.assertGreater(result.correlation, 0.99)
        self.assertEqual(result.offset, lead)
        self.assertEqual(result.identical, result.samples)
        self.assertEqual(result.failures, [])

    def test_levels_and_samples_that_differ(self):
        port = array.array("h", tone(200, 20, 9000))
        oracle = array.array("h", (int(v * 1.1) for v in port))
        result = sound.compare_segment(oracle, port, sound.Segment("s", 30, 150, -2, 2, exact=True))
        self.assertTrue(any(f.startswith("RMS") for f in result.failures))
        self.assertTrue(any("samples" in f for f in result.failures))

    def test_full_scale_counts(self):
        samples = array.array("h", [0x7FFF, -0x8000, 100, -0x7FFF] * 300)
        self.assertEqual(sound.level(samples, 1, 2).clipped, 900)

    def test_wav_round_trip(self):
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "a.wav")
            write_wav(path, [1, -2, 3, -4])
            rate, samples = sound.load_wav(path)
        self.assertEqual((rate, list(samples)), (32728, [1, -2, 3, -4]))


class ReferenceTests(unittest.TestCase):
    def make(self, port_wait_change=None, port_scale=1.0):
        d = Directories(self)
        write_trace(os.path.join(d.oracle, "sound.trace"), play_records(300, 240, counter_base=-30))
        write_trace(os.path.join(d.port, "sound.trace"), play_records(270, 240, wait_change=port_wait_change))
        port = tone(400, 280, 8000)
        write_wav(os.path.join(d.port, "audio.wav"), [int(v * port_scale) for v in port])
        write_wav(os.path.join(d.oracle, "audio.wav"), [0] * (2 * 16411) + tone(400, 280, 8000))
        d.frames(d.oracle, 430, counter_offset=-30)
        d.frames(d.port, 400)
        return d

    SPEC = sound.SoundSpec(segments=(sound.Segment("tone", 285, 380, 20, 40, exact=True),))

    def write(self, d) -> str:
        comparison = quiet(sound.compare_runs, d.oracle, d.port, self.SPEC, "b2_us", None, None)
        self.assertEqual(comparison.failures, [])
        path = os.path.join(d.root, "reference.tsv")
        self.assertGreater(sound.write_reference(path, comparison, d.oracle, d.port, self.SPEC, "b2_us", "a test"),
                           0)
        return path

    def test_the_port_that_wrote_it_passes(self):
        d = self.make()
        path = self.write(d)
        failures, counts = sound.check_reference(path, d.port)
        self.assertEqual(failures, [])
        self.assertEqual(counts["runs"], 240)
        self.assertEqual(counts["exact"], 1)
        self.assertGreater(counts["frames"], 0)

    def test_a_state_that_differs_fails(self):
        d = self.make()
        path = self.write(d)
        write_trace(os.path.join(d.port, "sound.trace"), play_records(270, 240, wait_change={100: 1}))
        failures, _ = sound.check_reference(path, d.port)
        self.assertEqual(len(failures), 1)
        self.assertIn("runs 65-128", failures[0])

    def test_sound_that_differs_fails(self):
        d = self.make()
        path = self.write(d)
        write_wav(os.path.join(d.port, "audio.wav"), [int(v * 1.2) for v in tone(400, 280, 8000)])
        failures, _ = sound.check_reference(path, d.port)
        self.assertTrue(any("RMS" in f for f in failures))
        self.assertTrue(any("loudest point" in f for f in failures))

    def test_sound_a_few_samples_later_passes(self):
        d = self.make()
        path = self.write(d)
        write_wav(os.path.join(d.port, "audio.wav"), [0] * 6 + tone(400, 280, 8000))
        failures, _ = sound.check_reference(path, d.port)
        self.assertEqual(failures, [])

    def test_a_reference_of_another_kind_is_refused(self):
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "r.tsv")
            with open(path, "w") as f:
                f.write("port\toracle\n")
            with self.assertRaises(compare.RunError):
                sound.load_reference(path)


class OracleSoundTests(unittest.TestCase):
    def test_the_specs_fit_their_sequences(self):
        for name, spec in oracle.SOUND.items():
            sequence = oracle.SEQUENCES[name]
            for segment in spec.segments:
                self.assertTrue(1 <= segment.first <= segment.last <= sequence.port_frames, (name, segment))
                self.assertLessEqual(segment.low, segment.high)
            for version, table in spec.known.items():
                self.assertIn(version, ("b2_us", "w2_us"))
                for frames in table:
                    self.assertTrue(compare.parse_frames(frames), (name, frames))

    def test_the_references_parse(self):
        for name in oracle.SOUND:
            for version in ("b2_us", "w2_us"):
                path = os.path.join(oracle.REPO_ROOT, oracle.SOUND_REFERENCE.format(name=name, version=version))
                if os.path.exists(path):
                    rows = sound.load_reference(path)
                    self.assertTrue(any(row[0] == "play" for row in rows), path)
                    self.assertTrue(any(row[0] == "exact" for row in rows), path)

    def test_sound_skips_without_the_oracle(self):
        with tempfile.TemporaryDirectory() as d:
            status = quiet(oracle.main, ["sound", "opening", "--oracle", os.path.join(d, "missing"), "--port", "x"])
        self.assertEqual(status, oracle.SKIP)

    def test_sound_check_skips_without_the_rom(self):
        with tempfile.TemporaryDirectory() as d:
            status = quiet(oracle.main, ["sound-check", "title", "--port", "x", "--rom",
                                         os.path.join(d, "missing.nds")])
        self.assertEqual(status, oracle.SKIP)


if __name__ == "__main__":
    unittest.main(verbosity=1)

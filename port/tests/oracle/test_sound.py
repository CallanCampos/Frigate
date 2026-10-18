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
           player_status: int = 0, ticks=None, locks=(0, 0), cnt=None, start_mix=None, alarms=None,
           run_mix: int = 0) -> bytes:
    """A sound trace's record: players {number: (flags, tempo, tempoCounter, [track numbers])}, tracks
    {number: (wait, position, [channel numbers])}, channels {number: (envStatus, volume, timer)}"""
    data = bytearray(sound.RECORD_SIZE)
    struct.pack_into("<QIIII", data, 0, cycle, frame, counter, 0x12345678, periods)
    struct.pack_into("<I", data, sound.OFF_RUN_MIX, run_mix)
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


def run_cycle(frame: int, slot: int) -> int:
    """The cycle of a frame's run in a made-up trace: three a frame, PERIOD apart, after the VBlank that the frame
    before ends with, so that each record is in the frame whose VBlank comes next"""
    return sound.FIRST_VBLANK_CYCLE + (frame - 2) * sound.CYCLES_PER_FRAME + 1 + slot * PERIOD


def play_records(start_frame: int, runs: int, wait_change: dict | None = None, counter_base: int = 0,
                 mix_base: int = 1000, start_delay: int = 0) -> list[bytes]:
    """A player 0 playing a track 3 with a channel 2: one periodic run a record, three a frame, from start_frame. The
    track's wait counts down each run, its position moves on every 4 runs, and a note starts at run 1 and every 8th:
    in the mixer sample start_delay after the one by which its run had begun (171 samples a run from mix_base)"""
    out = [record(run_cycle(start_frame - 1, 0), start_frame - 1)]
    for k in range(1, runs + 1):
        wait = 100 - k
        if wait_change and k in wait_change:
            wait = wait_change[k]
        frame = start_frame + (k - 1) // 3
        note = max(k // 8 * 8, 1)
        out.append(record(run_cycle(frame, (k - 1) % 3), frame, counter=counter_base + frame,
                          players={0: (3, 120, (k * 120) % 240, [3])}, tracks={3: (wait, k // 4 * 3, [2])},
                          channels={2: (2, 100 + k % 5, 0xFC00)}, player_status=1, ticks={0: k // 2},
                          start_mix={2: mix_base + note * 171 + start_delay}, run_mix=mix_base + k * 171))
    out.append(record(run_cycle(start_frame + runs // 3 + 1, 0), start_frame + runs // 3 + 1))
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
        second = play_records(20, 6)
        plays = sound.find_plays(self.trace("a", first[:-1] + second[1:]), 0)
        self.assertEqual(len(plays), 2)

    def test_the_same_plays_at_other_moments_are_equal(self):
        oracle = self.trace("o", play_records(600, 60, counter_base=-250))
        port = self.trace("p", play_records(350, 60))
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
        self.assertEqual((result.pairing.window, result.pairs, result.equal), (22, 21, 21))
        self.assertEqual((result.pairing.left_oracle, result.pairing.left_port), (9, 9))
        self.assertEqual(sound.play_failures(result), [])

    def test_the_count_limit_comes_first_on_either_program(self):
        # The press reaches the port 3 frames (9 runs) into the play and the oracle 6 frames in: from the port's run 10
        # on, the oracle's runs have nothing to pair with, and are left out, not missing
        oracle = self.trace("o", play_records(600, 40, counter_base=-500))
        port = self.trace("p", play_records(350, 40, counter_base=-247))
        result = sound.compare_plays(oracle, port, before_count=106)[0]
        self.assertEqual((result.pairing.window, result.pairs), (10, 9))
        self.assertEqual((result.pairing.left_oracle, result.pairing.left_port), (31, 31))
        self.assertEqual(sound.play_failures(result), [])

    def test_a_play_that_ends_early_on_the_port_fails(self):
        oracle = self.trace("o", play_records(600, 40))
        port = self.trace("p", play_records(350, 35))
        result = sound.compare_plays(oracle, port)[0]
        self.assertEqual((result.pairs, result.equal), (35, 35))
        self.assertEqual(result.pairing.oracle_only, [36, 37, 38, 39, 40])
        self.assertEqual(sound.play_failures(result), ["player 0's play 0: runs 36-40 are only on the oracle"])

    def test_a_play_that_ends_late_on_the_port_fails(self):
        oracle = self.trace("o", play_records(600, 40))
        port = self.trace("p", play_records(350, 44))
        result = sound.compare_plays(oracle, port)[0]
        self.assertEqual(result.pairing.port_only, [41, 42, 43, 44])
        self.assertEqual(sound.play_failures(result), ["player 0's play 0: runs 41-44 are only on the port"])

    def test_a_run_that_a_record_of_two_stands_for_isnt_missing(self):
        # The oracle's ARM7 busy over runs 3 and 4: one record of both, with run 4's state. The port's run 3 has
        # nothing to pair with, but isn't extra
        records = play_records(600, 6)
        both = bytearray(records[4])
        struct.pack_into("<I", both, sound.OFF_PERIODS, 2)
        merged = records[:3] + [bytes(both)] + records[5:]
        result = sound.compare_plays(self.trace("o", merged), self.trace("p", play_records(350, 6)))[0]
        self.assertEqual((result.pairing.covered, result.pairing.port_only, result.pairs), ([3], [], 5))
        self.assertEqual(sound.play_failures(result), [])

    def test_the_state_command_fails_on_a_play_cut_short(self):
        oracle = os.path.join(self.root, "o")
        port = os.path.join(self.root, "p")
        write_trace(oracle, play_records(600, 40))
        write_trace(port, play_records(350, 38))
        self.assertEqual(quiet(sound.main, ["state", oracle, port]), 1)
        write_trace(port, play_records(350, 40))
        self.assertEqual(quiet(sound.main, ["state", oracle, port]), 0)

    def test_note_starts_by_their_delay_after_the_run(self):
        oracle = self.trace("o", play_records(600, 40, mix_base=200000, start_delay=3))
        port = self.trace("p", play_records(350, 40))
        result = sound.compare_plays(oracle, port)[0]
        # The notes of runs 1, 8, 16, 24, 32 and 40, each on the port in the sample after its run
        self.assertEqual(sorted(result.starts), [1000 + k * 171 for k in (1, 8, 16, 24, 32, 40)])
        self.assertEqual(sound.start_delays(result.starts), ({3: 6}, 0))
        self.assertIn("6 at 3", sound.describe_starts(result.starts))

    def test_a_start_before_the_run_is_one_the_sampler_missed(self):
        oracle = self.trace("o", play_records(600, 16, start_delay=-1))
        port = self.trace("p", play_records(350, 16))
        result = sound.compare_plays(oracle, port)[0]
        self.assertEqual(sound.start_delays(result.starts), ({}, 3))
        self.assertIn("3 restarts the oracle's sampler can't see", sound.describe_starts(result.starts))


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
        # The player is active from run 7, in frame 3, whose last run is 8: frames 3-10 hold its tick count, and the
        # frames have two statuses between them
        self.assertEqual((result.with_player, result.statuses), (8, 2))

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


class StampTests(unittest.TestCase):
    def test_a_record_belongs_to_the_frame_whose_vblank_comes_next(self):
        vblank = sound.FIRST_VBLANK_CYCLE
        frame = sound.CYCLES_PER_FRAME
        self.assertEqual([sound.frame_after(c) for c in (0, vblank - 1, vblank, vblank + frame - 1, vblank + frame)],
                         [1, 1, 2, 2, 3])

    def test_a_record_stamped_by_melonds_frame_is_found(self):
        vblank = sound.FIRST_VBLANK_CYCLE
        frame = sound.CYCLES_PER_FRAME
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "t")
            # Frame 2: before its VBlank, then (wrongly) after it, as melonDS's frame counts lines 192-262; then a
            # record at frame 3's VBlank's own cycle, which may go either way
            write_trace(path, [record(vblank + 100, 2), record(vblank + frame + 100, 2),
                               record(vblank + frame, 2), record(vblank + frame, 3)])
            self.assertEqual(sound.misstamped(sound.load_trace(path)), [1])


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

    def test_a_segment_not_to_the_bit_finds_where_most_samples_agree(self):
        port = array.array("h", tone(200, 20, 9000))
        lead = 1234
        oracle = array.array("h", [0] * (2 * lead)) + port
        # Every 10th stereo sample of the oracle's differs, as a note that starts a sample apart would make it
        for n in range(lead, len(oracle) // 2, 10):
            oracle[2 * n] += 1
        result = sound.compare_segment(oracle, port, sound.Segment("s", 30, 150, 0, 6))
        self.assertEqual(result.failures, [])
        self.assertEqual(result.offset, lead)
        self.assertAlmostEqual(result.identical / result.samples, 0.95, delta=0.001)
        self.assertIn("(95.0%) the oracle's at the sample offset where most are", sound.describe_segment(result))

    def test_full_scale_by_second(self):
        rate = int(sound.SAMPLE_RATE)
        samples = array.array("h", [0] * (2 * 3 * rate))
        # Three samples at full scale in second 0, one (left and right) in second 2
        samples[10] = 0x7FFF
        samples[11] = -0x8000
        samples[200] = 0x7FFF
        samples[2 * (2 * rate + 5)] = 0x7FFF
        samples[2 * (2 * rate + 5) + 1] = -0x7FFF
        self.assertEqual(sound.clipped_seconds(samples, 0, 3 * rate), {0: 3, 2: 2})
        # The same samples 100 later, counted in the port's seconds
        later = array.array("h", [0] * 200) + samples
        self.assertEqual(sound.clipped_seconds(later, 100, 3 * rate + 100, 100), {0: 3, 2: 2})

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

    def write(self, d, statuses: bool = True) -> str:
        comparison = quiet(sound.compare_runs, d.oracle, d.port, self.SPEC, "b2_us", None, None)
        self.assertEqual(comparison.failures, [])
        path = os.path.join(d.root, "reference.tsv")
        self.assertGreater(sound.write_reference(path, comparison, d.oracle, d.port, self.SPEC, "b2_us", "a test"),
                           0)
        if not statuses:
            # As where the play's frames aren't on the game's count (the opening's movies): only its runs hold it
            with open(path) as f:
                lines = [line for line in f if not line.startswith("status\t")]
            with open(path, "w") as f:
                f.writelines(lines)
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

    def test_a_play_that_ends_late_fails(self):
        d = self.make()
        path = self.write(d, statuses=False)
        write_trace(os.path.join(d.port, "sound.trace"), play_records(270, 245))
        failures, _ = sound.check_reference(path, d.port)
        self.assertEqual(failures, ["player 0's play 0 has runs 241-245 on the port that the oracle doesn't"])

    def test_a_play_that_ends_early_fails(self):
        d = self.make()
        path = self.write(d, statuses=False)
        write_trace(os.path.join(d.port, "sound.trace"), play_records(270, 230))
        failures, _ = sound.check_reference(path, d.port)
        self.assertEqual(failures, ["player 0's play 0 has no runs 231-240 on the port"])

    def test_a_play_the_oracle_doesnt_have_fails(self):
        d = self.make()
        path = self.write(d, statuses=False)
        # The same play, then a second one from a start over the active player
        first = play_records(270, 240)
        second = play_records(352, 10)
        write_trace(os.path.join(d.port, "sound.trace"), first[:-1] + second[1:])
        failures, _ = sound.check_reference(path, d.port)
        self.assertEqual(failures, ["player 0 has 2 plays on the port, the oracle 1"])

    def test_a_record_in_the_wrong_frame_fails(self):
        d = self.make()
        path = self.write(d, statuses=False)
        records = play_records(270, 240)
        # Run 100's record stamped with the frame after its VBlank's, as melonDS's own frame number would
        wrong = bytearray(records[100])
        struct.pack_into("<I", wrong, sound.OFF_FRAME, struct.unpack_from("<I", wrong, sound.OFF_FRAME)[0] + 1)
        records[100] = bytes(wrong)
        write_trace(os.path.join(d.port, "sound.trace"), records)
        failures, _ = sound.check_reference(path, d.port)
        self.assertEqual(failures, ["records of the port's trace not in the frame whose VBlank comes next: 1, the "
                                    "first 100"])

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

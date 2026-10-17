#!/usr/bin/env python3
"""Tests of the oracle's Python side (port/tools/oracle): compare.py's PNG reader and writer, frames.tsv's versions,
screen comparisons exact and with the 3D tolerance screen by screen, comparisons by hash, frame alignment, memory
comparisons, symbols.py's map parser and name lookup, and oracle.py's handling of the port's runs (with a stand-in
program), of the clock, of the comparison sequences and their references, and of a missing oracle or ROM. They make
up their own runs and need neither melonDS nor a ROM. Standard library only.
"""

from __future__ import annotations

import io
import os
import shutil
import stat
import struct
import sys
import tempfile
import unittest
import zlib

TOOLS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "tools", "oracle")
sys.path.insert(0, os.path.abspath(TOOLS))

import compare  # noqa: E402
import oracle  # noqa: E402
import symbols  # noqa: E402

W = compare.SCREEN_WIDTH
H = compare.SCREEN_HEIGHT * 2


def solid(colour: bytes) -> bytearray:
    return bytearray(colour * (W * H))


def put(image: bytearray, x: int, y: int, colour: bytes) -> None:
    image[(y * W + x) * 3 : (y * W + x) * 3 + 3] = colour


def png_with_filters(width: int, height: int, pixels: bytes, channels: int, colour_type: int) -> bytes:
    """A PNG whose rows cycle through the five filter types, to test the reader's unfiltering"""
    stride = width * channels

    def paeth(a, b, c):
        p = a + b - c
        pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
        return a if pa <= pb and pa <= pc else b if pb <= pc else c

    raw = bytearray()
    previous = bytes(stride)
    for y in range(height):
        row = pixels[y * stride : (y + 1) * stride]
        kind = y % 5
        out = bytearray()
        for i in range(stride):
            a = row[i - channels] if i >= channels else 0
            b = previous[i]
            c = previous[i - channels] if i >= channels else 0
            predictor = [0, a, b, (a + b) >> 1, paeth(a, b, c)][kind]
            out.append((row[i] - predictor) & 0xFF)
        raw += bytes([kind]) + out
        previous = row

    def chunk(kind: bytes, body: bytes) -> bytes:
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)

    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, colour_type, 0, 0, 0)) +
            chunk(b"IDAT", zlib.compress(bytes(raw))) + chunk(b"IEND", b""))


class Runs:
    """Two run directories made up for a test"""

    def __init__(self, test: unittest.TestCase):
        self.root = tempfile.mkdtemp(prefix="oracle-test-")
        test.addCleanup(shutil.rmtree, self.root, True)
        self.oracle = os.path.join(self.root, "oracle")
        self.port = os.path.join(self.root, "port")
        os.makedirs(self.oracle)
        os.makedirs(self.port)
        self.rows = {self.oracle: [], self.port: []}

    def frame(self, run: str, number: int, image: bytes, counter=None, three_d=None, png=True) -> None:
        """three_d: None, a bool for both screens, or (top, bottom)"""
        path = os.path.join(run, compare.FRAME_FILE.format(number))
        compare.write_png(path, W, H, bytes(image))
        row = compare.frame_from_png(number, path)
        row.counter = counter
        row.top_3d, row.bottom_3d = three_d if isinstance(three_d, tuple) else (three_d, three_d)
        self.rows[run].append(row)
        if not png:
            os.remove(path)
        compare.write_frames_tsv(os.path.join(run, "frames.tsv"), self.rows[run])

    def memory(self, run: str, lines: list[str]) -> None:
        with open(os.path.join(run, "memory.txt"), "w") as f:
            f.write("#pbwmem 1\n" + "".join(line + "\n" for line in lines))


def captured(function, *args) -> tuple[int, str]:
    """Runs a command; returns its status and what it printed"""
    out = io.StringIO()
    saved = sys.stdout, sys.stderr
    sys.stdout = sys.stderr = out
    try:
        return function(*args), out.getvalue()
    finally:
        sys.stdout, sys.stderr = saved


def quiet(function, *args):
    """Runs a command, its output swallowed; returns its status"""
    devnull = open(os.devnull, "w")
    saved = sys.stdout, sys.stderr
    sys.stdout = sys.stderr = devnull
    try:
        return function(*args)
    finally:
        sys.stdout, sys.stderr = saved
        devnull.close()


class PngTests(unittest.TestCase):
    def test_round_trip(self):
        image = solid(b"\x10\x20\x30")
        put(image, 5, 7, b"\xff\x00\x80")
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "a.png")
            compare.write_png(path, W, H, bytes(image))
            self.assertEqual(compare.read_png(path), (W, H, bytes(image)))

    def test_filters_and_colour_types(self):
        width, height = 7, 11
        rgb = bytes((x * 37 + y * 11 + c * 101) & 0xFF for y in range(height) for x in range(width) for c in range(3))
        rgba = bytearray()
        for i in range(0, len(rgb), 3):
            rgba += rgb[i : i + 3] + bytes([i & 0xFF])
        grey = bytes((x * 13 + y * 7) & 0xFF for y in range(height) for x in range(width))
        with tempfile.TemporaryDirectory() as d:
            for name, data, channels, colour, expected in (
                ("rgb", rgb, 3, 2, rgb),
                ("rgba", bytes(rgba), 4, 6, rgb),
                ("grey", grey, 1, 0, bytes(v for v in grey for _ in range(3))),
            ):
                path = os.path.join(d, name + ".png")
                with open(path, "wb") as f:
                    f.write(png_with_filters(width, height, data, channels, colour))
                self.assertEqual(compare.read_png(path), (width, height, expected), name)

    def test_bad_files(self):
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "bad.png")
            with open(path, "wb") as f:
                f.write(b"not a png")
            with self.assertRaises(compare.RunError):
                compare.read_png(path)

    def test_hashes_and_blank(self):
        image = solid(b"\x00\x00\x00")
        top, bottom = compare.screen_hashes(bytes(image))
        self.assertEqual(top, bottom)
        self.assertTrue(compare.is_blank(bytes(image[: len(image) // 2])))
        # A stray line still counts as blank; a picture doesn't
        for x in range(W):
            put(image, x, 0, b"\xff\xff\xff")
        self.assertTrue(compare.is_blank(bytes(image[: len(image) // 2])))
        put(image, 0, 1, b"\xff\xff\xff")
        self.assertFalse(compare.is_blank(bytes(image[: len(image) // 2])))
        self.assertNotEqual(compare.screen_hashes(bytes(image))[0], top)


class ScreenTests(unittest.TestCase):
    def test_exact_frames(self):
        runs = Runs(self)
        image = solid(b"\x08\x10\x18")
        put(image, 100, 300, b"\xfb\xfb\xfb")
        runs.frame(runs.oracle, 10, image, three_d=False)
        runs.frame(runs.port, 10, image)
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port]), 0)

    def test_six_bit_depth(self):
        # The low two bits of each channel are below the DS's colour depth
        runs = Runs(self)
        a = solid(b"\xf8\xf8\xf8")
        b = solid(b"\xfb\xf9\xfa")
        runs.frame(runs.oracle, 1, a, three_d=False)
        runs.frame(runs.port, 1, b)
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port]), 0)

    def test_2d_difference_fails_with_a_report(self):
        runs = Runs(self)
        a = solid(b"\x00\x00\x00")
        b = bytearray(a)
        put(b, 3, 4, b"\x40\x00\x00")
        runs.frame(runs.oracle, 5, a, three_d=False)
        runs.frame(runs.port, 5, b)
        report = os.path.join(runs.root, "report")
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--report", report]), 1)
        diff = os.path.join(report, "diff_port000005_oracle000005.png")
        width, height, rgb = compare.read_png(diff)
        self.assertEqual((width, height), (W * 3, H))
        # The difference panel marks the pixel in red
        at = (4 * W * 3 + 2 * W + 3) * 3
        self.assertEqual(rgb[at : at + 3], b"\xff\x00\x00")

    def test_3d_tolerance(self):
        runs = Runs(self)
        a = solid(b"\x00\x00\x00")
        # A polygon edge one pixel to the right in the port: within the tolerance
        b = bytearray(a)
        for y in range(20, 60):
            put(a, 50, y, b"\xc0\xc0\xc0")
            put(b, 51, y, b"\xc0\xc0\xc0")
        runs.frame(runs.oracle, 1, a, three_d=True)
        runs.frame(runs.port, 1, b)
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port]), 0)
        # The same shift in a frame without 3D fails
        runs.frame(runs.oracle, 2, a, three_d=False)
        runs.frame(runs.port, 2, b)
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--frames", "2"]), 1)
        # A large difference fails in a 3D frame too
        c = bytearray(a)
        for y in range(100, 200):
            for x in range(0, 200):
                put(c, x, y, b"\xff\x00\x00")
        runs.frame(runs.oracle, 3, a, three_d=True)
        runs.frame(runs.port, 3, c)
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--frames", "3"]), 1)

    def test_3d_screen_by_screen(self):
        runs = Runs(self)
        black = solid(b"\x00\x00\x00")
        # A 20x24 block on the bottom screen, which has no 3D, in a frame whose top screen has it: no tolerance there
        block = bytearray(black)
        for y in range(300, 324):
            for x in range(100, 120):
                put(block, x, y, b"\xff\x00\x00")
        runs.frame(runs.oracle, 1, black, three_d=(True, False))
        runs.frame(runs.port, 1, block)
        status, out = captured(compare.main, ["screens", runs.oracle, runs.port])
        self.assertEqual(status, 1, out)
        self.assertIn("bottom 480 pixels differ", out)
        # The same block on the 3D screen is 1% of its pixels, beyond the 0.5% a screen with 3D may differ by
        runs.frame(runs.oracle, 2, black, three_d=(False, True))
        runs.frame(runs.port, 2, block)
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--frames", "2"]), 1)
        # 200 scattered pixels are within it, on the 3D screen only
        scattered = bytearray(black)
        for i in range(200):
            put(scattered, (i * 37) % W, 200 + (i * 7) % 180, b"\xff\xff\xff")
        runs.frame(runs.oracle, 3, black, three_d=(False, True))
        runs.frame(runs.port, 3, scattered)
        status, out = captured(compare.main, ["screens", runs.oracle, runs.port, "--frames", "3"])
        self.assertEqual(status, 0, out)
        self.assertIn("bottom 200 pixels differ, 200 beyond tolerance", out)
        runs.frame(runs.oracle, 4, black, three_d=(True, False))
        runs.frame(runs.port, 4, scattered)
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--frames", "4"]), 1)

    def test_hashes_only(self):
        runs = Runs(self)
        picture = solid(b"\x80\x80\x80")
        # Green's low bit of the 6-bit output, which --depth 5 leaves out
        greener = bytearray(picture)
        greener[1::3] = bytes(v + 4 for v in greener[1::3])
        runs.frame(runs.oracle, 1, picture, three_d=False, png=False)
        runs.frame(runs.oracle, 2, picture, three_d=(False, True), png=False)
        runs.frame(runs.oracle, 3, picture, three_d=True, png=False)
        runs.frame(runs.port, 1, greener)
        runs.frame(runs.port, 2, greener)
        runs.frame(runs.port, 3, picture)
        # At 6 bits, a screen without 3D whose hashes differ fails
        status, out = captured(compare.main, ["screens", runs.oracle, runs.port, "--frames", "1"])
        self.assertEqual(status, 1, out)
        self.assertIn("FAIL", out)
        # At 5 bits the hashes can't say: not compared, and not a pass
        status, out = captured(compare.main, ["screens", runs.oracle, runs.port, "--frames", "1", "--depth", "5"])
        self.assertEqual(status, 1, out)
        self.assertIn("not compared", out)
        self.assertNotIn("FAIL", out)
        # A screen with 3D whose hashes differ isn't compared; the other, without, still fails at 6 bits
        status, out = captured(compare.main, ["screens", runs.oracle, runs.port, "--frames", "2"])
        self.assertEqual(status, 1, out)
        self.assertIn("top its hashes differ; bottom no picture", out)
        # Equal hashes are exact, 3D or not
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--frames", "3"]), 0)

    def test_frames_tsv_version_1(self):
        runs = Runs(self)
        with open(os.path.join(runs.oracle, "frames.tsv"), "w") as f:
            f.write("#pbwframes 1\nframe\tvblank\tcounter\tkeyread\t3d\ttop\tbottom\ttop_blank\tbottom_blank\n"
                    "1\t5\t-\t1\t1\taa\tbb\t0\t1\n2\t6\t3\t0\t0\taa\tbb\t1\t1\n")
        run = compare.load_run(runs.oracle)
        self.assertEqual((run.frames[1].top_3d, run.frames[1].bottom_3d, run.frames[1].game_frame), (True, True, None))
        self.assertEqual((run.frames[2].top_3d, run.frames[2].counter, run.frames[2].keyread), (False, 3, 0))
        # Written back as version 2
        compare.write_frames_tsv(os.path.join(runs.port, "frames.tsv"), list(run.frames.values()))
        with open(os.path.join(runs.port, "frames.tsv")) as f:
            self.assertEqual(f.readline(), "#pbwframes 2\n")
        self.assertEqual(compare.load_run(runs.port).frames, run.frames)

    def test_offset_and_missing_pictures(self):
        runs = Runs(self)
        image = solid(b"\x20\x20\x20")
        runs.frame(runs.oracle, 15, image, three_d=False, png=False)
        runs.frame(runs.port, 5, image)
        # The oracle's hash in frames.tsv is enough without its picture
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--offset", "10"]), 0)
        # A frame the oracle didn't run fails
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--offset", "11"]), 1)

    def test_bad_runs(self):
        runs = Runs(self)
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, os.path.join(runs.root, "none")]), 2)
        with open(os.path.join(runs.oracle, "frames.tsv"), "w") as f:
            f.write("#pbwframes 1\nframe\tsomething\n")
        self.assertEqual(quiet(compare.main, ["align", runs.oracle, runs.port]), 2)


class AlignTests(unittest.TestCase):
    def make(self):
        runs = Runs(self)
        black = solid(b"\x00\x00\x00")
        pictures = []
        for i in range(4):
            image = bytearray(black)
            # A block big enough not to count as a stray line
            for y in range(10 * i, 10 * i + 20):
                for x in range(10 + i, 40 + i):
                    put(image, x, y, b"\xff\xff\xff")
            pictures.append(image)
        # The oracle boots 240 frames later, and lags once between its second and third pictures
        for frame in range(1, 260):
            runs.frame(runs.oracle, frame, black, counter=0, png=frame == 1)
        for i, frame in enumerate((260, 261, 263, 264)):
            runs.frame(runs.oracle, frame, pictures[i], counter=i + 1)
        for i, frame in enumerate((20, 21, 22, 23)):
            runs.frame(runs.port, frame, pictures[i], counter=i + 1)
        runs.frame(runs.port, 1, black, counter=0)
        return runs

    def test_first_change(self):
        runs = self.make()
        oracle_run = compare.load_run(runs.oracle)
        port_run = compare.load_run(runs.port)
        self.assertEqual(compare.first_change(oracle_run, complete=True), 260)
        self.assertEqual(compare.first_change(port_run), 20)
        # The port's run lacks frames 2-19, any of which could have been its first picture
        with self.assertRaises(compare.RunError):
            compare.first_change(port_run, complete=True)
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--align", "first-change"]), 2)

    def test_counter_and_image_offsets(self):
        runs = self.make()
        oracle_run = compare.load_run(runs.oracle)
        port_run = compare.load_run(runs.port)
        self.assertEqual(compare.counter_offsets(oracle_run, port_run), {1: 0, 20: 240, 21: 240, 22: 241, 23: 241})
        images = compare.image_offsets(oracle_run, port_run)
        self.assertEqual(images[22], [241])
        self.assertEqual(images[1][0], 0)
        # By the counter every pair matches
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--align", "counter"]), 0)
        status, out = captured(compare.main, ["align", runs.oracle, runs.port])
        self.assertEqual(status, 0)
        self.assertIn("port's run lacks frames", out)
        # With every port frame, by a single offset the lag shows
        black = solid(b"\x00\x00\x00")
        for frame in range(2, 20):
            runs.frame(runs.port, frame, black, counter=0, png=False)
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--align", "first-change",
                                              "--frames", "20-21"]), 0)
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--align", "first-change",
                                              "--frames", "20-23"]), 1)
        status, out = captured(compare.main, ["align", runs.oracle, runs.port])
        self.assertIn("oracle 260, port 20 (offset 240)", out)

    def test_sequence(self):
        runs = self.make()
        # Each port frame finds its picture, in order, whatever the lag
        mapping, notes = compare.sequence_mapping(compare.load_run(runs.oracle), compare.load_run(runs.port))
        self.assertEqual(mapping, {1: 1, 20: 260, 21: 261, 22: 263, 23: 264})
        self.assertEqual(notes, {})
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--align", "sequence"]), 0)
        self.assertEqual(compare.describe_ranges([1, 2, 3, 7, 9, 10]), "1-3, 7, 9-10")

    def test_sequence_extrapolates_and_depth(self):
        runs = Runs(self)
        black = solid(b"\x00\x00\x00")
        picture = bytearray(black)
        for y in range(40):
            for x in range(40):
                put(picture, x, y, b"\x80\x80\x80")
        # The port's picture differs from the oracle's in the low bit of the 6-bit output only (green's here)
        greener = bytearray(picture)
        greener[1::3] = bytes(v + 4 if v < 252 else v for v in greener[1::3])
        for frame in range(1, 8):
            runs.frame(runs.oracle, frame, black if frame < 5 else picture, three_d=False)
        runs.frame(runs.port, 2, black)
        runs.frame(runs.port, 3, greener)
        mapping, notes = compare.sequence_mapping(compare.load_run(runs.oracle), compare.load_run(runs.port))
        # Pictures pair at the palettes' depth, where the two are the same
        self.assertEqual(mapping, {2: 1, 3: 5})
        self.assertEqual(notes, {})
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--align", "sequence"]), 1)
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--align", "sequence",
                                              "--depth", "5"]), 0)
        # A port picture the oracle never shows pairs as far after the end of the last pair's picture (oracle frames
        # 5-7) as it is after the last pair (port frame 3)
        other = bytearray(black)
        for y in range(100, 140):
            for x in range(100, 140):
                put(other, x, y, b"\xff\x00\x00")
        runs.frame(runs.port, 6, other)
        mapping, notes = compare.sequence_mapping(compare.load_run(runs.oracle), compare.load_run(runs.port))
        self.assertEqual(mapping[6], 10)
        self.assertEqual(notes, {6: "extrapolated"})

    def test_sequence_extrapolates_by_the_counter(self):
        runs = Runs(self)
        black = solid(b"\x00\x00\x00")

        def scene(shade: int) -> bytearray:
            image = bytearray(black)
            for y in range(192, 300):
                for x in range(50, 200):
                    put(image, x, y, bytes([shade, 0, shade]))
            return image

        # A still that the oracle shows for ten frames over five passes of the main loop and the port for five frames
        # and passes; then pictures with 3D that differ between the two, one a pass, where the oracle drops a frame
        # in the middle (oracle frame 12 is the second of count 5)
        for frame, count in zip(range(1, 11), (0, 0, 1, 1, 2, 2, 3, 3, 4, 4)):
            runs.frame(runs.oracle, frame, black, counter=count, three_d=False, png=False)
        for frame, count, shade in ((11, 5, 40), (12, 5, 40), (13, 6, 80), (14, 7, 120)):
            runs.frame(runs.oracle, frame, scene(shade), counter=count, three_d=(False, True))
        for frame in range(1, 6):
            runs.frame(runs.port, frame, black, counter=frame - 1, png=False)
        for frame, count, shade in ((6, 5, 48), (7, 6, 88), (8, 7, 128)):
            runs.frame(runs.port, frame, scene(shade), counter=count)
        mapping, notes = compare.sequence_mapping(compare.load_run(runs.oracle), compare.load_run(runs.port))
        # The still pairs on the count (port frame 2, count 1, with oracle frame 3, the first of count 1); what follows
        # starts after the still's last frame on each (port 5, oracle 10), on the count: port frame 7 (count 6) is
        # oracle 13, not 12
        self.assertEqual(mapping, {1: 1, 2: 3, 3: 5, 4: 7, 5: 9, 6: 11, 7: 13, 8: 14})
        self.assertEqual(notes, {6: "extrapolated", 7: "extrapolated", 8: "extrapolated"})
        # Each pair's 3D screen differs by 8 over a block, 2 steps at 6 bits and 1 at 5: no exact match, but within
        # the pixel tolerance
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--align", "sequence"]), 0)
        # Without the port's counter, frames count instead: port frame 7 is oracle 12
        for row in runs.rows[runs.port]:
            row.counter = None
        compare.write_frames_tsv(os.path.join(runs.port, "frames.tsv"), runs.rows[runs.port])
        mapping, _ = compare.sequence_mapping(compare.load_run(runs.oracle), compare.load_run(runs.port))
        self.assertEqual((mapping[6], mapping[7], mapping[8]), (11, 12, 13))


    def test_sequence_takes_the_nearest_match(self):
        runs = Runs(self)
        black = solid(b"\x00\x00\x00")
        white = solid(b"\xfc\xfc\xfc")

        def scene(shade: int) -> bytearray:
            image = bytearray(black)
            for y in range(0, 100):
                for x in range(50, 200):
                    put(image, x, y, bytes([0, shade, 0]))
            return image

        # A flash of white, pictures with 3D that never match exactly, then a fade to white: the port's white after
        # the 3D is the oracle's second white, not the flash, though the flash comes first after the last exact pair
        for frame, image in enumerate([black, white] + [scene(40 * i) for i in range(1, 5)] + [white, black], 1):
            runs.frame(runs.oracle, frame, image, counter=frame - 1, three_d=(True, False))
        for frame, image in enumerate([black, white] + [scene(40 * i + 8) for i in range(1, 5)] + [white, black], 1):
            runs.frame(runs.port, frame, image, counter=frame - 1)
        mapping, notes = compare.sequence_mapping(compare.load_run(runs.oracle), compare.load_run(runs.port))
        self.assertEqual(mapping, {frame: frame for frame in range(1, 9)})
        self.assertEqual(notes, {frame: "extrapolated" for frame in range(3, 7)})
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--align", "sequence"]), 0)


class TimedAlignTests(unittest.TestCase):
    """--align counter with a clock stretch, a lag and the check for pictures no port frame shows, on a made-up boot:
    a start-up, two still pictures while the game counts passes, a movie that runs on a clock, black, then a title
    with 3D"""

    @staticmethod
    def picture(index: int) -> bytearray:
        image = solid(b"\x00\x00\x00")
        if index:
            for y in range(4 * index, 4 * index + 20):
                for x in range(10, 60):
                    put(image, x, y, bytes([40 * index, 255 - 40 * index, 128]))
        return image

    @staticmethod
    def scene(shade: int) -> bytearray:
        image = solid(b"\x00\x00\x00")
        for y in range(192, 300):
            for x in range(50, 200):
                put(image, x, y, bytes([shade, 0, shade]))
        return image

    # The oracle (frame: picture, count): start-up white at count 0 for two frames, A at counts 1 and 2, B for one
    # pass of two frames, the movie's three pictures for three frames and one pass each, black for two passes, then
    # the title's 3D. The port counts from its first frame, its pass at B takes one frame, and it shows each movie
    # picture for two frames and two passes, so that after the movie its count is 3 ahead
    ORACLE = [("W", 0), ("W", 0), ("A", 1), ("A", 2), ("B", 3), ("B", 3), ("M1", 4), ("M1", 4), ("M1", 4),
              ("M2", 5), ("M2", 5), ("M2", 5), ("M3", 6), ("M3", 6), ("M3", 6), ("K", 7), ("K", 8), ("S1", 9),
              ("S2", 10)]
    PORT = ["W", "A", "A", "B", "M1", "M1", "M2", "M2", "M3", "M3", "K", "K", "S1", "S2"]

    def make(self, oracle_pictures=None, port_pictures=None):
        runs = Runs(self)
        images = {"W": solid(b"\xff\xff\xff"), "K": solid(b"\x00\x00\x00"), "A": self.picture(1),
                  "B": self.picture(2), "M1": self.picture(3), "M2": self.picture(4), "M3": self.picture(5),
                  "X": self.picture(6), "S1": self.scene(40), "S2": self.scene(80), "S1'": self.scene(48),
                  "S2'": self.scene(88)}
        for frame, (name, count) in enumerate(oracle_pictures or self.ORACLE, 1):
            runs.frame(runs.oracle, frame, images[name], counter=count, three_d=(False, name.startswith("S")))
        # The port's 3D differs from the oracle's by 2 steps at 6 bits: within the tolerance, never exact
        for frame, name in enumerate(port_pictures or self.PORT, 1):
            runs.frame(runs.port, frame, images[name + "'" if name.startswith("S") else name], counter=frame - 1)
        return runs

    def test_counts_and_a_clock(self):
        runs = self.make()
        pairing = compare.counter_mapping(compare.load_run(runs.oracle), compare.load_run(runs.port), set(range(5, 11)))
        # Frames 1-4 on the count with the lag 0 (port frame 4's count 3 is oracle frame 5, the first of its pass);
        # the movie by its pictures, each port frame with the oracle frame of its picture nearest where it is expected;
        # after it, the lag where port frame 11 is expected after the end of M3 on each (port 10, oracle 15): count 7,
        # oracle frame 16, so the port's count 10 less 7, 3
        self.assertEqual(pairing.mapping, {1: 1, 2: 3, 3: 4, 4: 5, 5: 7, 6: 9, 7: 10, 8: 12, 9: 13, 10: 15, 11: 16,
                                           12: 17, 13: 18, 14: 19})
        self.assertEqual(pairing.timing, [
            "port frames 1-4: by the counter, lag 0 from power-on",
            "port frames 5-10: by pictures (the game waits on a clock)",
            "port frames 11-14: by the counter, lag 3, from where the frame is expected after the end of the last "
            "picture on each"])
        self.assertEqual(pairing.breaks, set())
        self.assertEqual(compare.unshown_frames(compare.load_run(runs.oracle), pairing.mapping, (1, 14)), [])
        status, out = captured(compare.main, ["screens", runs.oracle, runs.port, "--align", "counter", "--clock",
                                              "5-10"])
        self.assertEqual(status, 0, out)
        self.assertIn("14 of 14 frames match (12 exact, 2 with 3D within the tolerance)", out)
        # Declaring the lag after the clock gives the same pairs, and the frame a break
        pairing = compare.counter_mapping(compare.load_run(runs.oracle), compare.load_run(runs.port), set(range(5, 11)),
                                          {11: 3})
        self.assertEqual(pairing.mapping[11], 16)
        self.assertEqual(pairing.breaks, {11})
        self.assertIn("port frames 11-14: by the counter, lag 3, declared where the counts line up again after the "
                      "clock (oracle frames since the last pair: none, not checked)", pairing.timing)
        # Without the clock, the movie's frames pair on the count and the port's second frame of M1 (count 5) meets
        # the oracle's M2
        status, out = captured(compare.main, ["screens", runs.oracle, runs.port, "--align", "counter"])
        self.assertEqual(status, 1)
        self.assertIn("     6     10  -       FAIL", out)
        # --clock and --lag go with --align counter only
        with self.assertRaises(SystemExit):
            quiet(compare.main, ["screens", runs.oracle, runs.port, "--align", "sequence", "--clock", "5-10"])

    def test_late_and_skipped_pictures_fail(self):
        # A held a pass longer, so that B and all after it come a pass late: by pictures in order it passes, on the
        # count it fails where the game counts passes (port frame 4, count 3, against oracle frame 5's B)
        late = self.PORT[:3] + ["A"] + self.PORT[3:]
        runs = self.make(port_pictures=late)
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--align", "sequence"]), 0)
        status, out = captured(compare.main, ["screens", runs.oracle, runs.port, "--align", "counter", "--clock",
                                              "6-11"])
        self.assertEqual(status, 1)
        self.assertIn("     4      5  -       FAIL", out)
        # M2 skipped in the movie: every port frame finds its picture, but no port frame shows oracle frames 10-12
        skipped = list(self.PORT)
        skipped[6:8] = ["M3", "M3"]
        runs = self.make(port_pictures=skipped)
        for align in (["--align", "sequence"], ["--align", "counter", "--clock", "5-10"]):
            status, out = captured(compare.main, ["screens", runs.oracle, runs.port, *align])
            self.assertEqual(status, 1, align)
            self.assertIn("FAIL: oracle frames 10-12 (count 5): no port frame shows their pictures", out)
            self.assertIn("14 of 14 frames match", out)

    def test_a_torn_frame_and_known_oracle_frames(self):
        # The DS's pass at B takes two frames, and a register written partway through the second shows X: a picture
        # neither program's neighbouring pairs show
        torn = list(self.ORACLE)
        torn[5] = ("X", 3)
        runs = self.make(oracle_pictures=torn)
        status, out = captured(compare.main, ["screens", runs.oracle, runs.port, "--align", "counter", "--clock",
                                              "5-10"])
        self.assertEqual(status, 1)
        self.assertIn("FAIL: oracle frame 6 (count 3): no port frame shows its pictures", out)
        self.assertEqual(quiet(compare.main, ["screens", runs.oracle, runs.port, "--align", "counter", "--clock",
                                              "5-10", "--known-oracle", "6"]), 0)
        # Only between consecutive port frames within the frames compared, and not across a break
        oracle_run = compare.load_run(runs.oracle)
        mapping = {4: 5, 5: 7}
        self.assertEqual(compare.unshown_frames(oracle_run, mapping, (4, 5)), [6])
        self.assertEqual(compare.unshown_frames(oracle_run, mapping, (5, 5)), [])
        self.assertEqual(compare.unshown_frames(oracle_run, mapping, (4, 5), breaks={5}), [])
        self.assertEqual(compare.unshown_frames(oracle_run, {3: 5, 5: 7}, (3, 5)), [])

    def test_lags(self):
        self.assertEqual(compare.parse_lags("1109=0,3241=-1"), {1109: 0, 3241: -1})
        self.assertEqual(compare.parse_lags(None), {})
        with self.assertRaises(ValueError):
            compare.parse_lags("1109")
        # A declared lag within the counted frames: from port frame 3 on, the port's count is the oracle's + 1, so
        # port frame 3 (count 2) is oracle frame 3 (count 1), and port frame 4 (count 3) is oracle frame 4 (count 2)
        runs = self.make()
        pairing = compare.counter_mapping(compare.load_run(runs.oracle), compare.load_run(runs.port), set(), {3: 1})
        self.assertEqual((pairing.mapping[2], pairing.mapping[3], pairing.mapping[4]), (3, 3, 4))
        self.assertEqual(pairing.breaks, set())


class MemoryTests(unittest.TestCase):
    def test_memory(self):
        runs = Runs(self)
        runs.memory(runs.oracle, ["10 sVBlankCount 0x0209da64 4 05000000", "10 sWork 0x02100000 2 abcd",
                                  "12 sVBlankCount 0x0209da64 4 06000000"])
        runs.memory(runs.port, ["10 sVBlankCount 0x0209da64 4 05000000", "10 sWork 0x02100000 2 abce"])
        self.assertEqual(quiet(compare.main, ["memory", runs.oracle, runs.port, "--labels", "sVBlankCount"]), 0)
        self.assertEqual(quiet(compare.main, ["memory", runs.oracle, runs.port]), 1)
        self.assertEqual(quiet(compare.main, ["memory", runs.oracle, runs.port, "--offset", "2",
                                              "--labels", "sVBlankCount"]), 1)
        self.assertEqual(quiet(compare.main, ["memory", runs.oracle, runs.port, "--labels", "nothing"]), 2)

    def test_bad_memory_file(self):
        runs = Runs(self)
        runs.memory(runs.port, ["10 sVBlankCount 0x0209da64 4 0500"])
        self.assertEqual(quiet(compare.main, ["memory", runs.oracle, runs.port]), 2)


class SymbolTests(unittest.TestCase):
    MAP = [
        "# Link map of Entry\r\n",
        "\r\n",
        "# .arm9\r\n",
        "#>0209D740          ARM9_BSS_START (linker command file)\r\n",
        "  0209DA58 00000000 .bss    .bss\t(gfl_use.o)\r\n",
        "  0209DA58 00000004 .bss    sVBlankCallback\t(gfl_use.o)\r\n",
        "  0209DA64 00000004 .bss    sVBlankCount\t(gfl_use.o)\r\n",
        "  0209DA68 00000004 .bss    sWork\t(gfl_use.o)\r\n",
        "  02000000 00000000 .text   $t\t(gfl_use.o)\r\n",
        "\r\n",
        "# .ov033\r\n",
        "  021A0000 00000010 .bss    sWork\t(field.o)\r\n",
        "  021A0010 00000008 .data   @1234\t(field.o)\r\n",
    ]

    def test_map(self):
        mapped = symbols.parse_map(self.MAP)
        self.assertEqual([(s.name, s.module, s.address, s.size, s.file) for s in mapped], [
            ("sVBlankCallback", "main", 0x0209DA58, 4, "gfl_use.c"),
            ("sVBlankCount", "main", 0x0209DA64, 4, "gfl_use.c"),
            ("sWork", "main", 0x0209DA68, 4, "gfl_use.c"),
            ("sWork", "ov033", 0x021A0000, 0x10, "field.c"),
        ])

    def test_find(self):
        config = [
            symbols.Symbol("sVBlankCallback", "main", 0x0209DA58, 0),
            symbols.Symbol("data_x", "main", 0x0209DA70, 0),
            symbols.Symbol("func", "main", 0x02000000, 0x24, sized=True),
        ]
        everything = config + symbols.parse_map(self.MAP)
        symbols.finish(everything)
        # The same symbol in symbols.txt and the map counts once, with the map's size
        self.assertEqual(symbols.find(everything, "sVBlankCallback").size, 4)
        self.assertEqual(symbols.find(everything, "func").size, 0x24)
        # A symbol without a size runs to the next one (here, none: a word)
        self.assertEqual(symbols.find(everything, "data_x").size, 4)
        with self.assertRaises(KeyError):
            symbols.find(everything, "sWork")
        self.assertEqual(symbols.find(everything, "ov033/sWork").address, 0x021A0000)
        self.assertEqual(symbols.find(everything, "gfl_use.c/sWork").address, 0x0209DA68)
        with self.assertRaises(KeyError):
            symbols.find(everything, "nothing")

    def test_statics_format(self):
        mapped = symbols.parse_map(self.MAP)
        with tempfile.TemporaryDirectory() as d:
            path = os.path.join(d, "statics.txt")
            with open(path, "w") as f:
                symbols.write_statics(mapped, f)
            with open(path) as f:
                lines = [line for line in f if not line.startswith("#")]
        self.assertIn("sVBlankCount kind:map addr:0x0209da64 size:0x4 module:main file:gfl_use.c\n", lines)


FAKE_PORT = """#!{python}
# A stand-in for the port's program: a run directory (--run-dir) with a row for each frame and, for the frames of
# --run-screens, a picture of the frame's number; it stops at frame 50 as a trap stub would. It records its arguments
import os
import sys
sys.path.insert(0, {tools!r})
import compare
args = sys.argv[1:]
with open({record!r}, "w") as f:
    f.write("\\n".join(args))
frames = int(args[args.index("--frames") + 1])
out = args[args.index("--run-dir") + 1]
listed = args[args.index("--run-screens") + 1]
wanted = set() if listed == "none" else None if listed == "all" else set(compare.parse_frames(listed))
rows = []
for frame in range(1, frames + 1):
    image = bytearray(b"\\x00\\x00\\x00" * (256 * 384))
    image[0:3] = bytes([min(frame, 255), 0, 0])
    top, bottom = compare.screen_hashes(bytes(image))
    rows.append(compare.Frame(frame, frame, frame - 1, frame, None, None, None, top, bottom, False, False))
    compare.write_frames_tsv(os.path.join(out, "frames.tsv"), rows)
    if wanted is None or frame in wanted:
        compare.write_png(os.path.join(out, compare.FRAME_FILE.format(frame)), 256, 384, bytes(image))
    if frame == 50:
        sys.stderr.write("\\nport: the game called func_x (main, 0x02000000), which has no C yet\\n")
        sys.exit(70)
sys.stderr.write("2026-10-17 12:00:00.000 fakeport[1:2] port: stopped after %d frames\\n" % frames)
"""


def fake_port(directory: str) -> tuple[str, str]:
    """The stand-in program, and the file where it records its arguments"""
    program = os.path.join(directory, "fakeport")
    record = os.path.join(directory, "arguments")
    with open(program, "w") as f:
        f.write(FAKE_PORT.format(python=sys.executable, tools=os.path.abspath(TOOLS), record=record))
    os.chmod(program, os.stat(program).st_mode | stat.S_IEXEC)
    return program, record


class OracleScriptTests(unittest.TestCase):
    def test_port_runs(self):
        with tempfile.TemporaryDirectory() as d:
            program, record = fake_port(d)
            out = os.path.join(d, "run")
            os.makedirs(out)
            # A picture from another run doesn't stay
            compare.write_png(os.path.join(out, "frame_000005.png"), 1, 1, b"\x00\x00\x00")
            status, message = oracle.run_port(program, out, 40, "1,10,60", "rom.nds", oracle.DEFAULT_TIME)
            self.assertEqual((status, message), (0, "port: stopped after 40 frames"))
            run = compare.load_run(out)
            self.assertEqual(sorted(run.frames), list(range(1, 41)))
            self.assertEqual(run.captured(), [1, 10])
            self.assertFalse([name for name in os.listdir(out) if name.startswith("saves-")])
            with open(record) as f:
                arguments = f.read().split("\n")
            self.assertEqual(arguments[arguments.index("--fixed-time") + 1], oracle.DEFAULT_TIME)
            self.assertEqual(arguments[arguments.index("--run-screens") + 1], "1,10,60")
            # One that a trap stub ends keeps the frames before it, and says why it stopped
            status, message = oracle.run_port(program, out, 60, "all", "rom.nds", None, "x.pbwinput")
            self.assertEqual(status, 70)
            self.assertIn("has no C yet", message)
            self.assertEqual(max(compare.load_run(out).frames), 50)
            self.assertIn("exit 70 after 50 of 60 frames", oracle.describe_port_run(status, message, 60, out))
            with open(record) as f:
                arguments = f.read().split("\n")
            # Without --time, an input file's time line holds
            self.assertNotIn("--fixed-time", arguments)
            self.assertEqual(arguments[arguments.index("--input") + 1], "x.pbwinput")
            status, out_text = captured(oracle.main, ["port", "--binary", program, "--out", out, "--frames", "1,10,60",
                                                      "--dump", "NNS_G3dGlb:612@10", "--dump", "s=*SNDi_SharedWork:16"])
            self.assertEqual(status, 1)
            self.assertIn("no picture of frames 60", out_text)
            # Each --dump is one of the program's --run-dump
            with open(record) as f:
                arguments = f.read().split("\n")
            self.assertEqual([arguments[i + 1] for i, a in enumerate(arguments) if a == "--run-dump"],
                             ["NNS_G3dGlb:612@10", "s=*SNDi_SharedWork:16"])

    def test_run_passes_the_time_only_when_given(self):
        with tempfile.TemporaryDirectory() as d:
            # A stand-in for pbw-oracle that records its arguments
            program = os.path.join(d, "pbw-oracle")
            with open(program, "w") as f:
                f.write(f"#!{sys.executable}\nimport sys\nopen({os.path.join(d, 'args')!r}, 'w').write("
                        "'\\n'.join(sys.argv[1:]))\n")
            os.chmod(program, os.stat(program).st_mode | stat.S_IEXEC)
            out = os.path.join(d, "run")
            for given, expected in ((None, None), ("2013-05-04 08:30:00", "2013-05-04 08:30:00")):
                command = ["run", "--oracle", program, "--out", out, "--frames", "1", "--input", "x.pbwinput",
                           "--no-counter"]
                if given:
                    command += ["--time", given]
                self.assertEqual(quiet(oracle.main, command), 0)
                with open(os.path.join(d, "args")) as f:
                    arguments = f.read().split("\n")
                if expected is None:
                    self.assertNotIn("--time", arguments)
                else:
                    self.assertEqual(arguments[arguments.index("--time") + 1], expected)

    def test_sequence_reference(self):
        runs = Runs(self)
        black = solid(b"\x00\x00\x00")
        white = solid(b"\xfc\xfc\xfc")
        movie = solid(b"\x80\x00\x00")
        scene = bytearray(black)
        for y in range(192, 240):
            for x in range(10, 60):
                put(scene, x, y, b"\x40\x80\xc0")
        # The oracle shows black for longer (its count waits at 0), a frame of a movie on a clock for two frames and
        # one pass, then white and a 3D bottom screen; the port shows the movie frame for two passes
        for frame in range(1, 6):
            runs.frame(runs.oracle, frame, black, counter=0, three_d=False)
        runs.frame(runs.oracle, 6, movie, counter=1, three_d=False)
        runs.frame(runs.oracle, 7, movie, counter=1, three_d=False)
        runs.frame(runs.oracle, 8, white, counter=2, three_d=False)
        runs.frame(runs.oracle, 9, scene, counter=3, three_d=(False, True))
        for frame, image in enumerate((black, movie, movie, white, scene), 1):
            runs.frame(runs.port, frame, image, counter=frame - 1)
        saved = dict(oracle.SEQUENCES)
        oracle.SEQUENCES["test"] = oracle.Sequence(5, 9, "1-5", "2-3", known={"b2_us": {"4": "a known difference"}})
        try:
            path = os.path.join(runs.root, "test.tsv")
            rows = oracle.write_reference(path, "test", "b2_us", compare.load_run(runs.oracle),
                                          compare.load_run(runs.port))
        finally:
            oracle.SEQUENCES.clear()
            oracle.SEQUENCES.update(saved)
        # Port frame 1 pairs on the count (lag 0); 2 and 3, in the clock stretch, with the movie's oracle frames by
        # their pictures (lag -); 4, left out, and 5 on the count after the clock, with the lag where 4 is expected
        # after the end of the movie on each (port 3 at count 2, oracle 7 at count 1): the oracle's count 2, so the
        # lag is 1, and 5 (count 4) is oracle frame 9 (count 3)
        self.assertEqual(rows, 4)
        black_top, black_bottom = compare.screen_hashes(bytes(black))
        movie_top, movie_bottom = compare.screen_hashes(bytes(movie))
        top, _ = compare.screen_hashes(bytes(scene))
        reference = oracle.load_reference(path)
        self.assertEqual(reference, {1: (1, 0, 0, black_top, black_bottom),
                                     2: (6, 1, None, movie_top, movie_bottom),
                                     3: (7, 1, None, movie_top, movie_bottom),
                                     5: (9, 3, 1, top, "3d")})
        with open(path) as f:
            self.assertIn("# Left out, port frames 4: a known difference\n", f.read())
        # The port's run against it: as it is, nothing fails; a port a pass late fails on the count
        failures, compared, left, timed = oracle.check_reference(reference, compare.load_run(runs.port))
        self.assertEqual((failures, compared, left, timed), ([], 7, 1, 2))
        port_run = compare.load_run(runs.port)
        port_run.frames[5].counter = 5
        failures, _, _, _ = oracle.check_reference(reference, port_run)
        self.assertEqual(failures, ["port frame 5 is at count 5, not 4 (oracle frame 9's count 3 and the lag 1)"])
        port_run.frames[1].top = "0" * 40
        failures, _, _, _ = oracle.check_reference(reference, port_run)
        self.assertIn("port frame 1's top screen isn't oracle frame 1's", failures)

    def test_sequences_skip_without_the_rom_or_the_oracle(self):
        with tempfile.TemporaryDirectory() as d:
            for name in oracle.SEQUENCES:
                status = quiet(oracle.main, ["sequence-check", name, "--port", os.path.join(d, "port"), "--rom",
                                             os.path.join(d, "missing.nds")])
                self.assertEqual(status, oracle.SKIP)
                status = quiet(oracle.main, ["sequence", name, "--port", os.path.join(d, "port"), "--oracle",
                                             os.path.join(d, "missing")])
                self.assertEqual(status, oracle.SKIP)

    def test_sequences_declare_what_they_leave_out(self):
        # Every known difference is a frame list compare.py reads, within the frames each program runs (the state's
        # within the frames whose state is compared), lags are for versions the repository builds, and every sequence
        # has an input file for each version
        for name, sequence in oracle.SEQUENCES.items():
            self.assertTrue(set(compare.parse_frames(sequence.clock)) <= set(range(1, sequence.port_frames + 1)))
            for version, lags in sequence.lags.items():
                self.assertIn(version, ("b2_us", "w2_us"))
                self.assertTrue(set(compare.parse_lags(lags)) <= set(range(1, sequence.port_frames + 1)))
            for version, table in sequence.known.items():
                self.assertTrue(set(oracle.by_frame(table)) <= set(range(1, sequence.port_frames + 1)), name)
            for version, table in sequence.known_oracle.items():
                self.assertTrue(set(oracle.by_frame(table)) <= set(range(1, sequence.oracle_frames + 1)), name)
            if sequence.state_frames:
                state = set(compare.parse_frames(sequence.state_frames))
                self.assertTrue(state <= set(range(1, sequence.port_frames + 1)), name)
                self.assertTrue(set(compare.parse_frames(sequence.state_oracle)) <=
                                set(range(1, sequence.oracle_frames + 1)), name)
                for version, table in sequence.known_state.items():
                    self.assertTrue(set(oracle.by_frame(table)) <= state, name)
            for version in ("b2_us", "w2_us"):
                self.assertTrue(os.path.exists(os.path.join(oracle.REPO_ROOT, oracle.SEQUENCE_INPUT.format(
                    name=name, version=version))), (name, version))

    def test_check_skips_without_the_oracle(self):
        with tempfile.TemporaryDirectory() as d:
            status = quiet(oracle.main, ["check", "--oracle", os.path.join(d, "missing")])
        self.assertEqual(status, oracle.SKIP)

    def test_check_skips_without_the_rom(self):
        with tempfile.TemporaryDirectory() as d:
            fake = os.path.join(d, "pbw-oracle")
            open(fake, "w").close()
            status = quiet(oracle.main, ["check", "--oracle", fake, "--rom", os.path.join(d, "missing.nds")])
        self.assertEqual(status, oracle.SKIP)


if __name__ == "__main__":
    unittest.main(verbosity=1)

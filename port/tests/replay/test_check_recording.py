#!/usr/bin/env python3
"""ROM-free regressions for check_recording.py's evidence checks, using real run-directory parsers.

Synthetic counter/screen/seed/state mismatches must fail rather than turn a successful process exit into a false
record/replay pass. Native addresses may differ across processes; the requested payloads may not.
"""

import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("check_recording", HERE / "check_recording.py")
recording = importlib.util.module_from_spec(spec)
spec.loader.exec_module(recording)


class RecordingChecks(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="recording-checks-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.paths = [self.root / name for name in ("record", "replay")]
        self.frames = 1116
        for index, path in enumerate(self.paths):
            path.mkdir()
            (path / "saves").mkdir()
            frames = [recording.compare.Frame(frame, frame, frame, frame + 1, None, None, None,
                                              "top", "bottom", False, False)
                      for frame in range(1, self.frames + 1)]
            recording.compare.write_frames_tsv(str(path / "frames.tsv"), frames)
            (path / "run.txt").write_text("time: 2012-10-07 12:00:00\nclock: running\nseeds: 1\n"
                                          "seed_1: 0x0000000000000001 given\ncompleted: 1116\nstatus: 0\n")
            (path / "memory.txt").write_text("#pbwmem 1\n" + "".join(
                f"1116 {label} {0x100000000 + index * 0x2000:#x} {size} {'00112233' * (size // 4)}\n"
                for label, size in (("NNS_G3dGlb", 612), ("gfl_lc", 8), ("gfl_mt", 4), ("gfl_mt_index", 4))))

    def compare(self):
        return recording.compare_runs(*self.paths, self.frames)

    def edit(self, name, old, new):
        path = self.paths[1] / name
        path.write_text(path.read_text().replace(old, new))

    def test_payloads_match_despite_aslr(self):
        result = self.compare()
        self.assertEqual(result["frames_equal"], 1116)
        self.assertEqual(result["state_payloads_equal"], 4)

    def test_changed_screen_fails(self):
        self.edit("frames.tsv", "\ttop\tbottom\t0", "\tdifferent\tbottom\t0")
        with self.assertRaisesRegex(ValueError, "frame 1"):
            self.compare()

    def test_changed_counter_fails(self):
        self.edit("frames.tsv", "701\t701\t701\t", "701\t701\t700\t")
        with self.assertRaisesRegex(ValueError, "frame 701"):
            self.compare()

    def test_missing_frame_fails(self):
        path = self.paths[1] / "frames.tsv"
        path.write_text("\n".join(path.read_text().splitlines()[:-1]) + "\n")
        with self.assertRaisesRegex(ValueError, "every requested frame"):
            self.compare()

    def test_missing_state_fails(self):
        path = self.paths[1] / "memory.txt"
        path.write_text("\n".join(path.read_text().splitlines()[:-1]) + "\n")
        with self.assertRaisesRegex(ValueError, "all requested state"):
            self.compare()

    def test_changed_random_payload_fails(self):
        self.edit("memory.txt", "00112233", "00112234")
        with self.assertRaisesRegex(ValueError, "state differs"):
            self.compare()

    def test_identically_short_state_still_fails(self):
        for path in self.paths:
            memory = path / "memory.txt"
            memory.write_text(memory.read_text().replace(" 8 0011223300112233", " 4 00112233"))
        with self.assertRaisesRegex(ValueError, "wrong size"):
            self.compare()

    def test_undeclared_seed_fails(self):
        for path in self.paths:
            info = path / "run.txt"
            info.write_text(info.read_text().replace(" given", " built"))
        with self.assertRaisesRegex(ValueError, "undeclared seed"):
            self.compare()

    def test_different_clock_fails(self):
        self.edit("run.txt", "clock: running", "clock: pinned")
        with self.assertRaisesRegex(ValueError, "clock"):
            self.compare()

    def test_identically_missing_seed_request_fails(self):
        for path in self.paths:
            info = path / "run.txt"
            info.write_text(info.read_text().replace("seeds: 1", "seeds: 2"))
        with self.assertRaisesRegex(ValueError, "every request"):
            self.compare()

    def test_initial_and_video_header_required(self):
        path = self.root / "recorded.pbwinput"
        header = "pbwinput 1\nversion b2_us\nframes video\ntime 2012-10-07 12:00:00\nclock running\nseed 0x1\n"
        info = {"effective_time": "2012-10-07 12:00:00", "clock": "running"}
        path.write_text(header)
        with self.assertRaisesRegex(ValueError, "initial"):
            recording.check_header(path, "b2_us", info)
        path.write_text(header + "initial - -\n")
        self.assertEqual(recording.check_header(path, "b2_us", info)["declared_seeds"], 1)
        path.write_text((header + "initial - -\n").replace("frames video", "frames game"))
        with self.assertRaisesRegex(ValueError, "frames"):
            recording.check_header(path, "b2_us", info)

    def test_no_required_save_fails(self):
        with self.assertRaisesRegex(ValueError, "no required save"):
            recording.check_saves(*self.paths, True, None)

    def test_identically_corrupt_save_still_fails(self):
        for path in self.paths:
            (path / "saves" / "IREO.sav").write_bytes(b"\x00" * recording.saveimage.BACKUP_SIZE)
        with self.assertRaisesRegex(ValueError, "IREO.sav"):
            recording.check_saves(*self.paths, True, None)

    def test_unfinished_save_fails(self):
        (self.paths[0] / "saves" / "IREO.sav.tmp").write_bytes(b"unfinished")
        with self.assertRaisesRegex(ValueError, "unfinished save"):
            recording.check_saves(*self.paths, False, None)

    def test_missing_rom_has_real_skip_status(self):
        done = subprocess.run([sys.executable, str(HERE / "check_recording.py"), "--version", "b2_us",
                               "--binary", str(self.root / "unused-program"), "--rom", str(self.root / "absent.nds")],
                              capture_output=True, text=True)
        self.assertEqual(done.returncode, 66)
        self.assertIn("SKIP: no ROM", done.stdout)


if __name__ == "__main__":
    unittest.main()

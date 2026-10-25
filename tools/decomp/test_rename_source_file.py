#!/usr/bin/env python3
"""Tests of rename_source_file.py's planning."""
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.rename_source_file import plan  # noqa: E402

ENTRY = "lib/msl/src/fp/fp_fastI.c:\n    complete\n    .text       start:0x0208c224 end:0x0208ecd8\n"


class PlanTest(unittest.TestCase):
    def config(self, root: Path, version: str, text: str):
        path = root / "config" / version / "arm9" / "delinks.txt"
        path.parent.mkdir(parents=True)
        path.write_text(text)
        return path

    def test_renames_every_version_and_keeps_the_entry(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            b2 = self.config(root, "b2_us", ENTRY)
            w2 = self.config(root, "w2_us", ENTRY)
            planned = dict(plan("lib/msl/src/fp/fp_fastI.c", "lib/msl/src/fp/fp_fastI.s", root))
            self.assertEqual(set(planned), {b2, w2})
            self.assertEqual(planned[b2], ENTRY.replace("fp_fastI.c:", "fp_fastI.s:"))

    def test_refuses_a_new_path_already_registered(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.config(root, "b2_us", ENTRY + ENTRY.replace("fp_fastI.c", "fp_fastI.s"))
            with self.assertRaises(ValueError):
                plan("lib/msl/src/fp/fp_fastI.c", "lib/msl/src/fp/fp_fastI.s", root)

    def test_refuses_an_unregistered_path(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.config(root, "b2_us", ENTRY)
            with self.assertRaises(ValueError):
                plan("lib/msl/src/fp/other.c", "lib/msl/src/fp/other.s", root)

    def test_renames_the_path_in_section_fixes(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.config(root, "b2_us", ENTRY)
            fixes = root / "config" / "fixes.txt"
            fixes.write_text("# header\n"
                             ". 0x0208ecd8 section_end lib/msl/src/fp/fp_fastI.c:.text\n"
                             ". 0x0208ecd8 add_section lib/msl/src/fp/fp_fastI.c:.bss:0x4\n"
                             ". 0x0208ecd8 section_end lib/msl/src/fp/fp_fastI.cc:.text\n"
                             ". 0x0208ecd8 add_label lib/msl/src/fp/fp_fastI.c:x\n")
            planned = dict(plan("lib/msl/src/fp/fp_fastI.c", "lib/msl/src/fp/fp_fastI.s", root))
            self.assertEqual(planned[fixes].splitlines(), [
                "# header",
                ". 0x0208ecd8 section_end lib/msl/src/fp/fp_fastI.s:.text",
                ". 0x0208ecd8 add_section lib/msl/src/fp/fp_fastI.s:.bss:0x4",
                ". 0x0208ecd8 section_end lib/msl/src/fp/fp_fastI.cc:.text",
                ". 0x0208ecd8 add_label lib/msl/src/fp/fp_fastI.c:x",
            ])

    def test_does_not_rename_a_path_that_only_contains_the_old_one(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self.config(root, "b2_us", "src/a.c:\n    .text       start:0x1 end:0x2\n" + ENTRY)
            planned = dict(plan("src/a.c", "src/a.s", root))
            text = next(iter(planned.values()))
            self.assertIn("src/a.s:", text)
            self.assertIn("lib/msl/src/fp/fp_fastI.c:", text)


if __name__ == "__main__":
    unittest.main()

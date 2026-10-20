"""Boundary-checker regressions using temporary configs, without ROMs or generated build files.

    python3 -m unittest tools.decomp.test_check_section_bounds
"""
import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.decomp import check_section_bounds as bounds


class SectionBoundsTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        root_patch = patch.object(bounds, "ROOT", self.root)
        root_patch.start()
        self.addCleanup(root_patch.stop)

    def configs(self, sections, symbols, owners):
        header = "\n".join(
            f"    {name} start:0x{start:x} end:0x{end:x} kind:{kind} align:4"
            for name, start, end, kind in sections
        )
        for version in ("b2_us", "w2_us"):
            directory = self.root / "config" / version / "arm9" / bounds.MODULE
            directory.mkdir(parents=True, exist_ok=True)
            (directory / "delinks.txt").write_text(header + "\n\n" + owners)
            (directory / "symbols.txt").write_text(symbols)

    def test_unsized_last_rodata_stops_before_alignment_gap(self):
        self.configs(
            [(".rodata", 0x100, 0x120, "rodata"), (".data", 0x140, 0x160, "data")],
            "table kind:data(any) addr:0x100\nvalue kind:data(any) addr:0x140\n",
            "src/ov036/a.c:\n    .rodata start:0x100 end:0x120\n"
            "src/ov036/b.c:\n    .data start:0x140 end:0x160\n",
        )
        for version in ("b2_us", "w2_us"):
            with self.subTest(version=version):
                self.assertEqual(bounds.problems(version), [])
                self.assertIn((0x100, 0x120), bounds.objects(version, bounds.module_sections(version)))

    def test_last_bss_uses_module_end_despite_file_order(self):
        self.configs(
            [(".bss", 0x180, 0x1C0, "bss")],
            "head kind:bss(size=0x20) addr:0x180\ntail kind:bss addr:0x1a0\n",
            "src/ov036/tail.c:\n    .bss start:0x1a0 end:0x1c0\n"
            "src/ov036/head.c:\n    .bss start:0x180 end:0x1a0\n",
        )
        self.assertEqual(bounds.module_sections("b2_us"), {".bss": (0x180, 0x1C0)})
        self.assertIn((0x1A0, 0x1C0), bounds.objects("b2_us", bounds.module_sections("b2_us")))
        self.assertEqual(bounds.problems("b2_us"), [])

    def test_mixed_size_aliases_share_an_address(self):
        aliases = ["table kind:data(any,size=0x20) addr:0x100\n", "alias kind:data(any) addr:0x100\n"]
        for entries in (aliases, list(reversed(aliases))):
            with self.subTest(entries=entries):
                self.configs(
                    [(".rodata", 0x100, 0x140, "rodata"), (".ctor", 0x140, 0x140, "rodata")],
                    "".join(entries) + "next kind:data(any) addr:0x120\n",
                    "src/ov036/a.c:\n    .rodata start:0x100 end:0x120\n"
                    "src/ov036/b.c:\n    .rodata start:0x120 end:0x140\n",
                )
                self.assertEqual(bounds.problems("b2_us"), [])
                self.assertIn((0x100, 0x120), bounds.objects("b2_us", bounds.module_sections("b2_us")))

    def test_explicit_extent_is_not_clamped_to_module_end(self):
        self.configs(
            [(".rodata", 0x100, 0x120, "rodata")],
            "table kind:data(any,size=0x30) addr:0x100\n",
            "src/ov036/a.c:\n    .rodata start:0x100 end:0x120\n",
        )
        diagnostics = bounds.problems("b2_us")
        self.assertEqual(len(diagnostics), 1)
        self.assertIn("ends 0x00000130, past the end", diagnostics[0])

    def test_unsized_middle_object_can_cross_file_boundary(self):
        self.configs(
            [(".rodata", 0x100, 0x140, "rodata")],
            "table kind:data(any) addr:0x100\nnext kind:data(any) addr:0x120\n",
            "src/ov036/a.c:\n    .rodata start:0x100 end:0x110\n"
            "src/ov036/b.c:\n    .rodata start:0x110 end:0x140\n",
        )
        diagnostics = bounds.problems("b2_us")
        self.assertEqual(len(diagnostics), 2)
        self.assertIn("past the end", diagnostics[0])
        self.assertIn("starts inside", diagnostics[1])

    def test_start_inside_object_is_reported_when_ends_coincide(self):
        self.configs(
            [(".rodata", 0x100, 0x120, "rodata")],
            "table kind:data(any,size=0x20) addr:0x100\n",
            "src/ov036/a.c:\n    .rodata start:0x110 end:0x120\n",
        )
        diagnostics = bounds.problems("b2_us")
        self.assertEqual(len(diagnostics), 1)
        self.assertIn("starts inside the object at 0x00000100", diagnostics[0])

    def test_inference_does_not_stop_at_source_file_end(self):
        self.configs(
            [(".bss", 0x180, 0x1C0, "bss")],
            "tail kind:bss addr:0x1a0\n",
            "src/ov036/a.c:\n    .bss start:0x1a0 end:0x1b0\n",
        )
        diagnostics = bounds.problems("b2_us")
        self.assertEqual(len(diagnostics), 1)
        self.assertIn("ends 0x000001c0, past the end", diagnostics[0])

    def test_exact_object_boundary_passes(self):
        self.configs(
            [(".rodata", 0x100, 0x120, "rodata")],
            "table kind:data(any,size=0x20) addr:0x100\n",
            "src/ov036/a.c:\n    .rodata start:0x100 end:0x120\n",
        )
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            self.assertEqual(bounds.main(), 0)
        self.assertEqual(output.getvalue(), "0 problem(s)\n")

    def test_main_fails_when_either_version_has_a_cut_object(self):
        self.configs(
            [(".rodata", 0x100, 0x120, "rodata")],
            "table kind:data(any,size=0x20) addr:0x100\n",
            "src/ov036/a.c:\n    .rodata start:0x100 end:0x120\n",
        )
        white = self.root / "config/w2_us/arm9" / bounds.MODULE / "delinks.txt"
        white.write_text(white.read_text().replace("start:0x100 end:0x120\n", "start:0x110 end:0x120\n"))
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            self.assertEqual(bounds.main(), 1)
        self.assertIn("w2_us src/ov036/a.c", output.getvalue())
        self.assertTrue(output.getvalue().endswith("1 problem(s)\n"))


if __name__ == "__main__":
    unittest.main()

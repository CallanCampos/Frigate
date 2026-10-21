"""Range mapping of add_source_file.py, with temporary configs and no ROMs or generated build files.

    python3 -m unittest tools.decomp.test_add_source_file
"""
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.decomp import add_source_file
from tools.decomp.version_addresses import ModulePlacer, Unplaced


class MapRangeTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)

    def config(self, version, text, functions, data=(0x02007000, 0x02007100), files=""):
        """Writes a module's delinks.txt and symbols.txt from (address, size) functions."""
        directory = self.root / version
        directory.mkdir(exist_ok=True)
        start, end = text
        (directory / "delinks.txt").write_text(
            f"    .text       start:{start:#010x} end:{end:#010x} kind:code align:32\n"
            f"    .data       start:{data[0]:#010x} end:{data[1]:#010x} kind:data align:32\n\n{files}"
        )
        symbols = [f"func_{a:08x} kind:function(thumb,size={s:#x}) addr:{a:#010x}" for a, s in functions]
        (directory / "symbols.txt").write_text("\n".join(symbols) + "\n")
        return directory

    def map(self, start, end, primary_functions, other_functions, other_text=(0x02004000, 0x02006000)):
        primary = self.config("b2_us", (0x02004000, 0x02006000), primary_functions)
        other = self.config("w2_us", other_text, other_functions)
        pairs = {p[0]: o[0] for p, o in zip(primary_functions, other_functions)}
        return add_source_file.map_range(ModulePlacer.from_configs(".", primary, other, pairs), ".text", start, end)

    def test_paired_functions(self):
        functions = [(0x02004100, 0x20), (0x02004120, 0x20)]
        other = [(0x02004200, 0x20), (0x02004220, 0x20)]
        self.assertEqual(self.map(0x02004100, 0x02004140, functions, other), (0x02004200, 0x02004240))

    def test_padding_after_the_last_function(self):
        functions = [(0x02004100, 0x1c), (0x02004200, 0x20)]
        other = [(0x02004300, 0x1c), (0x02004400, 0x20)]
        self.assertEqual(self.map(0x02004100, 0x02004120, functions, other), (0x02004300, 0x02004320))

    def test_shuffled_functions_from_the_section_start(self):
        # ARM9 main's secure area: its stubs are in another order in each version, and the file runs from the start of
        # .text to the first function after it
        functions = [(0x02004010, 0x4), (0x02004020, 0x4), (0x02004800, 0x10)]
        other = [(0x02004024, 0x4), (0x02004014, 0x4), (0x02004800, 0x10)]
        self.assertEqual(self.map(0x02004000, 0x02004800, functions, other), (0x02004000, 0x02004800))

    def test_data_between_agreeing_functions(self):
        # ARM9 main's version strings, between crt0's last function and main
        functions = [(0x02004800, 0x50), (0x02004900, 0x10)]
        other = [(0x02004a00, 0x50), (0x02004b00, 0x10)]
        self.assertEqual(self.map(0x02004850, 0x02004890, functions, other), (0x02004a50, 0x02004a90))
        self.assertEqual(self.map(0x02004890, 0x020048c0, functions, other), (0x02004a90, 0x02004ac0))

    def test_disagreeing_functions_stop(self):
        functions = [(0x02004800, 0x50), (0x02004900, 0x10)]
        other = [(0x02004a00, 0x50), (0x02004b10, 0x10)]
        with self.assertRaises(Unplaced) as raised:
            self.map(0x02004860, 0x02004900, functions, other)
        self.assertIn("disagree", str(raised.exception))

    def test_end_inside_a_function_stops(self):
        functions = [(0x02004100, 0x40)]
        other = [(0x02004200, 0x40)]
        with self.assertRaises(SystemExit) as raised:
            self.map(0x02004100, 0x02004120, functions, other)
        self.assertIn("inside the function", str(raised.exception))

    def map_data(self, start, end, other_data):
        primary = self.config("b2_us", (0x02004000, 0x02006000), [])
        other = self.config("w2_us", (0x02004000, 0x02006000), [], data=other_data)
        return add_source_file.map_range(ModulePlacer.from_configs(".", primary, other, {}), ".data", start, end)

    def test_data_of_the_same_size(self):
        mapped = self.map_data(0x02007010, 0x02007020, (0x02007200, 0x02007300))
        self.assertEqual(mapped, (0x02007210, 0x02007220))

    def test_data_of_another_size(self):
        # White 2's .data is 0x10 longer: its edges place a file that fills it, and nothing places an end in between
        other_data = (0x02007200, 0x02007310)
        self.assertEqual(self.map_data(0x02007000, 0x02007100, other_data), (0x02007200, 0x02007310))
        with self.assertRaises(Unplaced):
            self.map_data(0x02007000, 0x02007010, other_data)
        with self.assertRaises(Unplaced):
            self.map_data(0x020070f0, 0x02007100, other_data)

    def test_overlapping_ranges_stop(self):
        directory = self.config(
            "b2_us", (0x02004000, 0x02006000), [], files="src/a.c:\n    .text       start:0x02004100 end:0x02004200\n"
        )
        add_source_file.check_range(directory / "delinks.txt", "src/b.c", ".text", 0x02004200, 0x02004300)
        with self.assertRaises(SystemExit) as raised:
            add_source_file.check_range(directory / "delinks.txt", "src/b.c", ".text", 0x020041f0, 0x02004300)
        self.assertIn("overlaps src/a.c", str(raised.exception))
        with self.assertRaises(SystemExit):
            add_source_file.check_range(directory / "delinks.txt", "src/b.c", ".text", 0x02005f00, 0x02006100)



class MainTests(unittest.TestCase):
    """add_source_file.py's command line on a temporary checkout with two functions and filler between them."""

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        root_patch = patch.object(add_source_file, "ROOT", self.root)
        root_patch.start()
        self.addCleanup(root_patch.stop)
        # White 2 has 0x10 more bytes of filler between the two functions
        for version, second in (("b2_us", 0x02004200), ("w2_us", 0x02004210)):
            directory = self.root / "config" / version / "arm9"
            directory.mkdir(parents=True)
            (directory / "delinks.txt").write_text(
                "    .text       start:0x02004000 end:0x02006000 kind:code align:32\n\n"
                "src/a.c:\n    .text       start:0x02004000 end:0x02004100\n"
            )
            (directory / "symbols.txt").write_text(
                f"func_a kind:function(thumb,size=0x100) addr:0x02004000\n"
                f"func_b kind:function(thumb,size=0x100) addr:{second:#010x}\n"
            )
        (self.root / "build").mkdir()
        (self.root / "build" / "version_map.tsv").write_text(
            "module\tb2_us_addr\tw2_us_addr\n.\t0x02004000\t0x02004000\n.\t0x02004200\t0x02004210\n"
        )

    def run_tool(self, *arguments):
        with patch("sys.argv", ["add_source_file.py", *arguments]):
            add_source_file.main()

    def delinks(self, version):
        return (self.root / "config" / version / "arm9" / "delinks.txt").read_text()

    def test_both_versions_from_the_map(self):
        self.run_tool("src/b.c", ".", ".text:0x02004200-0x02004300", "--incomplete")
        self.assertIn("src/b.c:\n    .text       start:0x02004200 end:0x02004300", self.delinks("b2_us"))
        self.assertIn("src/b.c:\n    .text       start:0x02004210 end:0x02004310", self.delinks("w2_us"))

    def test_filler_between_functions(self):
        # From one function's end to the next one's start, both anchors: White 2's extra filler goes with it
        self.run_tool("src/filler.c", ".", ".text:0x02004100-0x02004200", "--incomplete")
        self.assertIn("src/filler.c:\n    .text       start:0x02004100 end:0x02004210", self.delinks("w2_us"))

    def test_an_unplaced_range_writes_nothing(self):
        # 0x02004180 is 0x80 past func_a in White 2 but 0x90 before func_b, so the map can't place it
        before = self.delinks("b2_us"), self.delinks("w2_us")
        with self.assertRaises(SystemExit) as raised:
            self.run_tool("src/filler.c", ".", ".text:0x02004100-0x02004180", "--incomplete")
        self.assertIn("--other-range w2_us:.text:", str(raised.exception))
        self.assertEqual((self.delinks("b2_us"), self.delinks("w2_us")), before)

    def test_other_range_places_it(self):
        self.run_tool(
            "src/filler.c", ".", ".text:0x02004100-0x02004180", "--incomplete",
            "--other-range", "w2_us:.text:0x02004100-0x02004190",
        )
        self.assertIn("src/filler.c:\n    .text       start:0x02004100 end:0x02004180", self.delinks("b2_us"))
        self.assertIn("src/filler.c:\n    .text       start:0x02004100 end:0x02004190", self.delinks("w2_us"))

    def test_an_overlapping_other_range_writes_nothing(self):
        before = self.delinks("b2_us"), self.delinks("w2_us")
        with self.assertRaises(SystemExit) as raised:
            self.run_tool(
                "src/filler.c", ".", ".text:0x02004100-0x02004180", "--incomplete",
                "--other-range", "w2_us:.text:0x020040f0-0x02004190",
            )
        self.assertIn("overlaps src/a.c", str(raised.exception))
        self.assertEqual((self.delinks("b2_us"), self.delinks("w2_us")), before)

if __name__ == "__main__":
    unittest.main()

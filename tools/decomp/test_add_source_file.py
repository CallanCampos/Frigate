"""Code range mapping of add_source_file.py, with temporary configs and no ROMs or generated build files.

    python3 -m unittest tools.decomp.test_add_source_file
"""
import tempfile
import unittest
from pathlib import Path

from tools.decomp import add_source_file


class MapCodeRangeTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)

    def config(self, version, text, functions):
        """Writes a module's delinks.txt header and symbols.txt from (address, size) functions."""
        directory = self.root / version
        directory.mkdir(exist_ok=True)
        start, end = text
        (directory / "delinks.txt").write_text(f"    .text       start:{start:#010x} end:{end:#010x} kind:code align:32\n")
        symbols = [f"func_{a:08x} kind:function(thumb,size={s:#x}) addr:{a:#010x}" for a, s in functions]
        (directory / "symbols.txt").write_text("\n".join(symbols) + "\n")
        return directory

    def map(self, start, end, primary_functions, other_functions, other_text=(0x02004000, 0x02006000)):
        primary = self.config("b2_us", (0x02004000, 0x02006000), primary_functions)
        other = self.config("w2_us", other_text, other_functions)
        pairs = {p[0]: o[0] for p, o in zip(primary_functions, other_functions)}
        return add_source_file.map_range(".text", start, end, ".", "w2_us", primary, other, pairs)

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
        with self.assertRaises(SystemExit) as raised:
            self.map(0x02004860, 0x02004900, functions, other)
        self.assertIn("disagree", str(raised.exception))

    def test_end_inside_a_function_stops(self):
        functions = [(0x02004100, 0x40)]
        other = [(0x02004200, 0x40)]
        with self.assertRaises(SystemExit) as raised:
            self.map(0x02004100, 0x02004120, functions, other)
        self.assertIn("inside the function", str(raised.exception))


if __name__ == "__main__":
    unittest.main()

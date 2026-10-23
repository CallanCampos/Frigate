"""Address placement of version_addresses.py, on sections and functions given directly.

    python3 -m unittest tools.decomp.test_version_addresses
"""
import unittest

from tools.decomp.version_addresses import ModulePlacer, Unplaced

# White 2's .text is 0x10 longer, from an extra 0x10 of padding before its second function; its .data is 0x40 longer
SECTIONS = {".text": (0x02004000, 0x02004200), ".data": (0x02004200, 0x02004300), ".bss": (0x02004300, 0x02004400)}
OTHER_SECTIONS = {".text": (0x02004000, 0x02004210), ".data": (0x02004220, 0x02004360), ".bss": (0x02004360, 0x02004460)}
FUNCTIONS = {0x02004000: 0x02004080, 0x02004080: 0x020040a0, 0x02004100: 0x02004180}
OTHER_FUNCTIONS = {0x02004000: 0x02004080, 0x02004080: 0x020040a0, 0x02004110: 0x02004190}
# The function at 0x02004080 is not paired
PAIRS = {0x02004000: 0x02004000, 0x02004100: 0x02004110}


class PlaceTests(unittest.TestCase):
    def setUp(self):
        self.placer = ModulePlacer(".", SECTIONS, OTHER_SECTIONS, FUNCTIONS, OTHER_FUNCTIONS, PAIRS)

    def test_inside_a_paired_function_keeps_its_offset(self):
        self.assertEqual(self.placer.place(0x02004124), 0x02004134)

    def test_unpaired_function_between_agreeing_anchors(self):
        other_functions = {0x02004000: 0x02004080, 0x02004080: 0x020040a0, 0x02004100: 0x02004180}
        placer = ModulePlacer(".", SECTIONS, SECTIONS, FUNCTIONS, other_functions, {0x02004000: 0x02004000,
                                                                                     0x02004100: 0x02004100})
        self.assertEqual(placer.place(0x02004090), 0x02004090)
        # With the padding after it longer in White 2, its anchors disagree
        with self.assertRaises(Unplaced):
            self.placer.place(0x02004090)

    def test_padding_whose_size_differs_is_unplaced(self):
        with self.assertRaises(Unplaced) as raised:
            self.placer.place(0x020040c0)
        self.assertIn("disagree", str(raised.exception))

    def test_a_function_start_wins_over_the_previous_end(self):
        self.assertEqual(self.placer.place(0x02004080), 0x02004080)

    def test_section_edges(self):
        self.assertEqual(self.placer.place(0x02004200), 0x02004220)
        self.assertEqual(self.placer.place(0x02004300), 0x02004360)

    def test_a_named_section_ends_where_the_next_one_starts(self):
        # overlay 210's mus_item_data.c: the end of .data is .bss's start, at another place in White 2
        self.assertEqual(self.placer.place(0x02004300, ".data"), 0x02004360)
        self.assertEqual(self.placer.place(0x02004400), 0x02004460)

    def test_middle_of_a_section_of_another_size_is_unplaced(self):
        with self.assertRaises(Unplaced):
            self.placer.place(0x02004280)

    def test_outside_every_section(self):
        with self.assertRaises(Unplaced):
            self.placer.place(0x02005000)


if __name__ == "__main__":
    unittest.main()

#!/usr/bin/env python3
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import merge_objects  # noqa: E402

LCF = """    .main : {
        card_rom.o(.text)
        card_rom.o(.bss)
    } > MAIN
    .ltd_autoload_0 : {
        card_rom_twl.o(.ltdmain)
        card_hash.o(.ltdmain)
        card_rom_twl.o(.bss)
    } > LTD_AUTOLOAD_0
"""
OBJECTS = """"build/b2_us/lib/nitro/src/card/card_rom.o"
"build/b2_us/lib/nitro/src/spi/pm.o"
"build/b2_us/lib/twl/src/card/card_rom_twl.o"
"build/b2_us/lib/twl/src/card/card_hash.o"
"""
MERGED = {"lib/twl/src/card/card_rom_twl.c": "lib/nitro/src/card/card_rom.c"}


class MergeTests(unittest.TestCase):
    def test_places_the_part_from_the_whole(self):
        lcf, objects = merge_objects.merge(LCF, OBJECTS, MERGED)
        self.assertIn("        card_rom.o(.ltdmain)\n        card_hash.o(.ltdmain)\n", lcf)
        self.assertIn("        card_rom.o(.ltdmain.bss)\n", lcf)
        self.assertIn("        card_rom.o(.text)\n        card_rom.o(.bss)\n", lcf)
        self.assertNotIn("card_rom_twl", lcf)
        self.assertNotIn("card_rom_twl.o", objects)

    def test_lists_the_whole_in_the_part_s_place(self):
        _, objects = merge_objects.merge(LCF, OBJECTS, MERGED)
        self.assertEqual([line.rsplit("/", 1)[1] for line in objects.splitlines()],
                         ['pm.o"', 'card_rom.o"', 'card_hash.o"'])

    def test_part_missing_from_the_linker_script_fails(self):
        with self.assertRaises(SystemExit):
            merge_objects.merge(LCF.replace("card_rom_twl", "other"), OBJECTS, MERGED)

    def test_both_incomplete_link_their_original_code(self):
        objects = OBJECTS.replace("build/b2_us/lib/", "build/b2_us/delinks/lib/")
        self.assertEqual(merge_objects.merge(LCF, objects, MERGED), (LCF, objects))

    def test_one_incomplete_fails(self):
        objects = OBJECTS.replace("build/b2_us/lib/twl/", "build/b2_us/delinks/lib/twl/")
        with self.assertRaises(SystemExit):
            merge_objects.merge(LCF, objects, MERGED)

    def test_no_merged_sources_changes_nothing(self):
        self.assertEqual(merge_objects.merge(LCF, OBJECTS, {}), (LCF, OBJECTS))


if __name__ == "__main__":
    unittest.main()

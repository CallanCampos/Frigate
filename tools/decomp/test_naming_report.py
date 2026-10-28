"""The naming report's patterns; no fixture reads the repository's sources or configs."""
import unittest

from tools.decomp import naming_report as nr


class Patterns(unittest.TestCase):
    def test_unk_names(self):
        for name in ("unk_2E4", "unk1C", "unkE6", "BtlvEffect_GetUnk2E4", "WbtUnk18A", "POKEWOOD_BLOCK_FIELD_UNKE0",
                     "unk"):
            self.assertTrue(nr.UNK.search(name), name)
        for name in ("unknown", "Unknown", "trunk", "UNKNOWN_SPECIES", "junkyard"):
            self.assertFalse(nr.UNK.search(name), name)

    def test_placeholder_parameters(self):
        for name in ("a0", "a1", "param2", "arg10", "var3"):
            self.assertTrue(nr.PARAM.match(name), name)
        for name in ("a", "a10", "area", "params", "argc", "variant1"):
            self.assertFalse(nr.PARAM.match(name), name)

    def test_case_labels(self):
        self.assertTrue(nr.CASE.search("    case 3:"))
        self.assertTrue(nr.CASE.search("case 0x1f :"))
        self.assertTrue(nr.CASE.search("case -1:"))
        self.assertFalse(nr.CASE.search("case SEQ_INIT:"))

    def test_compares(self):
        for text in ("x == 2", "x != 0x20", "x == 10)"):
            self.assertTrue(nr.COMPARE.search(text), text)
        for text in ("x == 0", "x != 1", "x == FOO", "x == 2 * y", "x == 3 + y"):
            self.assertFalse(nr.COMPARE.search(text), text)

    def test_comments_and_strings_are_blanked(self):
        text = 'a = 1; // unk_2E4\n"case 3:"; /* unk1C\n*/ b = unk_10;'
        code = nr.code_only(text)
        self.assertEqual(code.count("\n"), text.count("\n"))
        self.assertNotIn("unk_2E4", code)
        self.assertNotIn("case 3", code)
        self.assertNotIn("unk1C", code)
        self.assertIn("unk_10", code)

    def test_modules(self):
        self.assertEqual(nr.module_of("src/ov036/fieldmap.c"), "ov036")
        self.assertEqual(nr.module_of("src/gfl/heap.c"), "main")
        self.assertEqual(nr.module_of("src/system/pokedex_savedata.c"), "main")
        self.assertEqual(nr.module_of("lib/nitro/src/fx/fx.c"), "lib/nitro")


if __name__ == "__main__":
    unittest.main()

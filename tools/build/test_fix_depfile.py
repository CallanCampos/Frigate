"""fix_depfile.py on dependency files as mwccarm writes them.

    python3 -m unittest tools.build.test_fix_depfile
"""
import unittest

from tools.build.fix_depfile import fix_depfile

ROOT = "/home/user/pokebw2"


class FixDepfileTests(unittest.TestCase):
    def test_paths_in_the_checkout_become_relative(self):
        text = ("build\\b2_us\\src\\system\\pml_item.o: src\\system\\pml_item.c \\\r\n"
                "\tZ:\\home\\user\\pokebw2\\include\\types.h \\\r\n"
                "\tZ:\\home\\user\\pokebw2\\build\\include\\generated\\constants\\items.h \r\n")
        self.assertEqual(fix_depfile(text, ROOT), "build/b2_us/src/system/pml_item.o: src/system/pml_item.c \\\n"
                                                  "\tinclude/types.h \\\n"
                                                  "\tbuild/include/generated/constants/items.h\n")

    def test_paths_outside_the_checkout_stay_absolute(self):
        text = ("build\\b2_us\\src\\system\\pml_item.o: src\\system\\pml_item.c \\\r\n"
                "\tZ:\\home\\user\\pokebw2-mod\\include\\types.h \\\r\n"
                "\tZ:\\opt\\mwccarm\\include\\stddef.h \r\n")
        self.assertEqual(fix_depfile(text, ROOT), "build/b2_us/src/system/pml_item.o: src/system/pml_item.c \\\n"
                                                  "\t/home/user/pokebw2-mod/include/types.h \\\n"
                                                  "\t/opt/mwccarm/include/stddef.h\n")


if __name__ == "__main__":
    unittest.main()

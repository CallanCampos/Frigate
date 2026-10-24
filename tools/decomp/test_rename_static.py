"""Recognize the statics that rename_symbol may give a name other files' statics already have."""
from pathlib import Path
import tempfile
import unittest

from tools.decomp import rename_symbol as rename


class IsStaticTests(unittest.TestCase):
    def check(self, source: str, name: str) -> bool:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'file.c'
            path.write_text(source)
            return rename.is_static(path, name)

    def test_plain_static(self):
        self.assertTrue(self.check('static const u16 sTable[] = { 1, 2 };\n', 'sTable'))

    def test_function_pointer_array(self):
        self.assertTrue(self.check('static u32 (*const sStateFuncs[])(Work *wk) = {\n    Init,\n};\n', 'sStateFuncs'))

    def test_anonymous_struct(self):
        self.assertTrue(self.check('static struct {\n    int a;\n    int b;\n} sNet = { -1, -1 };\n', 'sNet'))

    def test_parameter_is_not_a_static(self):
        self.assertFalse(self.check('static void Run(Work *sWork);\n', 'sWork'))

    def test_global(self):
        self.assertFalse(self.check('const u8 sKeyPalettes[5] = { 2, 1, 1, 1, 2 };\n', 'sKeyPalettes'))


if __name__ == '__main__':
    unittest.main()

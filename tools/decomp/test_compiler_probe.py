"""Original-code lookup must distinguish file-local names before reporting a match."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from tools.decomp import compiler_probe


class OriginalLookupTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name).resolve()
        self.config = self.root / "config/b2_us/arm9"
        self.modules = {}
        self.first = self.module("overlays/ov011", 0x2100000, b"first123second45")
        (self.first / "symbols.txt").write_text(
            "Local kind:function(thumb,size=0x8) addr:0x02100000\n"
            "Local kind:function(thumb,size=0x8) addr:0x02100008\n"
            "Unique kind:function(arm,size=0x4) addr:0x02100000\n")
        (self.first / "delinks.txt").write_text(
            "src/first.c:\n    .text start:0x02100000 end:0x02100008\n"
            "src/second.c:\n    .text start:0x02100008 end:0x02100010\n")
        patcher = patch.object(compiler_probe, "ROOT", self.root)
        patcher.start()
        self.addCleanup(patcher.stop)
        self.addCleanup(compiler_probe.source_ranges.cache_clear)

    def module(self, name, base, data):
        directory = self.config / name
        directory.mkdir(parents=True)
        binary = directory / "original.bin"
        binary.write_bytes(data)
        self.modules[str(directory)] = (binary, base)
        return directory

    def test_duplicate_names_in_one_module_use_the_source_range(self):
        self.assertEqual(compiler_probe.find_function("b2_us", "Local", self.modules, Path("src/second.c")),
                         (b"second45", 0x2100008, True))

    def test_overlapping_overlay_addresses_use_the_source_module(self):
        other = self.module("overlays/ov182", 0x2100000, b"overlay2")
        (other / "symbols.txt").write_text("Local kind:function(thumb,size=0x8) addr:0x02100000\n")
        (other / "delinks.txt").write_text("src/other.c:\n    .text start:0x02100000 end:0x02100008\n")
        self.assertEqual(compiler_probe.find_function("b2_us", "Local", self.modules, Path("src/other.c")),
                         (b"overlay2", 0x2100000, True))

    def test_absolute_source_path_and_exclusive_range_end(self):
        self.assertEqual(compiler_probe.find_function("b2_us", "Local", self.modules, self.root / "src/first.c"),
                         (b"first123", 0x2100000, True))

    def test_ambiguous_name_without_an_owner_cannot_silently_match(self):
        with self.assertRaisesRegex(ValueError, "ambiguous original"):
            compiler_probe.find_function("b2_us", "Local", self.modules)

    def test_unique_names_still_work_for_unregistered_probe_sources(self):
        expected = (b"firs", 0x2100000, False)
        self.assertEqual(compiler_probe.find_function("b2_us", "Unique", self.modules), expected)
        self.assertEqual(compiler_probe.find_function("b2_us", "Unique", self.modules, Path("scratch.c")), expected)

    def test_registered_owner_does_not_fall_back_to_another_files_symbol(self):
        self.assertIsNone(compiler_probe.find_function("b2_us", "Unique", self.modules, Path("src/second.c")))

    def test_each_version_uses_its_own_source_ranges(self):
        other = self.root / "config/w2_us/arm9/overlays/ov011"
        other.mkdir(parents=True)
        binary = other / "original.bin"
        binary.write_bytes(b"WHITE2!!")
        self.modules[str(other)] = (binary, 0x2200000)
        (other / "symbols.txt").write_text("Local kind:function(thumb,size=0x8) addr:0x02200000\n")
        (other / "delinks.txt").write_text("src/second.c:\n    .text start:0x02200000 end:0x02200008\n")
        self.assertEqual(compiler_probe.find_function("w2_us", "Local", self.modules, Path("src/second.c")),
                         (b"WHITE2!!", 0x2200000, True))

    def test_migrated_ltdmain_section_is_found_in_the_ltd_autoload(self):
        # An ARM9 main file whose entry has a migrated .ltdmain section: its DSi part's functions are the LTD autoload's
        binary = self.config / "arm9.bin"
        binary.write_bytes(b"mainpart")
        self.modules[str(self.config)] = (binary, 0x2000000)
        (self.config / "symbols.txt").write_text("Local kind:function(thumb,size=0x8) addr:0x02000000\n")
        (self.config / "delinks.txt").write_text(
            "src/whole.c:\n    .text start:0x02000000 end:0x02000008\n    .ltdmain start:0x02700000 end:0x02700008\n")
        ltd = self.module("ltd_autoload_0", 0x2700000, b"dsi_part")
        (ltd / "symbols.txt").write_text("Part kind:function(thumb,size=0x8) addr:0x02700000\n")
        (ltd / "delinks.txt").write_text("")
        self.assertEqual(compiler_probe.source_ranges(self.root, "b2_us", Path("src/whole.c")),
                         {self.config: ((0x2000000, 0x2000008),), ltd: ((0x2700000, 0x2700008),)})
        self.assertEqual(compiler_probe.find_function("b2_us", "Part", self.modules, Path("src/whole.c")),
                         (b"dsi_part", 0x2700000, True))

    def test_encrypted_duplicate_uses_its_owners_delinked_object(self):
        (self.first / "symbols.txt").write_text(
            "Local kind:function(thumb,size=0x8) addr:0x02100000\n"
            "Local kind:function(thumb,size=0x8,dsprot=(0x4,0x1)) addr:0x02100008\n")
        obj = self.root / "build/b2_us/delinks/src/second.o"
        obj.parent.mkdir(parents=True)
        obj.write_bytes(b"DECIPHER")
        # Object parsing is separate; a populated global-name cache must not override this owner.
        with patch.dict(compiler_probe._delinked, {"b2_us": {"Local": b"WRONG!!!"}}):
            with patch.object(compiler_probe, "compiled_functions", return_value={"Local": (b"DECIPHER", set())}) as read:
                self.assertEqual(compiler_probe.find_function("b2_us", "Local", self.modules, Path("src/second.c")),
                                 (b"DECIPHER", 0x2100008, True))
                read.assert_called_once_with(obj)

    def test_unique_encrypted_name_still_works_for_an_unregistered_source(self):
        (self.first / "symbols.txt").write_text(
            "Secret kind:function(thumb,size=0x8,dsprot=(0x4,0x1)) addr:0x02100008\n")
        with patch.dict(compiler_probe._delinked, {"b2_us": {"Secret": b"DECIPHER"}}):
            self.assertEqual(compiler_probe.find_function("b2_us", "Secret", self.modules, Path("scratch.c")),
                             (b"DECIPHER", 0x2100008, True))


if __name__ == "__main__":
    unittest.main()

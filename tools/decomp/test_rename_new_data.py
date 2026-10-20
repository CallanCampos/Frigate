"""Rename newly added data without trusting unrelated symbols at mapped addresses."""
import contextlib
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from tools.decomp import config_fixes as fixes
from tools.decomp import rename_symbol as rename

MODULE = 'overlays/ov036'
ADDRESS = 0x021cff00
MAPPED = ADDRESS + 0x34
OLD = 'data_ov036_021cff00'
NEW = 'sCursorColorEndpoints'


class NewDataRenameTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        for owner, attribute, value in [(rename, 'ROOT', self.root), (fixes, 'ROOT', self.root),
                                         (fixes, 'MAP', self.root / 'version_map.tsv')]:
            patcher = patch.object(owner, attribute, value)
            patcher.start()
            self.addCleanup(patcher.stop)
        # No paired functions: AddressMapper must use equal-sized .rodata sections, whose offset is +0x34.
        patcher = patch.object(fixes, 'load_modules', return_value={MODULE: type('Module', (), {'functions': lambda self: []})()})
        patcher.start()
        self.addCleanup(patcher.stop)
        fixes.MAP.write_text('module\tb2_us_addr\tw2_us_addr\n')
        for version, start in [('b2_us', ADDRESS), ('w2_us', MAPPED)]:
            directory = self.path(version).parent
            directory.mkdir(parents=True)
            (directory / 'delinks.txt').write_text(
                f'    .rodata start:{start:#010x} end:{start + 0x100:#010x} kind:rodata align:4\n\n')
            self.path(version).write_text('')
        fixes.apply_everywhere(fixes.AddressMapper(), MODULE, ADDRESS, 'add_data', OLD)
        self.pairs = {'w2_us': {}}

    def path(self, version):
        return self.root / 'config' / version / 'arm9' / MODULE / 'symbols.txt'

    def apply(self):
        with contextlib.redirect_stdout(io.StringIO()):
            return rename.apply(MODULE, ADDRESS, NEW, self.pairs)

    def test_new_add_data_uses_section_offset_in_both_versions(self):
        self.assertEqual(self.apply(), [OLD, OLD])
        self.assertIn(f'{NEW} kind:data(any) addr:{ADDRESS:#010x}', self.path('b2_us').read_text())
        self.assertIn(f'{NEW} kind:data(any) addr:{MAPPED:#010x}', self.path('w2_us').read_text())

    def test_replay_after_primary_already_renamed(self):
        rename.set_name('b2_us', MODULE, ADDRESS, NEW)
        self.assertEqual(self.apply(), [NEW, OLD])
        snapshot = self.path('w2_us').read_bytes()
        self.assertEqual(self.apply(), [NEW, NEW])
        self.assertEqual(snapshot, self.path('w2_us').read_bytes())

    def test_unrelated_version_only_name_is_preserved(self):
        for conflicting in ['sWhiteOnlyTable', 'data_ov036_021cff34', 'data_ov099_021cff00']:
            rename.set_name('w2_us', MODULE, MAPPED, conflicting)
            before = self.path('w2_us').read_bytes()
            self.assertEqual(self.apply(), [OLD if conflicting == 'sWhiteOnlyTable' else NEW])
            self.assertEqual(before, self.path('w2_us').read_bytes())

    def test_mapping_failure_preserves_other_version(self):
        before = self.path('w2_us').read_bytes()
        with patch.object(fixes.AddressMapper, 'map', side_effect=SystemExit('unmappable')):
            self.assertEqual(self.apply(), [OLD])
        self.assertEqual(before, self.path('w2_us').read_bytes())

    def test_missing_explicit_counterpart_fails_before_primary_write(self):
        self.pairs['w2_us'][(MODULE, ADDRESS)] = MAPPED + 4
        before = self.path('b2_us').read_bytes()
        with self.assertRaises(SystemExit):
            self.apply()
        self.assertEqual(before, self.path('b2_us').read_bytes())


if __name__ == '__main__':
    unittest.main()

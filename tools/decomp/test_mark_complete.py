"""Cross-file static checks use temporary configs, never the live checkout."""
import contextlib
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from tools.decomp import add_source_file, mark_complete


class StaticReferenceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.version = self.root / 'config/b2_us/arm9'
        self.owner = self.version / 'overlays/ov011'
        self.owner.mkdir(parents=True)
        self.source = 'src/library.c'
        (self.root / 'src').mkdir()
        (self.root / self.source).write_text(
            'static int Local(void) { return 1; }\n'
            'static int OtherLocal(void) { return 2; }\n'
            'int PublicAlias(void) { return 3; }\n')
        (self.owner / 'delinks.txt').write_text(
            f'{self.source}:\n    .text start:0x02100000 end:0x02100020\n')
        (self.owner / 'relocs.txt').write_text('')
        self.symbols('Local kind:function(thumb,size=0x10) addr:0x02100000')
        self.relocs('from:0x02000000 kind:abs32 to:0x02100000 module:overlay(11)')
        root_patch = patch.object(mark_complete, 'ROOT', self.root)
        root_patch.start()
        self.addCleanup(root_patch.stop)

    def symbols(self, *lines):
        (self.owner / 'symbols.txt').write_text('\n'.join(lines) + '\n')

    def relocs(self, *lines):
        (self.version / 'relocs.txt').write_text('\n'.join(lines) + '\n')

    def check(self):
        return mark_complete.externally_used_statics(self.source, self.version)

    def test_ordinary_static_reference(self):
        self.assertEqual(self.check(), ['Local'])

    def test_function_and_label_in_either_order(self):
        function = 'Local kind:function(thumb,size=0x10) addr:0x02100000'
        label = 'UnresolvedTarget kind:label addr:0x02100000'
        for lines in ((function, label), (label, function)):
            with self.subTest(lines=lines):
                self.symbols(*lines)
                self.assertEqual(self.check(), ['Local'])

    def test_all_static_names_retained_and_nonstatic_aliases_filtered(self):
        self.symbols(
            'Local kind:function(thumb,size=0x10) addr:0x02100000',
            'PublicAlias kind:label addr:0x02100000',
            'OtherLocal kind:label addr:0x02100000',
            'UnresolvedTarget kind:label addr:0x02100000')
        self.assertEqual(self.check(), ['Local', 'OtherLocal'])

    def test_thumb_bit_normalized_on_symbols_and_references(self):
        for symbol, target in ((0x02100001, 0x02100000),
                               (0x02100000, 0x02100001),
                               (0x02100001, 0x02100001)):
            with self.subTest(symbol=symbol, target=target):
                self.symbols(f'Local kind:function(thumb,size=0x10) addr:{symbol:#010x}')
                self.relocs(f'from:0x02000000 kind:abs32 to:{target:#010x} module:overlay(11)')
                self.assertEqual(self.check(), ['Local'])

    def test_same_file_code_and_data_references_ignored(self):
        self.relocs()
        with (self.owner / 'delinks.txt').open('a') as f:
            f.write('    .rodata start:0x02100040 end:0x02100050\n'
                    '    .data start:0x02100060 end:0x02100070\n')
        for origin in (0x02100000, 0x02100010, 0x0210001f,
                       0x02100040, 0x0210004f, 0x02100060, 0x0210006f):
            with self.subTest(origin=origin):
                (self.owner / 'relocs.txt').write_text(
                    f'from:{origin:#010x} kind:abs32 to:0x02100000 module:overlay(11)\n')
                self.assertEqual(self.check(), [])

    def test_same_overlay_cross_file_code_and_callback_references(self):
        self.relocs()
        with (self.owner / 'delinks.txt').open('a') as f:
            f.write('\nsrc/caller.c:\n'
                    '    .text start:0x02100020 end:0x02100040\n'
                    '    .rodata start:0x02100040 end:0x02100050\n'
                    '    .data start:0x02100060 end:0x02100070\n')
        self.symbols('Local kind:function(thumb,size=0x10) addr:0x02100000',
                     'Alias kind:label(thumb) addr:0x02100000')
        for origin in (0x02100020, 0x02100024, 0x02100040, 0x02100060):
            with self.subTest(origin=origin):
                (self.owner / 'relocs.txt').write_text(
                    f'from:{origin:#010x} kind:load to:0x02100001 add:0x4 module:overlays(11,12)\n')
                self.assertEqual(self.check(), ['Local'])

    def test_same_overlay_unowned_and_end_boundary_sources_rejected(self):
        self.relocs()
        with (self.owner / 'delinks.txt').open('a') as f:
            f.write('    .rodata start:0x02100040 end:0x02100050\n')
        for origin in (0x020ffffc, 0x02100020, 0x0210003c, 0x02100050):
            with self.subTest(origin=origin):
                (self.owner / 'relocs.txt').write_text(
                    f'from:{origin:#010x} kind:abs32 to:0x02100000 module:overlay(11)\n')
                self.assertEqual(self.check(), ['Local'])

    def test_owned_data_is_not_a_static_function_target(self):
        self.relocs()
        with (self.owner / 'delinks.txt').open('a') as f:
            f.write('    .data start:0x02100040 end:0x02100050\n')
        self.symbols('Local kind:label(thumb) addr:0x02100040')
        (self.owner / 'relocs.txt').write_text(
            'from:0x02100100 kind:abs32 to:0x02100040 module:overlay(11)\n')
        self.assertEqual(self.check(), [])

    def test_same_overlay_rejection_in_either_version_does_not_mutate_configs(self):
        self.relocs()
        other = self.root / 'config/w2_us/arm9/overlays/ov011'
        other.mkdir(parents=True)
        for name in ('delinks.txt', 'symbols.txt'):
            (other / name).write_bytes((self.owner / name).read_bytes())
        for rejected in (self.owner, other):
            with self.subTest(version=rejected):
                for module in (self.owner, other):
                    (module / 'relocs.txt').write_text('')
                (rejected / 'relocs.txt').write_text(
                    'from:0x02100100 kind:abs32 to:0x02100000 module:overlay(11)\n')
                original = {p: p.read_bytes() for p in self.root.rglob('*.txt')}
                with patch('sys.argv', ['mark_complete.py', self.source]), contextlib.redirect_stdout(io.StringIO()) as output:
                    self.assertEqual(mark_complete.main(), 1)
                self.assertIn('Local', output.getvalue())
                self.assertEqual({p: p.read_bytes() for p in self.root.rglob('*.txt')}, original)

    def test_other_target_module_ignored(self):
        self.relocs('from:0x02000000 kind:abs32 to:0x02100000 module:overlay(12)')
        self.assertEqual(self.check(), [])

    def test_ambiguous_overlay_binds_only_first_candidate(self):
        for modules, expected in (('overlays(11,12)', ['Local']),
                                  ('overlays(12,11)', []),
                                  ('overlays(12,13,11)', []),
                                  ('overlays(11,13,12)', ['Local'])):
            with self.subTest(modules=modules):
                self.relocs(f'from:0x02000000 kind:abs32 to:0x02100000 module:{modules}')
                self.assertEqual(self.check(), expected)

    def test_ambiguous_overlay_alias_and_thumb_reference(self):
        self.symbols('Local kind:function(thumb,size=0x10) addr:0x02100001',
                     'Alias kind:label addr:0x02100000')
        self.relocs('from:0x02000000 kind:abs32 to:0x02100001 module:overlays(11,12)')
        self.assertEqual(self.check(), ['Local'])

    def test_relocation_addend_preserves_base_symbol_reference(self):
        self.symbols('Local kind:function(thumb,size=0x10) addr:0x02100000',
                     'Alias kind:label addr:0x02100000')
        for module in ('overlay(11)', 'overlays(11,12)'):
            for addend in ('0x0', '0x4', '-0x4'):
                with self.subTest(module=module, addend=addend):
                    self.relocs('from:0x02000000 kind:load to:0x02100000 '
                                f'add:{addend} module:{module}')
                    self.assertEqual(self.check(), ['Local'])

    def test_relocation_addend_does_not_change_selected_overlay(self):
        self.relocs('from:0x02000000 kind:load to:0x02100000 '
                    'add:0x4 module:overlays(12,11)')
        self.assertEqual(self.check(), [])

    def test_main_module_external_reference(self):
        # Move the registered source to main and refer to it from an overlay.
        for name in ('delinks.txt', 'symbols.txt'):
            (self.owner / name).replace(self.version / name)
        self.relocs()
        (self.owner / 'relocs.txt').write_text(
            'from:0x02200000 kind:abs32 to:0x02100000 module:main\n')
        self.assertEqual(self.check(), ['Local'])

    def test_reference_outside_text_ignored(self):
        for target in (0x020ffffe, 0x02100020):
            with self.subTest(target=target):
                self.symbols(f'Local kind:function(thumb,size=0x10) addr:{target:#010x}')
                self.relocs(f'from:0x02000000 kind:abs32 to:{target:#010x} module:overlay(11)')
                self.assertEqual(self.check(), [])

    def test_main_refuses_without_any_delink_writes(self):
        other = self.root / 'config/w2_us/arm9'
        other.mkdir(parents=True)
        (other / 'delinks.txt').write_text(f'{self.source}:\n    .text start:0x02200000 end:0x02200020\n')
        (other / 'symbols.txt').write_text('')
        paths = list(self.root.rglob('delinks.txt'))
        original = {path: path.read_bytes() for path in paths}
        with patch('sys.argv', ['mark_complete.py', self.source]), contextlib.redirect_stdout(io.StringIO()) as output:
            self.assertEqual(mark_complete.main(), 1)
        self.assertIn('Local', output.getvalue())
        self.assertEqual({path: path.read_bytes() for path in paths}, original)

    def test_add_source_file_import_uses_same_guard(self):
        self.assertIs(add_source_file.externally_used_statics, mark_complete.externally_used_statics)
        self.symbols('Local kind:function(thumb,size=0x10) addr:0x02100000',
                     'Alias kind:label addr:0x02100000')
        self.assertEqual(add_source_file.externally_used_statics(self.source, self.version), ['Local'])


if __name__ == '__main__':
    unittest.main()

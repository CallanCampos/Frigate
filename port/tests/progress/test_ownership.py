"""Delinks ownership regressions for progress.py's ROM source-coverage reports (no game code is executed).

Explicit main-object LTD/DTCM sections must retain their source owner in their destination module. Shared overlay
addresses, conflicting claims and ranges outside section bounds must never manufacture matching coverage.
"""
import csv
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
SCRIPT = ROOT / 'port/tools/progress/progress.py'
spec = importlib.util.spec_from_file_location('port_progress', SCRIPT)
progress = importlib.util.module_from_spec(spec)
spec.loader.exec_module(progress)


def file(path, sections, complete=True):
    return {'path': path, 'sections': sections, 'complete': complete}


def module(sections, files=()):
    return {'sections': {name: {'start': start, 'end': end} for name, (start, end) in sections.items()},
            'files': list(files)}


class OwnershipTests(unittest.TestCase):
    def test_ltd_code_and_bss(self):
        source = file('sdk.c', {'.text': (0x100, 0x120), '.ltdmain': (0x200, 0x240),
                                '.ltdmain.bss': (0x300, 0x320)})
        owners = progress.resolve_owners({'main': module({'.text': (0x100, 0x180)}, [source]),
                                         'ltd_autoload_0': module({'.ltdmain': (0x200, 0x280),
                                                                   '.bss': (0x300, 0x380)})})
        self.assertIs(progress.range_owner(owners['main'], '.text', 0x100, 0x20), source)
        self.assertIs(progress.range_owner(owners['ltd_autoload_0'], '.ltdmain', 0x200, 0x40), source)
        self.assertEqual(owners['ltd_autoload_0']['.bss'], [(0x300, 0x320, source)])
        self.assertNotIn('.ltdmain', owners['main'])

    def test_dtcm_explicit_spellings(self):
        source = file('irq.c', {'.dtcm.data': (0x400, 0x420), '.dtcm.bss': (0x440, 0x450)})
        owners = progress.resolve_owners({'main': module({}, [source]),
                                         'dtcm': module({'.data': (0x400, 0x440), '.bss': (0x440, 0x480)})})
        self.assertIs(progress.range_owner(owners['dtcm'], '.data', 0x400), source)
        self.assertIs(progress.range_owner(owners['dtcm'], '.bss', 0x440), source)
        source['sections'] = {'.dtcm': (0x440, 0x450)}
        owners = progress.resolve_owners({'main': module({}, [source]),
                                         'dtcm': module({'.bss': (0x440, 0x480)})})
        self.assertIs(progress.range_owner(owners['dtcm'], '.bss', 0x440), source)

    def test_same_module_and_overlay_alias(self):
        a = file('a.c', {'.text': (0x100, 0x120)})
        b = file('b.c', {'.text': (0x100, 0x120)})
        owners = progress.resolve_owners({'ov001': module({'.text': (0x100, 0x140)}, [a]),
                                         'ov002': module({'.text': (0x100, 0x140)}, [b]),
                                         'ov003': module({'.text': (0x100, 0x140)})})
        self.assertIs(progress.range_owner(owners['ov001'], '.text', 0x100), a)
        self.assertIs(progress.range_owner(owners['ov002'], '.text', 0x100), b)
        self.assertIsNone(progress.range_owner(owners['ov003'], '.text', 0x100))

    def test_only_main_can_migrate(self):
        a = file('overlay.c', {'.ltdmain': (0x200, 0x240)})
        owners = progress.resolve_owners({'ov001': module({'.text': (0x100, 0x140)}, [a]),
                                         'ltd_autoload_0': module({'.ltdmain': (0x200, 0x280)})})
        self.assertIsNone(progress.range_owner(owners['ltd_autoload_0'], '.ltdmain', 0x200))

    def test_no_global_address_fallback(self):
        a = file('main.c', {'.text': (0x100, 0x120)})
        owners = progress.resolve_owners({'main': module({'.text': (0x100, 0x140)}, [a]),
                                         'ltd_autoload_0': module({'.text': (0x100, 0x140)})})
        self.assertIsNone(progress.range_owner(owners['ltd_autoload_0'], '.text', 0x100))

    def test_ambiguous_ranges_leave_only_intersection_unowned(self):
        a = file('a.c', {'.ltdmain': (0x200, 0x250)})
        b = file('b.c', {'.ltdmain': (0x220, 0x240)})
        owners = progress.resolve_owners({'main': module({}, [a, b]),
                                         'ltd_autoload_0': module({'.ltdmain': (0x200, 0x280)})})
        spans = owners['ltd_autoload_0']
        self.assertIs(progress.range_owner(spans, '.ltdmain', 0x200, 0x20), a)
        self.assertIsNone(progress.range_owner(spans, '.ltdmain', 0x220))
        self.assertIsNone(progress.range_owner(spans, '.ltdmain', 0x210, 0x20))
        self.assertIs(progress.range_owner(spans, '.ltdmain', 0x240), a)
        self.assertEqual(sum(end - start for start, end, _ in spans['.ltdmain']), 0x30)

    def test_same_module_overlap_is_ambiguous(self):
        a = file('a.c', {'.data': (0x100, 0x120)})
        b = file('b.c', {'.data': (0x110, 0x130)})
        owners = progress.resolve_owners({'ov001': module({'.data': (0x100, 0x140)}, [a, b])})
        self.assertIs(progress.range_owner(owners['ov001'], '.data', 0x100), a)
        self.assertIsNone(progress.range_owner(owners['ov001'], '.data', 0x110))
        self.assertIs(progress.range_owner(owners['ov001'], '.data', 0x120), b)

    def test_local_migrated_collision_not_arbitrated(self):
        a = file('a.c', {'.ltdmain': (0x200, 0x240)})
        b = file('b.c', {'.ltdmain': (0x200, 0x240)})
        owners = progress.resolve_owners({'main': module({}, [a]),
                                         'ltd_autoload_0': module({'.ltdmain': (0x200, 0x280)}, [b])})
        self.assertIsNone(progress.range_owner(owners['ltd_autoload_0'], '.ltdmain', 0x200))

    def test_wrong_section_or_incomplete_range_never_clipped(self):
        a = file('a.c', {'.ltdmain': (0x1f0, 0x240)})
        for section in ('.ltdmain', '.text'):
            owners = progress.resolve_owners({'main': module({}, [a]),
                                             'ltd_autoload_0': module({section: (0x200, 0x280)})})
            self.assertIsNone(progress.range_owner(owners['ltd_autoload_0'], section, 0x210))

    def test_incomplete_owner_is_not_marked_complete(self):
        a = file('a.c', {'.ltdmain': (0x200, 0x240)}, complete=False)
        owners = progress.resolve_owners({'main': module({}, [a]),
                                         'ltd_autoload_0': module({'.ltdmain': (0x200, 0x280)})})
        self.assertFalse(progress.range_owner(owners['ltd_autoload_0'], '.ltdmain', 0x200)['complete'])

    def test_full_report_module_identity_and_data_accounting(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            main = root / 'config/b2_us/arm9'
            ltd = main / 'ltd_autoload_0'
            ltd.mkdir(parents=True)
            (root / 'sdk.c').write_text('void Native(void) {}\nvoid Migrated(void) {}\n')
            (main / 'delinks.txt').write_text('''    .text start:0x100 end:0x120 kind:code
sdk.c:
    complete
    .text start:0x100 end:0x120
    .ltdmain start:0x200 end:0x220
    .ltdmain.bss start:0x300 end:0x310
''')
            (ltd / 'delinks.txt').write_text('''    .ltdmain start:0x200 end:0x220 kind:code
    .bss start:0x300 end:0x320 kind:bss
''')
            (main / 'symbols.txt').write_text('Native kind:function(thumb,size=0x20) addr:0x100\n')
            (ltd / 'symbols.txt').write_text('Migrated kind:function(thumb,size=0x20) addr:0x200\n')
            out = root / 'report'
            done = subprocess.run([sys.executable, str(SCRIPT), '--root', str(root), '--out', str(out)],
                                  capture_output=True, text=True)
            self.assertEqual(done.returncode, 0, done.stderr)
            rows = list(csv.DictReader((out / 'functions.tsv').read_text().splitlines(), delimiter='\t'))
            self.assertEqual([(r['module'], r['section'], r['addr'], r['file'], r['tier']) for r in rows],
                             [('main', '.text', '0x100', 'sdk.c', 'a'),
                              ('ltd_autoload_0', '.ltdmain', '0x200', 'sdk.c', 'a')])
            data = json.loads((out / 'data.json').read_text())
            self.assertEqual(sum(r['bytes'] for r in data if r['tier'] == 'a'), 16)
            self.assertEqual(sum(r['bytes'] for r in data if r['tier'] == 'c_gap'), 16)
            files = list(csv.DictReader((out / 'files.tsv').read_text().splitlines(), delimiter='\t'))
            self.assertEqual([(r['module'], r['text'], r['data']) for r in files],
                             [('main', '32', '0'), ('ltd_autoload_0', '32', '16')])


if __name__ == '__main__':
    unittest.main()

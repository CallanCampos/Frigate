"""ROM-free regressions for the experimental IRES loader's rom_manifest.py and actual rom.cmake emission.

Fabricated FNT/FAT/overlay bytes exercise rejection before an exact-file hash can authorize malformed ROM tables.
The CMake probe evaluates the production manifest fragment with target registration stubbed; it compiles no game.
"""
import copy
import importlib.util
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
SCRIPT = ROOT / 'port/tools/research/assets/rom_manifest.py'
spec = importlib.util.spec_from_file_location('rom_manifest', SCRIPT)
manifest = importlib.util.module_from_spec(spec)
spec.loader.exec_module(manifest)


def fixture():
    rom = bytearray(0x4000)
    rom[0x0c:0x10] = b'IRES'
    # 344 unique overlay files and one named data file, all one byte long.
    struct.pack_into('<6I', rom, 0x40, 0x200, 14, 0x240, 345 * 8, 0xd20, 344 * 32)
    struct.pack_into('<IHH', rom, 0x200, 8, 344, 1)
    rom[0x208:0x20e] = b'\x04data\x00'
    for i in range(345):
        struct.pack_into('<II', rom, 0x240 + i * 8, 0x3900 + i, 0x3901 + i)
    for i in range(344):
        struct.pack_into('<8I', rom, 0xd20 + i * 32, i, 0x2100000, 1, 0, 0, 0, i, 0)
    fix_crc(rom)
    return rom


def fix_crc(rom):
    struct.pack_into('<H', rom, 0x15e, manifest.crc16(rom[:0x15e]))


class ManifestTests(unittest.TestCase):
    def test_valid_round_trip(self):
        entry = manifest.make_entry(fixture())
        manifest.validate_entry(entry, 344)
        self.assertEqual(entry['files']['data']['size'], 1)
        self.assertTrue(entry['header_crc16_valid'])

    def reject(self, mutate, text):
        rom = fixture()
        mutate(rom)
        fix_crc(rom)
        with self.assertRaisesRegex(ValueError, text):
            manifest.make_entry(rom)

    def test_short_header_and_crc(self):
        with self.assertRaisesRegex(ValueError, 'header'):
            manifest.make_entry(b'IRES')
        rom = fixture()
        rom[0x20] ^= 1
        with self.assertRaisesRegex(ValueError, 'CRC'):
            manifest.make_entry(rom)

    def test_overlay_outside_rom_cannot_self_certify(self):
        self.reject(lambda r: struct.pack_into('<I', r, 0x50, len(r) + 32), 'overlay table lies outside')

    def test_table_bounds(self):
        for field in (0x40, 0x48):
            with self.subTest(field=field):
                self.reject(lambda r: struct.pack_into('<I', r, field, len(r)), 'outside')
        self.reject(lambda r: struct.pack_into('<I', r, 0x4c, 345 * 8 - 1), 'multiple of eight')
        self.reject(lambda r: struct.pack_into('<I', r, 0x54, 32), 'overlay count')

    def test_overlay_ids_and_sizes(self):
        self.reject(lambda r: struct.pack_into('<I', r, 0xd20, 1), 'overlay ID')
        self.reject(lambda r: struct.pack_into('<I', r, 0xd20 + 24, 345), 'file ID')
        self.reject(lambda r: struct.pack_into('<I', r, 0xd20 + 8, 2), 'exceeds FAT')
        self.reject(lambda r: struct.pack_into('<I', r, 0xd20 + 28, 0x1000002), 'exceeds FAT')

    def test_fat_file_bounds(self):
        self.reject(lambda r: struct.pack_into('<II', r, 0x240, 9, 8), 'FAT file')
        self.reject(lambda r: struct.pack_into('<II', r, 0x240, 0x4000, 0x4001), 'FAT file')

    def test_fnt_bounds_ids_and_cycle(self):
        self.reject(lambda r: struct.pack_into('<I', r, 0x200, 14), 'subtable')
        self.reject(lambda r: struct.pack_into('<H', r, 0x204, 345), 'file ID')
        self.reject(lambda r: r.__setitem__(slice(0x208, 0x20e), b'\x81a\x00\xf0\x00\x00'), 'cyclic')
        self.reject(lambda r: r.__setitem__(slice(0x208, 0x20e), b'\x81a\x01\xf0\x00\x00'), 'directory ID')
        self.reject(lambda r: r.__setitem__(0x208, 127), 'truncated')

    def test_duplicate_and_unsafe_names(self):
        def duplicate(r):
            struct.pack_into('<I', r, 0x44, 19)
            struct.pack_into('<H', r, 0x204, 343)
            r[0x208:0x213] = b'\x04data\x04data\x00'
        self.reject(duplicate, 'duplicate')
        for name in (b'a"aa', b'a\\aa', b'a;aa', b'../a'):
            self.reject(lambda r: r.__setitem__(slice(0x209, 0x20d), name), 'unsafe')

    def test_only_ires(self):
        self.reject(lambda r: r.__setitem__(slice(0x0c, 0x10), b'IRDS'), 'only experimental')

    def test_schema(self):
        entry = manifest.make_entry(fixture())
        for key, value in [('gamecode', 'IREO'), ('validation_version', 0), ('header_crc16_valid', False),
                           ('rom_sha1', 'oops'), ('header_crc16', 7), ('files', {}), ('overlay_table', []), ('overlay_table', {'size': 32, 'sha1': '0' * 40})]:
            with self.subTest(key=key):
                bad = copy.deepcopy(entry)
                bad[key] = value
                with self.assertRaises(ValueError):
                    manifest.validate_entry(bad, 344)
        bad = copy.deepcopy(entry)
        bad['files']['a";bad'] = bad['files'].pop('data')
        with self.assertRaisesRegex(ValueError, 'unsafe'):
            manifest.validate_entry(bad, 344)

    def test_atomic_write_and_preservation(self):
        with tempfile.TemporaryDirectory() as work:
            path = Path(work) / 'manifest.json'
            path.write_text('{"untouched":{"x":1}}')
            original = path.read_bytes()
            with patch.object(manifest.os, 'replace', side_effect=OSError('full device')):
                with self.assertRaises(OSError):
                    manifest.write_manifest(path, manifest.make_entry(fixture()))
            self.assertEqual(path.read_bytes(), original)
            self.assertEqual(sorted(p.name for p in Path(work).iterdir()), ['manifest.json'])
            manifest.write_manifest(path, manifest.make_entry(fixture()))
            self.assertEqual(json.loads(path.read_text())['untouched'], {'x': 1})

    def test_bad_rom_preserves_manifest_cli(self):
        with tempfile.TemporaryDirectory() as work:
            rom = Path(work) / 'broken.nds'
            output = Path(work) / 'manifest.json'
            original = b'{"keep":1}\n'
            output.write_bytes(original)
            bad = fixture()
            struct.pack_into('<I', bad, 0x50, len(bad) + 32)
            fix_crc(bad)
            rom.write_bytes(bad)
            result = subprocess.run([sys.executable, str(SCRIPT), str(rom), str(output)],
                                    capture_output=True, text=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn('overlay table lies outside', result.stderr)
            self.assertEqual(output.read_bytes(), original)
            with patch.object(manifest.json, 'dump', side_effect=OSError('full device')):
                with self.assertRaises(OSError):
                    manifest.write_manifest(output, manifest.make_entry(fixture()))
            self.assertEqual(output.read_bytes(), original)
            self.assertEqual(sorted(p.name for p in Path(work).iterdir()), ['broken.nds', 'manifest.json'])

    def test_duplicate_json_rejected(self):
        with tempfile.TemporaryDirectory() as work:
            path = Path(work) / 'manifest.json'
            path.write_text('{"b2_es":{},"b2_es":{}}')
            with self.assertRaisesRegex(ValueError, 'duplicate JSON'):
                manifest.load_json(path)

    @unittest.skipUnless(shutil.which('cmake'), 'CMake is unavailable')
    def test_production_cmake_fragment(self):
        with tempfile.TemporaryDirectory() as work:
            root = Path(work)
            # A tiny stand-in repository supplies only build inputs; the fragment and validator stay production code.
            assets = root / 'port/tools/research/assets'
            assets.mkdir(parents=True)
            (assets / 'rom_manifest.py').write_bytes(SCRIPT.read_bytes())
            shutil.copy(ROOT / 'port/tools/research/assets/nitrofs_manifest.json', assets)
            for version in ('b2_us', 'w2_us'):
                shutil.copy(ROOT / f'{version}.sha1', root)
                for i in range(344):
                    (root / f'config/{version}/arm9/overlays/ov{i:03}').mkdir(parents=True)
            output = root / 'gen/rom'
            output.mkdir(parents=True)
            probe = root / 'probe.cmake'
            ignored = ('set_property', 'set_target_properties', 'target_sources', 'add_library', 'add_executable', 'target_link_libraries',
                       'target_include_directories', 'target_compile_options', 'target_compile_definitions', 'add_test',
                       'set_tests_properties', 'enable_testing')
            prefix = ''.join(f'function({name})\nendfunction()\n' for name in ignored)
            prefix += (f'set(REPO_ROOT "{root.as_posix()}")\nset(PORT_GEN "{(root / "gen").as_posix()}")\n'
                       f'set(PORT_PYTHON "{Path(sys.executable).as_posix()}")\n'
                       f'set(CMAKE_CURRENT_SOURCE_DIR "{(ROOT / "port").as_posix()}")\n'
                       'set(PORT_GAME_NAME "Black 2")\nset(PORT_ROM_REGION "US")\n')
            def run(language, entry=None, version='b2_us'):
                path = assets / 'nitrofs_manifest_languages.json'
                if entry is None:
                    path.unlink(missing_ok=True)
                else:
                    path.write_text(json.dumps(entry))
                probe.write_text(prefix + f'set(PORT_VERSION {version})\nset(PORT_ROM_LANGUAGE {language})\n'
                                 f'include("{(ROOT / "port/cmake/rom.cmake").as_posix()}")\n')
                return subprocess.run(['cmake', '-P', str(probe)], capture_output=True, text=True)
            result = run('us')
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            us = (output / 'rom_manifest.c').read_bytes()
            self.assertEqual(run('us', {'b2_es': {'invalid': True}}).returncode, 0)
            self.assertEqual((output / 'rom_manifest.c').read_bytes(), us)
            self.assertNotEqual(run('es').returncode, 0)
            self.assertNotEqual(run('es', {'w2_es': {}}).returncode, 0)
            entry = manifest.make_entry(fixture())
            self.assertEqual(run('es', {'b2_es': entry}).returncode, 0)
            self.assertNotEqual(run('es', {'b2_es': entry}, 'w2_us').returncode, 0)
            for key, value in [('gamecode', 'IRDS'), ('header_crc16_valid', False), ('validation_version', 0)]:
                bad = copy.deepcopy(entry)
                bad[key] = value
                self.assertNotEqual(run('es', {'b2_es': bad}).returncode, 0)


if __name__ == '__main__':
    unittest.main()

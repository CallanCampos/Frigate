#!/usr/bin/env python3
"""Check rejection of ELF metadata and paths the veneer rewrite cannot preserve.

    python3 -m unittest discover -s tools/build -p test_restore_dsprot_veneers.py

Real both-version links additionally validate source-byte preservation, complete
symbol/relocation reindexing and exact module images; these tests exercise the
failure paths without requiring a base ROM or Metrowerks installation.
"""
import struct
import tempfile
import unittest
from pathlib import Path

import restore_dsprot_veneers as veneers


def object_fixture():
    """Minimal ET_REL with one global and a REL debug-section reference to it."""
    names = b'\0.symtab\0.strtab\0.shstrtab\0.debug_info\0.rel.debug_info\0'
    blobs = [b'', bytes(16) + struct.pack('<IIIBBH', 1, 0, 0, 0x10, 0, 0),
             b'\0external\0', names, bytes(4), struct.pack('<II', 0, 1 << 8 | 2)]
    headers = [(0, 0, 0, 0, 0, 0, 0, 0, 0, 0)]
    raw = bytearray(struct.pack('<16sHHIIIIIHHHHHH', b'\x7fELF\x01\x01\x01' + bytes(9),
                                1, 40, 1, 0, 0, 0, 0x02100000, 52, 0, 0, 40, 6, 3))
    for index, name, kind, link, info, entry in [(1, b'.symtab', 2, 2, 1, 16),
                                               (2, b'.strtab', 3, 0, 0, 0),
                                               (3, b'.shstrtab', 3, 0, 0, 0),
                                               (4, b'.debug_info', 1, 0, 0, 0),
                                               (5, b'.rel.debug_info', 9, 1, 4, 8)]:
        headers.append((names.index(name), kind, 0, 0, len(raw), len(blobs[index]),
                        link, info, 0, entry))
        raw.extend(blobs[index])
    struct.pack_into('<I', raw, 32, len(raw))
    for header in headers:
        raw.extend(struct.pack('<10I', *header))
    return raw


class VeneerInputTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.path = self.root / 'input.o'

    def parse(self, data):
        self.path.write_bytes(data)
        return veneers.Elf32(self.path, relocatable=True)

    def section_mutation(self, section, field, value):
        data = object_fixture()
        table = struct.unpack_from('<I', data, 32)[0]
        struct.pack_into('<I', data, table + section * 40 + field * 4, value)
        return data

    def test_rel_and_original_serialization(self):
        obj = self.parse(object_fixture())
        self.assertEqual(obj.relocations[5], [(0, 258)])
        self.path.write_bytes(obj.serialize())
        self.assertEqual(veneers.Elf32(self.path, relocatable=True).relocations[5], [(0, 258)])

    def test_symbol_index_metadata_is_rejected(self):
        for kind in (17, 18, 5, 11, 0x70000001, 0x80000000):
            with self.subTest(kind=kind), self.assertRaisesRegex(ValueError, 'unsupported section'):
                self.parse(self.section_mutation(4, 1, kind))
        with self.assertRaisesRegex(ValueError, 'unsupported metadata'):
            self.parse(self.section_mutation(4, 7, 1))

    def test_program_headers_and_extended_indices_are_rejected(self):
        for offset, fmt, value, diagnostic in [(28, '<I', 52, 'without program headers'),
                                                (44, '<H', 1, 'without program headers'),
                                                (48, '<H', 0, 'extended or invalid'),
                                                (50, '<H', 0xffff, 'extended or invalid')]:
            data = object_fixture()
            struct.pack_into(fmt, data, offset, value)
            with self.subTest(offset=offset), self.assertRaisesRegex(ValueError, diagnostic):
                self.parse(data)

    def test_invalid_symbol_and_relocation_tables_are_rejected(self):
        with self.assertRaisesRegex(ValueError, 'partition'):
            self.parse(self.section_mutation(1, 7, 2))
        with self.assertRaisesRegex(ValueError, 'relocation linkage'):
            self.parse(self.section_mutation(5, 6, 2))
        with self.assertRaisesRegex(ValueError, 'relocation entries'):
            self.parse(self.section_mutation(5, 9, 12))
        with self.assertRaisesRegex(ValueError, 'invalid section alignment'):
            self.parse(self.section_mutation(4, 8, 3))

    def test_response_preserves_paths_as_data(self):
        self.assertEqual(veneers.response_objects('"C:\\a folder\\detect.o"\n-overlaygroup OV165,0\n'),
                         ['C:\\a folder\\detect.o'])
        with self.assertRaisesRegex(ValueError, 'unsupported response'):
            veneers.response_objects('"detect.o"\n@another-response.txt\n')

    def test_memory_paths_are_confined(self):
        good = 'MEMORY {\nOV165 : ORIGIN = 0 > build/arm9_ov165.bin\nSPACE : ORIGIN = AFTER(OV165)\n}\n} > OV165\n'
        self.assertEqual(veneers.memory_outputs(good, self.root), [self.root / 'build/arm9_ov165.bin'])
        for path in ('/tmp/escape.bin', 'build/../escape.bin', 'C:/escape.bin'):
            with self.subTest(path=path), self.assertRaisesRegex(ValueError, 'MEMORY line'):
                veneers.memory_outputs(good.replace('build/arm9_ov165.bin', path), self.root)
        with self.assertRaisesRegex(ValueError, 'outside MEMORY'):
            veneers.memory_outputs(good + '} > /tmp/escape.bin\n', self.root)


if __name__ == '__main__':
    unittest.main()

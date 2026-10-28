#!/usr/bin/env python3
"""ROM-free corruption tests of the actual oracle save validator (port/tools/oracle/saveimage.py).

Synthetic full card images follow src/system/save_table.c's two main copies and src/gfl/backup_system.c's block
footers, CRC table and SaveFooter. These checks protect SAVE/interchange validation: a good backup cannot hide a
bad trainer CRC in the other copy, even where an oracle route permits clock/CRC differences. No game data or saves.
"""

from __future__ import annotations

import contextlib
import hashlib
import io
from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[1] / "tools" / "oracle"))
sys.path.insert(0, str(HERE.parent / "replay"))

import saveimage  # noqa: E402
import check_oracle_save as replay  # noqa: E402


def refresh_table(image: bytearray, copy: int) -> None:
    crc = saveimage.crc16_ccitt(image[copy + saveimage.CRC_TABLE:copy + saveimage.SAVE_FOOTER])
    struct.pack_into("<H", image, copy + saveimage.SAVE_FOOTER + 14, crc)


def refresh_block(image: bytearray, blocks: list[tuple[int, int]], copy: int, index: int) -> None:
    start, size = blocks[index]
    crc = saveimage.crc16_ccitt(image[copy + start:copy + start + size])
    struct.pack_into("<HH", image, copy + start + size, 1, crc)
    struct.pack_into("<H", image, copy + saveimage.CRC_TABLE + 2 * index, crc)
    refresh_table(image, copy)


def make_image() -> tuple[bytes, list[tuple[int, int]]]:
    image = bytearray(b"\xff" * saveimage.BACKUP_SIZE)
    # Deliberately synthetic sizes, with the same source-defined alignment and fixed table/footer positions.
    # One large first block and 72 one-page blocks exactly fill the main save's block area.
    first_size = saveimage.CRC_TABLE - (saveimage.MAIN_BLOCK_COUNT - 1) * saveimage.BLOCK_ALIGN - 4
    blocks = [(0, first_size)] + [(first_size + 4 + i * 0x100, 0xfc)
                                  for i in range(saveimage.MAIN_BLOCK_COUNT - 1)]
    for copy in saveimage.COPIES:
        for index, (start, size) in enumerate(blocks):
            image[copy + start:copy + start + size] = bytes([index]) * size
            crc = saveimage.crc16_ccitt(image[copy + start:copy + start + size])
            struct.pack_into("<HH", image, copy + start + size, 1, crc)
            struct.pack_into("<H", image, copy + saveimage.CRC_TABLE + 2 * index, crc)
        # There are 73 CRCs and one padded u16. SaveData_CalcBlockCrcsCrc includes that padding.
        struct.pack_into("<H", image, copy + saveimage.CRC_TABLE + 2 * saveimage.MAIN_BLOCK_COUNT, 0)
        struct.pack_into("<IIIHH", image, copy + saveimage.SAVE_FOOTER,
                         1, saveimage.MAIN_SAVE_SIZE, saveimage.SAVE_MAGIC, 0, 0)
        refresh_table(image, copy)
    return bytes(image), blocks


class SaveImageTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.valid, cls.blocks = make_image()

    def assert_broken(self, image: bytes, text: str):
        problems = saveimage.validate_image(image, reference=self.valid)
        self.assertTrue(problems)
        self.assertTrue(any(text in problem for problem in problems), problems)

    def test_known_crc_vector(self):
        self.assertEqual(saveimage.crc16_ccitt(b""), 0xffff)
        self.assertEqual(saveimage.crc16_ccitt(b"123456789"), 0x29b1)
        self.assertEqual(saveimage._crc16_update(0x29b1, b""), 0x29b1)
        # CRC-16/CCITT-FALSE's known check value must hold across every split, including empty updates.
        # Discovery carries this state between chunks; resetting it would pass the whole-message vector above.
        for split in range(10):
            with self.subTest(split=split):
                first = saveimage._crc16_update(0xffff, b"123456789"[:split])
                self.assertEqual(saveimage._crc16_update(first, b"123456789"[split:]), 0x29b1)

    def test_full_valid_image_and_reference(self):
        self.assertEqual(saveimage.validate_image(self.valid), [])
        self.assertEqual(saveimage.validate_image(self.valid, reference=self.valid), [])
        self.assertEqual(saveimage.MAIN_BLOCK_COUNT, 73)
        self.assertEqual(saveimage.CRC_TABLE_SIZE, 0x94)
        self.assertEqual(saveimage.SAVE_FOOTER, 0x25f94)
        self.assertEqual(saveimage.MAIN_SAVE_SIZE, 0x25fa4)

    def test_each_copy_bad_trainer_footer_crc_never_falls_back(self):
        for copy in saveimage.COPIES:
            with self.subTest(copy=copy):
                image = bytearray(self.valid)
                start, size = self.blocks[27]
                image[copy + start + size + 2] ^= 1
                self.assert_broken(bytes(image), f"copy {copy:#x} block 27")

    def test_round_trip_rejects_masked_crc_before_decryption_or_boot(self):
        # Exercise the real command, not just the validator: even a permitted CRC byte must be valid in each bank.
        oracle = replay.oracle
        for copy in saveimage.COPIES:
            with self.subTest(copy=copy), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                reference = root / "reference.sav"
                reference.write_bytes(self.valid)
                binary = root / "oracle"
                binary.touch()
                changed = bytearray(self.valid)
                start, size = self.blocks[27]
                bad_offset = copy + start + size + 2
                changed[bad_offset] ^= 1
                trip = oracle.RoundTrip("floccesy_save", 34600, "floccesy_continue",
                                        known={"b2_us": {hex(bad_offset): "a permitted CRC byte"}})

                def make_native(*args, **kwargs):
                    Path(args[1]).mkdir(parents=True)
                    Path(kwargs["save_out"]).write_bytes(changed)
                    return 0, "synthetic run"

                args = SimpleNamespace(name="unit-crc", version="b2_us", oracle=str(binary), rom=None,
                                       port="synthetic", time=None, work=str(root / "run"), save_cache=None)
                output = io.StringIO()
                with mock.patch.dict(oracle.ROUND_TRIPS, {"unit-crc": trip}), \
                     mock.patch.object(oracle, "rom_matches", return_value=None), \
                     mock.patch.object(oracle, "obtain_save", return_value=(str(reference), None)), \
                     mock.patch.object(oracle, "run_port", side_effect=make_native), \
                     mock.patch.object(oracle, "describe_port_run", return_value="synthetic run"), \
                     mock.patch.object(oracle, "run_oracle") as boot, \
                     mock.patch.object(oracle, "mystery_gift_decrypted") as decrypt, \
                     contextlib.redirect_stdout(output):
                    self.assertEqual(oracle.command_round_trip(args), 1)
                boot.assert_not_called()
                decrypt.assert_not_called()
                self.assertIn("invalid save image", output.getvalue())
                self.assertIn(f"copy {copy:#x} block 27", output.getvalue())

    def test_each_copy_bad_payload_never_falls_back(self):
        for copy in saveimage.COPIES:
            with self.subTest(copy=copy):
                image = bytearray(self.valid)
                image[copy + self.blocks[27][0]] ^= 1
                self.assert_broken(bytes(image), f"copy {copy:#x} block 27")

    def test_all_three_blocks_beyond_legacy_references_are_checked(self):
        for index in (70, 71, 72):
            with self.subTest(index=index):
                image = bytearray(self.valid)
                image[saveimage.COPY_SIZE + self.blocks[index][0]] ^= 1
                self.assert_broken(bytes(image), f"block {index}")

    def test_valid_clock_change_with_recomputed_block_and_table_crcs(self):
        image = bytearray(self.valid)
        for copy in saveimage.COPIES:
            image[copy + self.blocks[27][0] + 0x27] ^= 1
            refresh_block(image, self.blocks, copy, 27)
        self.assertNotEqual(bytes(image), self.valid)
        self.assertEqual(saveimage.validate_image(bytes(image), reference=self.valid), [])

    def test_bad_table_entry_with_repaired_table_checksum_still_fails(self):
        image = bytearray(self.valid)
        image[saveimage.COPY_SIZE + saveimage.CRC_TABLE + 27 * 2] ^= 1
        refresh_table(image, saveimage.COPY_SIZE)
        self.assert_broken(bytes(image), "block 27")

    def test_bad_table_checksum_and_padding_are_checked(self):
        image = bytearray(self.valid)
        image[saveimage.SAVE_FOOTER + 14] ^= 1
        self.assert_broken(bytes(image), "CRC table checksum")
        image = bytearray(self.valid)
        image[saveimage.CRC_TABLE + 2 * saveimage.MAIN_BLOCK_COUNT] ^= 1
        self.assert_broken(bytes(image), "CRC table checksum")
        refresh_table(image, 0)
        self.assertEqual(saveimage.validate_image(bytes(image), reference=self.valid), [])

    def test_footer_size_and_magic_are_checked_in_each_copy(self):
        for copy in saveimage.COPIES:
            for offset, name in ((4, "size"), (8, "magic")):
                with self.subTest(copy=copy, field=name):
                    image = bytearray(self.valid)
                    image[copy + saveimage.SAVE_FOOTER + offset] ^= 1
                    self.assert_broken(bytes(image), f"save footer {name}")

    def test_uncovered_footer_fields_and_extra_card_area_are_not_invented_checks(self):
        image = bytearray(self.valid)
        image[saveimage.SAVE_FOOTER] ^= 1  # SaveFooter count is not part of the CRC table checksum.
        image[saveimage.SAVE_FOOTER + 12] ^= 1  # The source leaves unkC unused.
        image[saveimage.AFTER_COPIES] ^= 1  # Independently loaded extra saves are outside this validator's scope.
        self.assertEqual(saveimage.validate_image(bytes(image), reference=self.valid), [])

    def test_short_long_and_short_reference_fail_explicitly(self):
        for image in (self.valid[:-1], self.valid + b"\x00", b""):
            with self.subTest(length=len(image)):
                self.assertIn("length", saveimage.validate_image(image)[0])
        self.assertIn("reference image length", saveimage.validate_image(self.valid, reference=b"")[0])

    def test_reference_extents_prevent_native_footer_relocation(self):
        image = bytearray(self.valid)
        start, size = self.blocks[27]
        # A self-consistent fake block four bytes shorter still occupies the same aligned page. Native inference
        # certifies both banks of this layout; the trusted DS reference must keep the original footer extent.
        for copy in saveimage.COPIES:
            new_size = size - 4
            crc = saveimage.crc16_ccitt(image[copy + start:copy + start + new_size])
            struct.pack_into("<HH", image, copy + start + new_size, 1, crc)
            image[copy + start + size:copy + start + size + 4] = b"\xff" * 4
            struct.pack_into("<H", image, copy + saveimage.CRC_TABLE + 27 * 2, crc)
            refresh_table(image, copy)
        self.assertEqual(saveimage.validate_image(bytes(image)), [])
        self.assert_broken(bytes(image), "block 27")

    def test_bad_reference_range_cannot_be_used(self):
        reference = bytearray(self.valid)
        # Certify a fake first footer in the first aligned page instead of after the large first block. The
        # remaining reference CRCs cannot describe 73 contiguous source-aligned blocks ending at CRC_TABLE.
        crc = saveimage.crc16_ccitt(reference[:4])
        struct.pack_into("<HH", reference, 4, 1, crc)
        struct.pack_into("<H", reference, saveimage.CRC_TABLE, crc)
        refresh_table(reference, 0)
        problems = saveimage.validate_image(self.valid, reference=bytes(reference))
        self.assertTrue(problems)
        self.assertTrue(problems[0].startswith("reference"), problems)

    def test_all_zero_image_has_explicit_layout_diagnostic(self):
        self.assertTrue(saveimage.validate_image(bytes(saveimage.BACKUP_SIZE))[0].startswith("save layout:"))

    def test_bad_reference_other_copy_is_rejected(self):
        reference = bytearray(self.valid)
        reference[saveimage.COPY_SIZE + self.blocks[27][0]] ^= 1
        self.assertTrue(saveimage.validate_image(self.valid, reference=bytes(reference))[0].startswith("reference"))

    def test_zero_block_save_count_is_not_an_invalid_crc(self):
        image = bytearray(self.valid)
        start, size = self.blocks[27]
        for copy in saveimage.COPIES:
            struct.pack_into("<H", image, copy + start + size, 0)
        self.assertEqual(saveimage.validate_image(bytes(image)), [])
        self.assertEqual(saveimage.validate_image(bytes(image), reference=self.valid), [])
        self.assertEqual(saveimage.find_blocks(bytes(image)), self.blocks)
        # The reference writer must accept the same valid wrapped count as the integrity checker.
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            saved = path / "wrapped.sav"
            saved.write_bytes(image)
            reference = path / "synthetic_b2_us.save.tsv"
            maker = SimpleNamespace(sha1={"b2_us": hashlib.sha1(image).hexdigest()})
            with mock.patch.dict(replay.oracle.SAVES, {"synthetic": maker}), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(replay.write_reference(str(reference), str(saved)), 0)
            rows = replay.read_reference(str(reference))
            self.assertEqual(sum(row[0] != "after" for row in rows), 73)
            self.assertEqual(replay.check_save(bytes(image), rows), [])

    def test_replay_reexports_and_new_reference_output_covers_73_blocks(self):
        self.assertIs(replay.find_blocks, saveimage.find_blocks)
        self.assertIs(replay.crc16_ccitt, saveimage.crc16_ccitt)
        self.assertEqual(replay.find_blocks(self.valid), self.blocks)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            image = path / "synthetic.sav"
            image.write_bytes(self.valid)
            reference = path / "synthetic_b2_us.save.tsv"
            maker = SimpleNamespace(sha1={"b2_us": hashlib.sha1(self.valid).hexdigest()})
            with mock.patch.dict(replay.oracle.SAVES, {"synthetic": maker}), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(replay.write_reference(str(reference), str(image)), 0)
            rows = replay.read_reference(str(reference))
            self.assertEqual([row[0] for row in rows], [str(i) for i in range(73)] + ["after"])
            self.assertEqual(replay.check_save(self.valid, rows), [])
            legacy_rows = [row for row in rows if row[0] == "after" or int(row[0]) < 70]
            self.assertEqual(replay.check_save(self.valid, legacy_rows), [])
            broken = bytearray(self.valid)
            broken[saveimage.COPY_SIZE + self.blocks[72][0]] ^= 1
            self.assertTrue(any("block 72" in error for error in replay.check_save(bytes(broken), legacy_rows)))
            image.write_bytes(broken)
            maker.sha1["b2_us"] = hashlib.sha1(broken).hexdigest()
            with mock.patch.dict(replay.oracle.SAVES, {"synthetic": maker}), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(replay.write_reference(str(reference), str(image)), 1)


if __name__ == "__main__":
    unittest.main()

#!/usr/bin/env python3
"""Checks BW2's two main save copies without the ROM, before an oracle comparison masks clock/CRC bytes.

The owners are src/system/save_table.c (73 blocks, copy offsets and magic), src/gfl/savedata.c
(SaveData_BuildTable's alignment), and src/gfl/backup_system.c (BlockFooter, SaveFooter and their CRC coverage).
This is stricter than the game's recovery loader: a good copy never excuses a broken other copy. The rest of the
card contains separately loaded extra saves; only its length, not those extra saves' contents, is checked here.

Pass the SHA-checked DS image as reference when validating a native image. Block extents are discovered only from
that reference, so a corrupt native CRC cannot move a footer. CRC discovery is not proof of authenticity or a
replacement for the oracle's examined SHA. The replay checker re-exports the same helpers; older 70-row reference
files still check their recorded content hashes, while image integrity and newly written references cover all 73
main-save blocks. Standard library only.
"""

from __future__ import annotations

import struct

BACKUP_SIZE = 0x80000
COPY_SIZE = 0x26000
COPIES = (0, COPY_SIZE)
AFTER_COPIES = 2 * COPY_SIZE
CRC_TABLE = 0x25f00
# include/save/save_control.h SAVE_BLOCK_MAX, including the final Funfest, Pokewood and Dream Radar blocks.
BLOCK_COUNT = 73
MAIN_BLOCK_COUNT = BLOCK_COUNT
BLOCK_ALIGN = 0x100
BLOCK_FOOTER_SIZE = 4
# SaveData_Init rounds an odd number of u16 CRCs up to an even number; the padding IS covered by the table CRC.
CRC_TABLE_SIZE = ((MAIN_BLOCK_COUNT + 1) // 2) * 4
SAVE_FOOTER_SIZE = 0x10
SAVE_FOOTER = CRC_TABLE + CRC_TABLE_SIZE
MAIN_SAVE_SIZE = SAVE_FOOTER + SAVE_FOOTER_SIZE
SAVE_MAGIC = 0x31053527


def _crc16_update(crc: int, data: bytes) -> int:
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xffff if crc & 0x8000 else (crc << 1) & 0xffff
    return crc


def crc16_ccitt(data: bytes) -> int:
    """CRC-16/CCITT-FALSE, as NitroSDK's MATH_CalcCRC16CCITT computes it."""
    return _crc16_update(0xffff, data)


def _discover_blocks(save: bytes, count: int) -> list[tuple[int, int]]:
    blocks = []
    start = 0
    for index in range(count):
        crc = struct.unpack_from("<H", save, CRC_TABLE + 2 * index)[0]
        size = 0
        actual = 0xffff
        while True:
            if start + size + BLOCK_FOOTER_SIZE > CRC_TABLE:
                raise ValueError(f"block {index}: no footer with its CRC {crc:#06x} after {start:#x}")
            footer = struct.unpack_from("<HH", save, start + size)
            if footer[1] == crc and actual == crc:
                break
            # Keep discovery linear even for a broken all-zero reference with many candidate footers.
            actual = _crc16_update(actual, save[start + size:start + size + 4])
            size += 4
        blocks.append((start, size))
        start = (start + size + BLOCK_FOOTER_SIZE + BLOCK_ALIGN - 1) // BLOCK_ALIGN * BLOCK_ALIGN
    return blocks


def find_blocks(save: bytes) -> list[tuple[int, int]]:
    """All 73 main blocks from an examined image, including a u16 save count which has wrapped to zero."""
    return _main_layout(save)


def _main_layout(reference: bytes) -> list[tuple[int, int]]:
    # A block's u16 save count may wrap to zero; the game's CRC checks do not reject that count.
    blocks = _discover_blocks(reference, MAIN_BLOCK_COUNT)
    end, size = blocks[-1]
    end = (end + size + BLOCK_FOOTER_SIZE + BLOCK_ALIGN - 1) // BLOCK_ALIGN * BLOCK_ALIGN
    if end != CRC_TABLE:
        raise ValueError(f"blocks end at {end:#x}, not the main save's CRC table at {CRC_TABLE:#x}")
    return blocks


def _validate_copies(data: bytes, blocks: list[tuple[int, int]]) -> list[str]:
    problems = []
    for copy in COPIES:
        prefix = f"copy {copy:#x}"
        for index, (start, size) in enumerate(blocks):
            actual = crc16_ccitt(data[copy + start:copy + start + size])
            _, footer_crc = struct.unpack_from("<HH", data, copy + start + size)
            table_crc = struct.unpack_from("<H", data, copy + CRC_TABLE + 2 * index)[0]
            if actual != footer_crc:
                problems.append(f"{prefix} block {index}: data CRC {actual:#06x} != footer CRC {footer_crc:#06x}")
            if actual != table_crc:
                problems.append(f"{prefix} block {index}: data CRC {actual:#06x} != table CRC {table_crc:#06x}")
        # SaveFooter is count:u32, size:u32, magic:u32, unused:u16, crc:u16. Only the CRC table (including its
        # two-byte padding), not the SaveFooter itself or the unused card area, is covered by this last CRC.
        _, size, magic, _, footer_crc = struct.unpack_from("<IIIHH", data, copy + SAVE_FOOTER)
        if size != MAIN_SAVE_SIZE:
            problems.append(f"{prefix}: save footer size {size:#x} != {MAIN_SAVE_SIZE:#x}")
        if magic != SAVE_MAGIC:
            problems.append(f"{prefix}: save footer magic {magic:#x} != {SAVE_MAGIC:#x}")
        actual = crc16_ccitt(data[copy + CRC_TABLE:copy + SAVE_FOOTER])
        if actual != footer_crc:
            problems.append(f"{prefix}: CRC table checksum {actual:#06x} != save footer CRC {footer_crc:#06x}")
    return problems


def validate_image(data: bytes, reference: bytes | None = None) -> list[str]:
    """Diagnostics for either broken main-save copy; [] only when both copies and their table/footer checks pass.

    reference is the trusted, SHA-checked DS image, not an inferred layout from the native image being tested.
    An invalid reference is also an error. Without reference, discover the layout from data's first copy (for
    checking the examined DS image itself). Neither mode repairs a copy or falls back to the other bank.
    """
    if len(data) != BACKUP_SIZE:
        return [f"save image length {len(data):#x} != {BACKUP_SIZE:#x}"]
    layout_image = data if reference is None else reference
    if len(layout_image) != BACKUP_SIZE:
        return [f"reference image length {len(layout_image):#x} != {BACKUP_SIZE:#x}"]
    try:
        blocks = _main_layout(layout_image)
    except (ValueError, struct.error) as error:
        return [f"{'save' if reference is None else 'reference'} layout: {error}"]
    if reference is not None:
        problems = _validate_copies(reference, blocks)
        if problems:
            return [f"reference {problem}" for problem in problems]
    return _validate_copies(data, blocks)

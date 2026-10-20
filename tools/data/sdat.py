"""Read and write Nitro sound archives (SDAT): their symbols, info records and files.

An archive has eight kinds of entries, in KINDS' order. Each kind is a list of slots, each an entry's name (or None)
and info record (or None for an empty slot), and the files are a list in the archive's order, which the records name
by their place in it (the first two bytes of a sequence's, bank's, wave archive's or stream's record).
"""
import struct

KINDS = ("sequences", "sequence_archives", "banks", "wave_archives", "players", "groups", "stream_players", "streams")
# The kinds whose records start with a file ID
FILE_KINDS = ("sequences", "sequence_archives", "banks", "wave_archives", "streams")
FILE_ALIGN = 32


def align(value: int, to: int) -> int:
    return value + (-value % to)


def read(data: bytes) -> dict:
    """Returns an archive's names, records and files."""
    if data[:4] != b"SDAT":
        raise ValueError("not a Nitro sound archive")
    symb, _, info, _, fat, _, file_block, _ = struct.unpack_from("<8I", data, 0x10)
    names = {}
    if symb:
        offsets = struct.unpack_from("<8I", data, symb + 8)
        for kind, offset in zip(KINDS, offsets):
            count = struct.unpack_from("<I", data, symb + offset)[0]
            pointers = struct.unpack_from(f"<{count}I", data, symb + offset + 4)
            names[kind] = [data[symb + p:data.index(b"\0", symb + p)].decode("ascii") if p else None
                           for p in pointers]
    records = {}
    offsets = struct.unpack_from("<8I", data, info + 8)
    ends = list(offsets[1:]) + [struct.unpack_from("<I", data, info + 4)[0]]
    for kind, offset, end in zip(KINDS, offsets, ends):
        count = struct.unpack_from("<I", data, info + offset)[0]
        pointers = struct.unpack_from(f"<{count}I", data, info + offset + 4)
        starts = sorted(p for p in pointers if p) + [end]
        sizes = {start: following - start for start, following in zip(starts, starts[1:])}
        records[kind] = [data[info + p:info + p + sizes[p]] if p else None for p in pointers]
    count = struct.unpack_from("<I", data, fat + 8)[0]
    files = []
    for index in range(count):
        offset, size = struct.unpack_from("<II", data, fat + 12 + 16 * index)
        files.append(data[offset:offset + size])
    return {"names": names, "records": records, "files": files}


def symbol_block(names: dict) -> tuple[bytes, int]:
    """Returns the SYMB block, padded, and its size before padding."""
    tables = []
    position = 0x40
    for kind in KINDS:
        tables.append(position)
        position += 4 + 4 * len(names[kind])
    strings = bytearray()
    pointers = {}
    for kind in KINDS:
        pointers[kind] = []
        for name in names[kind]:
            if name is None:
                pointers[kind].append(0)
                continue
            pointers[kind].append(position + len(strings))
            strings += name.encode("ascii") + b"\0"
    size = position + len(strings)
    block = bytearray(struct.pack("<4sI8I", b"SYMB", align(size, 4), *tables) + bytes(0x40 - 0x28))
    for kind in KINDS:
        block += struct.pack(f"<I{len(pointers[kind])}I", len(pointers[kind]), *pointers[kind])
    block += strings
    return bytes(block) + bytes(-size % 4), size


def info_block(records: dict) -> bytes:
    body = bytearray()
    tables = []
    for kind in KINDS:
        slots = records[kind]
        position = 0x40 + len(body)
        tables.append(position)
        cursor = position + 4 + 4 * len(slots)
        pointers = []
        for record in slots:
            pointers.append(cursor if record is not None else 0)
            cursor += len(record) if record is not None else 0
        body += struct.pack(f"<I{len(slots)}I", len(slots), *pointers)
        body += b"".join(record for record in slots if record is not None)
    size = 0x40 + len(body)
    return struct.pack("<4sI8I", b"INFO", size, *tables) + bytes(0x40 - 0x28) + bytes(body)


def write(archive: dict) -> bytes:
    """Returns the archive of the names, records and files, laid out as the game's is."""
    names, records, files = archive["names"], archive["records"], archive["files"]
    symb, symb_size = symbol_block(names) if names else (b"", 0)
    info = info_block(records)
    symb_offset = 0x40
    info_offset = symb_offset + len(symb)
    fat_offset = info_offset + len(info)
    fat_size = 12 + 16 * len(files)
    file_offset = fat_offset + fat_size
    entries = []
    body = bytearray()
    cursor = file_offset + 0x10
    for data in files:
        position = align(cursor, FILE_ALIGN)
        body += bytes(position - cursor) + data
        entries.append((position, len(data)))
        cursor = position + len(data)
    body += bytes(-cursor % FILE_ALIGN)
    file_size = 0x10 + len(body)
    fat = struct.pack("<4sII", b"FAT ", fat_size, len(files)) + b"".join(
        struct.pack("<IIII", offset, size, 0, 0) for offset, size in entries)
    file_block = struct.pack("<4sIII", b"FILE", file_size, len(files), 0) + bytes(body)
    total = file_offset + file_size
    header = struct.pack("<4sHHIHH8I", b"SDAT", 0xFEFF, 0x0100, total, 0x40, 4, symb_offset if names else 0, symb_size,
                         info_offset, len(info), fat_offset, fat_size, file_offset, file_size)
    return header + bytes(0x40 - len(header)) + symb + info + fat + file_block

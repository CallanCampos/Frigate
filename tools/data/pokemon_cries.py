#!/usr/bin/env python3
"""Export and pack Pokémon cries as WAV plus cry_volume in data.json.

    pokemon_cries.py extract data/pokemon
    pokemon_cries.py pack data/pokemon black2|white2 SDAT VOLUME_ARCHIVE
"""

import io
import json
import struct
import sys
import wave
from pathlib import Path

from datajson import write
from narc import read_narc, write_narc
from pokemon_graphics import ROOT, species_names


# The forms with cries of their own, after the species' (POKE_VOICE_EXTRA_ENTRIES, pm_voice.c)
FORMS = (("shaymin", 1), ("kyurem", 1), ("kyurem", 2),
         ("tornadus", 1), ("thundurus", 1), ("landorus", 1))


def cry_folder(root, cry_id):
    """Returns the directory of a cry: each species' its own, the egg's, the bad egg's and the Pokéstar Studios
    species' too, then the forms'."""
    names = species_names()
    if cry_id < len(names):
        return root / names[cry_id]
    species, form = FORMS[cry_id - len(names)]
    return root / species / f"form_{form}"


def volume_path(root, cry_id):
    """Returns the data.json with a species' cry_volume (a/1/9/1, which has an entry per species)."""
    return root / species_names()[cry_id] / "data.json"


def sound_tables(data):
    if data[:4] != b"SDAT":
        raise ValueError("expected Nitro sound archive")
    info, fat, file_section = (struct.unpack_from("<I", data, offset)[0]
                               for offset in (24, 32, 40))
    if data[info:info + 4] != b"INFO" or data[fat:fat + 4] != b"FAT " or data[file_section:file_section + 4] != b"FILE":
        raise ValueError("invalid Nitro sound sections")
    table = info + struct.unpack_from("<I", data, info + 20)[0]
    count = struct.unpack_from("<I", data, fat + 8)[0]
    entries = [struct.unpack_from("<II", data, fat + 12 + 16 * i) for i in range(count)]
    cries = {}
    for cry_id in range(1, 691):
        relative = struct.unpack_from("<I", data, table + 4 + cry_id * 4)[0]
        if not relative:
            raise ValueError(f"cry {cry_id} has no wave archive")
        file_id = struct.unpack_from("<I", data, info + relative)[0]
        cries[cry_id] = file_id
    if len(set(cries.values())) != len(cries):
        raise ValueError("Pokémon cries share sound file IDs")
    return fat, file_section, entries, cries


def cry_wave(swar):
    if swar[:4] != b"SWAR" or struct.unpack_from("<I", swar, 0x38)[0] != 1:
        raise ValueError("expected a one-wave Pokémon SWAR")
    offset = struct.unpack_from("<I", swar, 0x3c)[0]
    fmt, loop, rate, _, start, length = struct.unpack_from("<BBHHHI", swar, offset)
    if fmt != 0 or loop or start or offset + 12 + length * 4 != len(swar):
        raise ValueError("expected an unlooped PCM8 Pokémon cry")
    return rate, swar[offset + 12:]


def wav_bytes(rate, samples):
    out = io.BytesIO()
    with wave.open(out, "wb") as file:
        file.setnchannels(1)
        file.setsampwidth(1)
        file.setframerate(rate)
        file.writeframes(bytes(value ^ 0x80 for value in samples))
    return out.getvalue()


def read_wav(path):
    with wave.open(str(path), "rb") as file:
        if file.getnchannels() != 1 or file.getcomptype() != "NONE" or file.getsampwidth() not in (1, 2):
            raise ValueError(f"{path}: expected mono PCM8 or PCM16 WAV")
        rate = file.getframerate()
        width = file.getsampwidth()
        samples = file.readframes(file.getnframes())
    if not 4000 <= rate <= 65535:
        raise ValueError(f"{path}: sample rate exceeds Nitro range")
    if width == 1:
        return rate, bytes(value ^ 0x80 for value in samples)
    signed = struct.iter_unpack("<h", samples)
    return rate, bytes(max(-128, min(127, round(value / 256))) & 255 for (value,) in signed)


def patch_wave(swar, rate, samples):
    result = bytearray(swar[:struct.unpack_from("<I", swar, 0x3c)[0] + 12])
    offset = struct.unpack_from("<I", result, 0x3c)[0]
    result.extend(samples)
    result.extend(b"\0" * (-len(samples) % 4))
    if len(result) > 0x5c30:
        raise ValueError("edited cry exceeds the game's 0x5c30-byte playback buffer")
    struct.pack_into("<H", result, offset + 2, rate)
    struct.pack_into("<H", result, offset + 4, round(33513982 / (2 * rate)))
    struct.pack_into("<I", result, offset + 8, (len(samples) + 3) // 4)
    struct.pack_into("<I", result, 8, len(result))
    struct.pack_into("<I", result, 20, len(result) - 16)
    return bytes(result)


def replace_files(data, fat, file_section, entries, changes):
    if not changes:
        return data
    order = sorted(range(len(entries)), key=lambda index: entries[index][0])
    first = entries[order[0]][0]
    if first < file_section + 16:
        raise ValueError("sound files overlap their header")
    result = bytearray(data[:first])
    cursor = first
    for file_id in order:
        offset, size = entries[file_id]
        if offset < cursor:
            raise ValueError("overlapping sound files")
        if offset != first:
            result.extend(b"\0" * (-len(result) % 32))
        new_offset = len(result)
        member = changes.get(file_id, data[offset:offset + size])
        result.extend(member)
        struct.pack_into("<II", result, fat + 12 + 16 * file_id, new_offset, len(member))
        cursor = offset + size
    result.extend(data[cursor:])
    # The FILE block's size is both its own and the header's (at 0x2c), and the archive's total size is at 8
    struct.pack_into("<I", result, file_section + 4, len(result) - file_section)
    struct.pack_into("<I", result, 0x2c, len(result) - file_section)
    struct.pack_into("<I", result, 8, len(result))
    return bytes(result)


def export(root):
    sound = (ROOT / "extract/b2_us/files/swan_sound_data.sdat").read_bytes()
    _, _, entries, cries = sound_tables(sound)
    volume = read_narc((ROOT / "extract/b2_us/files/a/1/9/1").read_bytes())
    for cry_id, file_id in cries.items():
        folder = cry_folder(root, cry_id)
        folder.mkdir(parents=True, exist_ok=True)
        offset, size = entries[file_id]
        rate, samples = cry_wave(sound[offset:offset + size])
        (folder / "cry.wav").write_bytes(wav_bytes(rate, samples))
        if cry_id < len(volume):
            path = volume_path(root, cry_id)
            data = json.loads(path.read_text()) if path.exists() else {}
            data["cry_volume"] = struct.unpack_from("<H", volume[cry_id], 2)[0]
            write(path, data)
    print(f"{len(cries)} cries", flush=True)


def pack(root, game, sound_output, volume_output):
    version = {"black2": "b2_us", "white2": "w2_us"}[game]
    sound = (ROOT / "extract" / version / "files/swan_sound_data.sdat").read_bytes()
    fat, file_section, entries, cries = sound_tables(sound)
    changes = {}
    for cry_id, file_id in cries.items():
        offset, size = entries[file_id]
        original = sound[offset:offset + size]
        rate, samples = read_wav(cry_folder(root, cry_id) / "cry.wav")
        if (rate, samples) != cry_wave(original):
            changes[file_id] = patch_wave(original, rate, samples)
    result = replace_files(sound, fat, file_section, entries, changes)
    target = Path(sound_output)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.unlink(missing_ok=True)
    target.write_bytes(result)
    volume = read_narc((ROOT / "extract" / version / "files/a/1/9/1").read_bytes())
    for cry_id in range(1, len(volume)):
        path = volume_path(root, cry_id)
        value = json.loads(path.read_text())["cry_volume"]
        if type(value) is not int or not 0 <= value <= 127:
            raise ValueError(f"{path}: cry_volume must be an integer from 0 to 127")
        entry = bytearray(volume[cry_id])
        struct.pack_into("<H", entry, 2, value)
        volume[cry_id] = bytes(entry)
    target = Path(volume_output)
    target.parent.mkdir(parents=True, exist_ok=True)
    target.unlink(missing_ok=True)
    target.write_bytes(write_narc(volume))


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "extract":
        export(Path(sys.argv[2]))
    elif len(sys.argv) == 6 and sys.argv[1] == "pack":
        pack(Path(sys.argv[2]), sys.argv[3], sys.argv[4], sys.argv[5])
    else:
        raise SystemExit(__doc__)

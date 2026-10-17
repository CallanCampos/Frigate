#!/usr/bin/env python3
"""Export Pokémon cries as WAV plus cry_volume in data.json, and pack the volumes.

    pokemon_cries.py extract data/pokemon
    pokemon_cries.py pack data/pokemon black2|white2 VOLUME_ARCHIVE

The cries' wave archives are packed into the sound archive by sound_data.py, which takes them from cry.wav.
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


def pack(root, game, volume_output):
    version = {"black2": "b2_us", "white2": "w2_us"}[game]
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
    elif len(sys.argv) == 5 and sys.argv[1] == "pack":
        pack(Path(sys.argv[2]), sys.argv[3], sys.argv[4])
    else:
        raise SystemExit(__doc__)

#!/usr/bin/env python3
"""The name input's lists of profane words, res/name_input/profane_words.json (ARCID_PROF_WORD, a/2/1/6): dump writes it
from the ROM once, and pack builds the archive from it, which the build does.

    profane_word_data.py dump extract/b2_us/files res/name_input/profane_words.json
    profane_word_data.py pack res/name_input/profane_words.json ARCHIVE

The archive has PROF_WORD_LIST_COUNT lists, each a run of words of PROF_WORD_LEN characters with a terminator, 0xffff,
encrypted as one with _decryptData from PROF_WORD_SEED (prof_word.c). ProfWord_Check rejects a name that is one of
the words of any list, after folding katakana to hiragana and Latin letters to capitals, so the words are written in
those. The characters after a word's terminator are the original's leftovers, which the game doesn't read: "padding"
keeps them for each word, in hex, and the packer cuts them or fills them with 0 to the space a word leaves.

Both versions have the same file. profane_words.schema.json describes the fields.
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.datajson import DataError, label, load, load_schema, write  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

LIST_COUNT = 7  # PROF_WORD_LIST_COUNT
WORD_LEN = 32  # PROF_WORD_LEN
SEED = 0x72012891  # PROF_WORD_SEED
TERMINATOR = 0xFFFF


def crypt(words: list[int]) -> list[int]:
    """decryptData: each u16 XORed with the next of crypt_SAV_BVideo_MG_misc_data's numbers, which also encrypts."""
    seed = SEED
    out = []
    for word in words:
        seed = (seed * 1103515245 + 24691) & 0xFFFFFFFF
        out.append(word ^ seed >> 16)
    return out


def list_json(data: bytes) -> dict:
    if len(data) % (2 * WORD_LEN):
        raise ValueError(f"a list of {len(data)} bytes isn't whole words")
    plain = crypt(list(struct.unpack(f"<{len(data) // 2}H", data)))
    words, padding = [], []
    for start in range(0, len(plain), WORD_LEN):
        slot = plain[start:start + WORD_LEN]
        if TERMINATOR not in slot:
            raise ValueError(f"word {start // WORD_LEN} has no terminator")
        end = slot.index(TERMINATOR)
        if any(0xD800 <= c < 0xE000 or c < 0x20 for c in slot[:end]):
            raise ValueError(f"word {start // WORD_LEN} has a character that isn't one")
        words.append("".join(map(chr, slot[:end])))
        padding.append("".join(f"{c:04x}" for c in slot[end + 1:]))
    return {"words": words, "padding": padding}


def list_bytes(entry: dict, where: str) -> bytes:
    words, padding = entry["words"], entry["padding"]
    if len(padding) > len(words):
        raise DataError(f"{where}: more padding than words")
    plain = []
    for i, word in enumerate(words):
        if len(word) >= WORD_LEN:
            raise DataError(f"{where}: {word!r} is longer than {WORD_LEN - 1} characters")
        rest = padding[i] if i < len(padding) else ""
        filler = [int(rest[j:j + 4], 16) for j in range(0, len(rest), 4)]
        free = WORD_LEN - 1 - len(word)
        plain += [ord(c) for c in word] + [TERMINATOR] + (filler + [0] * free)[:free]
    return struct.pack(f"<{len(plain)}H", *crypt(plain))


def dump(files: Path, output: Path):
    members = read_narc((files / "a/2/1/6").read_bytes())
    if len(members) != LIST_COUNT:
        raise ValueError(f"{len(members)} lists, not {LIST_COUNT}")
    lists = [list_json(data) for data in members]
    write(output, {"$schema": "profane_words.schema.json", "lists": lists})
    print(f"wrote {sum(len(entry['words']) for entry in lists)} words to {output}")


def pack(path: Path, output: Path):
    lists = load(path, load_schema(path.parent / "profane_words.schema.json"))["lists"]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc([list_bytes(entry, f"{label(path)}: list {i}") for i, entry in enumerate(lists)]))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write profane_words.json from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build the profane words' archive")
    pack_parser.add_argument("path", type=Path)
    pack_parser.add_argument("archive", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.path, args.archive)
    except DataError as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()

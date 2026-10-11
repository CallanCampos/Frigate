#!/usr/bin/env python3
"""Convert the game's text archives (a/0/0/2, the system messages, and a/0/0/3, the script messages) to and from
editable text files, one per message file, with one message per line. The build packs the text files back into the
archives and checks that they match.

    text_data.py unpack extract/b2_us/files/a/0/0/2 data/text/system
    text_data.py pack data/text/system build/b2_us/files/a/0/0/2

The files are named after the constants of their archive's list (TEXT_LISTS): a system message file after its
TEXT_BANK_* in data/constants/text_banks.txt, as species_names.txt, and a script message file after its SCRIPT_TEXT_*
in script_text_banks.txt, as black_city.txt; the list gives their order.

The text is UTF-8. In it:

- `\\n` is a line break within a message, and `\\\\`, `\\{` and `\\}` are a backslash and braces.
- `{TTTT}` or `{TTTT:a,b}` is a control code: its type in hex and its arguments, such as the placeholder of a name.
- `\\x{HHHH}` is a character the text can't show as itself. Some messages end with `\\x{ffff}`: a terminator before
  the last one, which the game's files have as padding.
- A message that starts with `\\c` is stored compressed, as the game stores names, among others.
- A last line `\\pad{XX}` is not a message but the byte that fills the end of the file to a multiple of 4 bytes, which
  the game's files have as leftovers rather than 0; it is ignored when no filling is needed.
- A line `\\from{species.name}` is not a message but stands for messages from the game data in JSON, one per species,
  move or trainer, such as the species' names (see text_sources.py). Unpacking writes the messages themselves.

Every message line starts with the message's ID, `BlackCity_Text_DaveUsedLotThugs = Dave: Used to be...`: its bank's
name, then words of the message as unpacking first wrote it, unique in its bank. The build makes a header of each bank's
IDs (text_ids.py), so the code and the scripts name messages rather than count lines, and an ID can be renamed freely.

A message file holds one language. Each message is encrypted with a key that starts at 0x7c89 + 0x2983 times its
index and rotates by 3 bits per character, and a compressed one packs its characters in 9 bits each.
"""
import argparse
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.text.msgdata import COMPRESSED, CONTROL, LINE_END, NEWLINE, decompress, decrypt  # noqa: E402
from tools.data.datajson import DataError  # noqa: E402
from tools.data.gen_constants import ordered_files  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402
from tools.text.text_sources import expand  # noqa: E402

BASE_KEY = 0x7C89
KEY_STEP = 0x2983
ESCAPED = {"\\": "\\\\", "{": "\\{", "}": "\\}"}


def printable(c: int) -> bool:
    if c < 0x20 or 0xD800 <= c < 0xE000 or 0xE000 <= c < 0xF900 or c >= CONTROL:
        return False
    return chr(c).isprintable()


def format_message(chars: list[int], compressed: bool) -> str:
    """Returns a message as a line of text; chars has no terminator."""
    out = ["\\c"] if compressed else []
    i = 0
    while i < len(chars):
        c = chars[i]
        if c == CONTROL and i + 2 < len(chars) and i + 3 + chars[i + 2] <= len(chars):
            kind, count = chars[i + 1], chars[i + 2]
            args = chars[i + 3:i + 3 + count]
            out.append(f"{{{kind:04x}" + (":" + ",".join(map(str, args)) if count else "") + "}")
            i += 3 + count
            continue
        if c == NEWLINE:
            out.append("\\n")
        elif printable(c):
            out.append(ESCAPED.get(chr(c), chr(c)))
        else:
            out.append(f"\\x{{{c:04x}}}")
        i += 1
    return "".join(out)


def parse_message(text: str) -> tuple[list[int], bool]:
    """Returns the characters of a line of text, without terminator, and whether it is compressed."""
    compressed = text.startswith("\\c")
    if compressed:
        text = text[2:]
    chars = []
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == "\\":
            nxt = text[i + 1]
            if nxt == "n":
                chars.append(NEWLINE)
                i += 2
            elif nxt == "x":
                end = text.index("}", i)
                chars.append(int(text[i + 3:end], 16))
                i = end + 1
            elif nxt in "\\{}":
                chars.append(ord(nxt))
                i += 2
            else:
                raise ValueError(f"unknown escape \\{nxt} in {text!r}")
        elif ch == "{":
            end = text.index("}", i)
            kind, _, args = text[i + 1:end].partition(":")
            values = [int(a) for a in args.split(",")] if args else []
            chars += [CONTROL, int(kind, 16), len(values), *values]
            i = end + 1
        else:
            chars.append(ord(ch))
            i += 1
    return chars, compressed


def compress(chars: list[int]) -> list[int]:
    """Packs a message's characters in 9 bits each, from the lowest bit of each word up, ending with 0x1ff. The game's
    encoder fills the last word with 1 bits when the terminator leaves 8 or fewer bits of it, and otherwise with 0
    bits followed by a word of 0xffff; every compressed message in the game follows this."""
    bits = 0
    count = 0
    words = [COMPRESSED]
    for c in chars + [0x1FF]:
        if c > 0x1FF:
            raise ValueError(f"character {c:#x} doesn't fit a compressed message")
        bits |= c << count
        count += 9
        while count >= 16:
            words.append(bits & 0xFFFF)
            bits >>= 16
            count -= 16
    if count:
        if count <= 8:
            words.append((bits | (0xFFFF << count)) & 0xFFFF)
        else:
            words += [bits & 0xFFFF, LINE_END]
    return words


def encrypt(chars: list[int], index: int) -> list[int]:
    key = (BASE_KEY + KEY_STEP * index) & 0xFFFF
    out = []
    for c in chars:
        out.append(c ^ key)
        key = ((key << 3) | (key >> 13)) & 0xFFFF
    return out


def unpack_file(data: bytes) -> str:
    num_sections, num_lines = struct.unpack_from("<HH", data, 0)
    if num_sections != 1:
        raise ValueError(f"{num_sections} languages")
    section = struct.unpack_from("<I", data, 12)[0]
    lines = []
    for i in range(num_lines):
        offset, length, attribute = struct.unpack_from("<IHH", data, section + 4 + 8 * i)
        if attribute:
            raise ValueError(f"message {i} has attribute {attribute}")
        chars = decrypt(struct.unpack_from(f"<{length}H", data, section + offset))
        compressed = bool(chars) and chars[0] == COMPRESSED
        if compressed:
            chars = decompress(chars)
        if not chars or chars[-1] != LINE_END:
            raise ValueError(f"message {i} doesn't end with a terminator")
        lines.append(format_message(chars[:-1], compressed))
    end = 4 + 8 * num_lines + 2 * sum(struct.unpack_from("<IHH", data, section + 4 + 8 * i)[1] for i in range(num_lines))
    size = struct.unpack_from("<I", data, section)[0]
    if size > end and data[section + end] != 0:
        lines.append(f"\\pad{{{data[section + end]:02x}}}")
    return "".join(line + "\n" for line in lines)


# The constant list that names each directory's files, by the directory's name, and the prefix of its constants
TEXT_LISTS = {"system": ("text_banks", "TEXT_BANK_"), "script": ("script_text_banks", "SCRIPT_TEXT_")}


def text_files(directory: Path) -> list[Path]:
    list_name, prefix = TEXT_LISTS[directory.name]
    return ordered_files(list_name, prefix, directory, ".txt")


def message_lines(archive: Path, number: int) -> list[str]:
    """Returns the messages of a file of a text archive as lines of a text file, without the padding line."""
    lines = unpack_file(read_narc(archive.read_bytes())[number]).split("\n")[:-1]
    return [line for line in lines if not line.startswith("\\pad{")]


MESSAGE = re.compile(r"([A-Za-z_]\w*) =(?: (.*))?")
STOP_WORDS = {"a", "an", "the", "and", "or", "of", "to", "is", "it", "i", "you", "your", "my", "me", "be", "in", "on",
              "for", "this", "that", "are", "was", "with", "at", "as", "so", "do"}


def bank_prefix(path: Path) -> str:
    """The prefix of a bank's message IDs: its name in CamelCase, Bank0406 for a numbered one, and Script before a
    script bank's whose name a system bank also has."""
    stem = path.stem
    name = f"Bank{stem}" if stem.isdigit() else "".join(word.capitalize() for word in stem.split("_"))
    if path.parent.name == "script" and (path.parent.parent / "system" / path.name).exists():
        name = "Script" + name
    return name + "_Text_"


def message_ids(prefix: str, messages: list[str]) -> list[str]:
    """IDs for messages, from their first words, numbered from _2 where they repeat."""
    ids, count = [], {}
    for message in messages:
        text = re.sub(r"\\x\{[^}]*\}|\{[^}]*\}|\\[nc]|\\.", " ", message)
        text = text.replace("é", "e").replace("É", "E").replace("’", "'")
        words = [w.replace("'", "") for w in re.findall(r"[A-Za-z0-9][A-Za-z0-9']*", text)]
        words = [w for w in words if w.lower() not in STOP_WORDS] or words
        base = prefix + ("".join(w[:1].upper() + w[1:].lower() for w in words[:4]) or "Empty")
        count[base] = count.get(base, 0) + 1
        ids.append(base if count[base] == 1 else f"{base}_{count[base]}")
    return ids


def with_ids(path: Path, text: str) -> str:
    """Puts an ID before each message line of a file's text, as unpacking writes it."""
    lines = text.split("\n")[:-1]
    messages = [line for line in lines if not line.startswith("\\pad{")]
    ids = iter(message_ids(bank_prefix(path), messages))
    return "".join((line if line.startswith("\\pad{") else f"{next(ids)} = {line}".rstrip()) + "\n"
                   for line in lines)


def split_message(line: str) -> tuple[str | None, str]:
    """Returns a message line's ID and text; a \\from or \\pad line has no ID."""
    if FROM.fullmatch(line) or line.startswith("\\pad{"):
        return None, line
    match = MESSAGE.fullmatch(line)
    if not match:
        raise ValueError(f"a message line starts with its ID, ID = text: {line[:60]!r}")
    return match[1], match[2] or ""


FROM = re.compile(r"\\from\{([\w.]+)\}")


def pack_file(text: str) -> bytes:
    lines = text.split("\n")
    if lines[-1] != "":
        raise ValueError("the file doesn't end with a newline")
    messages = []
    for line in lines[:-1]:
        match = FROM.fullmatch(line)
        if match:
            messages += expand(match[1])
        else:
            messages.append(split_message(line)[1])
    pad = 0
    if messages and messages[-1].startswith("\\pad{"):
        pad = int(messages.pop()[5:-1], 16)
    encoded = []
    for i, message in enumerate(messages):
        chars, compressed = parse_message(message)
        chars = compress(chars) if compressed else chars + [LINE_END]
        encoded.append(encrypt(chars, i))
    table_size = 4 + 8 * len(messages)
    offset = table_size
    table = b""
    body = b""
    for chars in encoded:
        table += struct.pack("<IHH", offset, len(chars), 0)
        body += struct.pack(f"<{len(chars)}H", *chars)
        offset += 2 * len(chars)
    size = (table_size + len(body) + 3) & ~3
    section = struct.pack("<I", size) + table + body
    section += bytes([pad]) * (size - len(section))
    header = struct.pack("<HHIII", 1, len(messages), size, 0, 16)
    return header + section


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    command = commands.add_parser("unpack", help="write an archive's files as text")
    command.add_argument("archive", type=Path)
    command.add_argument("output", type=Path)
    command = commands.add_parser("pack", help="pack a directory of text files into an archive")
    command.add_argument("directory", type=Path)
    command.add_argument("output", type=Path)
    args = parser.parse_args()

    if args.command == "unpack":
        args.output.mkdir(parents=True, exist_ok=True)
        files = read_narc(args.archive.read_bytes())
        paths = text_files(args.output)
        if len(paths) != len(files):
            raise SystemExit(f"{len(files)} files in the archive and {len(paths)} in its list")
        kept = []
        for path, data in zip(paths, files):
            # A file that takes messages from the data in JSON would get them twice
            if path.exists() and any(FROM.fullmatch(line) for line in path.read_text(encoding="utf-8").split("\n")):
                kept.append(path.name)
                continue
            path.write_text(with_ids(path, unpack_file(data)), encoding="utf-8")
        print(f"wrote {len(files) - len(kept)} files to {args.output}")
        if kept:
            print(f"kept {', '.join(kept)}, which take messages from the data in JSON")
    else:
        sources = text_files(args.directory)
        args.output.parent.mkdir(parents=True, exist_ok=True)
        try:
            files = [pack_file(s.read_text(encoding="utf-8")) for s in sources]
        except DataError as error:
            raise SystemExit(f"error: {error}")
        args.output.write_bytes(write_narc(files))


if __name__ == "__main__":
    main()

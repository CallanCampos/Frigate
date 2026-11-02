#!/usr/bin/env python3
"""The debug screens' 2D graphics at the file system's root in res/debug/: dump writes them from the ROM once, and pack
builds each archive from its directory, which the build does.

    debug_graphics_data.py dump extract/b2_us/files res/debug
    debug_graphics_data.py pack res/debug/skb black2 SKB_NARC

- skb/ (skb.narc): a software keyboard's, with hiragana, katakana and letters to type a name in Japanese: its
  characters in four sets, a palette and the screen.
- soundstatus/ (soundstatus.narc): a sound status screen's, which shows the player's tempo, pitch, volume, reverb and
  tracks: its characters, a palette and two screens.

No code of the game loads them by name or number: they are the files of debug tools that the retail game leaves out.
Each directory holds the archive's members as a graphics archive's (graphics_archive.py) are, numbered: a character
resource is an indexed PNG of its palette indices, a palette a JASC palette, and a screen its .nscr file, and they pack
as that script packs them, an unchanged member as the game's bytes. Both versions have the same archives.
"""
import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.graphics_archive import GraphicsError, build_member, member_file, member_files, unpack  # noqa: E402
from tools.data.narc import read_narc, write_narc  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
ARCHIVES = {"skb": "skb.narc", "soundstatus": "soundstatus.narc"}
VERSIONS = {"black2": "b2_us", "white2": "w2_us"}


def dump(files: Path, output: Path):
    for name, path in ARCHIVES.items():
        members = [unpack(blob) for blob in read_narc((files / path).read_bytes())]
        if any(kind for _, kind in members):
            raise ValueError(f"{path} has compressed members, which this script doesn't keep")
        contents = [data for data, _ in members]
        directory = output / name
        directory.mkdir(parents=True, exist_ok=True)
        for index, data in enumerate(contents):
            file_name, content = member_file(f"{index:02d}", data, contents, index)
            (directory / file_name).write_bytes(content)
        print(f"wrote {directory} from {path}")


def pack(directory: Path, game: str, output: Path):
    if directory.name not in ARCHIVES:
        raise GraphicsError(f"{directory}: not one of {', '.join(ARCHIVES)}")
    originals = read_narc((ROOT / "extract" / VERSIONS[game] / "files" / ARCHIVES[directory.name]).read_bytes())
    result = []
    for index, (_, file) in enumerate(member_files(directory, game)):
        original = originals[index] if index < len(originals) else None
        result.append(build_member(file, original))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(write_narc(result))


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    dump_parser = commands.add_parser("dump", help="write res/debug/ from the extracted files")
    dump_parser.add_argument("files", type=Path, help="the extracted files/ directory")
    dump_parser.add_argument("output", type=Path)
    pack_parser = commands.add_parser("pack", help="build one of the archives")
    pack_parser.add_argument("directory", type=Path)
    pack_parser.add_argument("game", choices=VERSIONS)
    pack_parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if args.command == "dump":
        dump(args.files, args.output)
        return
    try:
        pack(args.directory, args.game, args.output)
    except (GraphicsError, ValueError) as error:
        sys.exit(f"error: {error}")


if __name__ == "__main__":
    main()

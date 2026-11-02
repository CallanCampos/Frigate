#!/usr/bin/env python3
"""Disassemble the musical stage's scripts, and write the macros they are assembled with.

    sta_script.py inc include/asm/sta_script.inc     # write the assembler macros
    sta_script.py disasm SCRIPT OUTPUT.s [--program N]  # write a script file as source
    sta_script.py sync [--dry-run]                   # rename the commands after their handlers

Overlay 209 runs them (sta_act_script.c), with the commands of script_command.c's STA_SCRIPT_COMMANDS. A script file
starts with a table of the offsets of its scripts from the file's start, which ends where the first script starts; a
musical program's stage script (member 2 of its archive, tools/data/musical_program_data.py) has 14, of which the game
reads the first, the program's script, and the third to sixth, the scripts of the four positions on the stage, which
StaActing_StartScripts runs beside the first where they differ from it. A command is a 16-bit ID, the index of its
handler in STA_SCRIPT_COMMANDS, followed by 32-bit arguments, which COMMANDS gives from what each handler reads.
There are no jumps: a script runs from its start to its End.

The commands are named after their handlers, as the field scripts' are: StaScriptCmd_PokeMove is PokeMove, and the
macros follow STA_SCRIPT_COMMANDS, so that a handler renamed with rename_symbol.py renames its command with `sync`,
which writes the macros again and renames the command in res/musicals.

The .s files go through the C preprocessor, so that they can use the constants in include/constants, and assemble back
to the original bytes. The scripts in res/musicals are source, edited by hand; disassembling them again is for
reference.
"""
import argparse
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.data.gen_constants import header_text  # noqa: E402

SOURCE = ROOT / "src" / "ov209" / "script_command.c"
INC = ROOT / "include" / "asm" / "sta_script.inc"
SCRIPTS = ROOT / "res" / "musicals"
HANDLER_PREFIX = "StaScriptCmd_"

# The arguments of each command, in ID order, with their kinds, and what the command does. The kinds:
#   value  a 32-bit value
#   fx     an fx32, 0x1000 for 1, written in hex
#   bool   a flag the handler compares with TRUE or tests, TRUE or FALSE
#   poke   the Pokémon at a position, 0 to 3, or STA_SCRIPT_OWN_POKE, those the script was started for
#          (StaScript_GetPokeMask)
#   seq    a sequence, a SEQ_* constant
# A move takes frames, and is done at once for 0; with wait TRUE the script waits for it.
XYZ = [("x", "fx"), ("y", "fx"), ("z", "fx")]
COMMANDS = [
    ([], "Ends the script"),
    ([("frames", "value")], "Waits a number of frames"),
    ([("frame", "value")], "Waits until the script has run a number of frames"),
    ([("unused", "value")], "In a synced script, waits until every synced script waits; the value isn't read"),
    ([("wait", "bool")], "Opens the curtain"),
    ([("wait", "bool")], "Closes the curtain"),
    ([("frames", "value"), ("offset", "value"), ("wait", "bool")], "Moves the curtain to an offset"),
    ([("frames", "value"), ("offset", "value"), ("wait", "bool")], "Scrolls the stage to an offset"),
    ([("unused", "value")], "Does nothing"),
    ([("poke", "poke"), ("show", "bool")], "Shows or hides Pokémon"),
    ([("poke", "poke"), ("flip", "bool")], "Turns Pokémon to face the other way, or back"),
    ([("poke", "poke"), ("frames", "value"), *XYZ, ("wait", "bool")], "Moves Pokémon to a position"),
    ([("poke", "poke"), ("frames", "value"), *XYZ, ("wait", "bool")], "Moves Pokémon by an offset"),
    ([("poke", "poke")], "Stops Pokémon's animation"),
    ([("poke", "poke")], "Starts Pokémon's animation"),
    ([("poke", "poke"), ("anime", "value")], "Changes Pokémon's animation"),
    ([("poke", "poke"), ("period", "value"), ("count", "value"), ("height", "fx"), ("wait", "bool")],
     "Makes Pokémon jump a number of times, each taking period frames"),
    ([("poke0", "bool"), ("poke1", "bool"), ("poke2", "bool"), ("poke3", "bool")],
     "Sets the positions STA_SCRIPT_OWN_POKE means"),
    ([("index", "value"), ("objId", "value")], "Adds a stage object in a slot"),
    ([("index", "value")], "Removes the object of a slot"),
    ([("index", "value")], "Shows the object of a slot"),
    ([("index", "value")], "Hides the object of a slot"),
    ([("index", "value"), ("frames", "value"), *XYZ], "Moves the object of a slot"),
    ([("index", "value"), ("effectNo", "value")], "Adds a particle effect in a slot"),
    ([("index", "value")], "Removes the effect of a slot"),
    ([("index", "value"), ("emitterNo", "value"), *XYZ], "Creates an emitter of a slot's effect at a position"),
    ([("index", "value"), ("emitterNo", "value")], "Deletes an emitter of a slot's effect"),
    ([("index", "value"), ("emitterNo", "value"), ("period", "value"), ("count", "value"), ("minX", "fx"),
      ("minY", "fx"), ("minZ", "fx"), ("maxX", "fx"), ("maxY", "fx"), ("maxZ", "fx")],
     "Creates an emitter at random positions in a box, count times, every period frames"),
    ([("index", "value"), ("unk", "value")], "Adds a spotlight in a slot"),
    ([("index", "value")], "Removes the light of a slot"),
    ([("index", "value"), ("frames", "value"), *XYZ], "Moves the light of a slot"),
    ([("index", "value"), ("poke", "poke"), ("frames", "value"), *XYZ],
     "Makes the light of a slot follow a Pokémon at an offset for frames"),
    ([("index", "value"), ("r", "value"), ("g", "value"), ("b", "value"), ("alpha", "value")],
     "Sets the color of the light of a slot"),
    ([("msgId", "value"), ("wait", "value")], "Prints a message of the program's messages.txt, by its line"),
    ([], "Clears the message"),
    ([("letter", "value"), ("shadow", "value"), ("background", "value")], "Sets the message's colors"),
    ([("seq", "seq")], "Plays a sequence"),
    ([], "Stops the sequence"),
    ([("poke", "poke"), ("frames", "value"), ("start", "value"), ("angle", "value"), ("wait", "bool")],
     "Rotates Pokémon from an angle by an angle"),
    ([("poke", "poke"), ("front", "bool")], "Turns Pokémon to the front, or back"),
    ([("poke", "poke"), ("show", "bool")], "Shows or hides Pokémon's props"),
    ([("poke", "poke")], "Makes Pokémon appeal"),
    ([("poke", "poke")], "Turns the audience to Pokémon"),
    ([], "Turns the audience away from every Pokémon"),
    ([], "Enables the props' buttons"),
    ([], "Disables the props' buttons"),
    ([("frames", "value"), ("wait", "bool")], "Lines the Pokémon up by their points at the end"),
    ([("unused", "value")], "Does nothing"),
    ([("seq", "seq")], "Plays a BGM"),
    ([], "Stops the BGM"),
    ([("pos", "value"), ("file", "value")], "Starts an appeal script for a position, by its points"),
    ([("poke", "poke"), ("file", "value")], "Starts a Pokémon script"),
    ([("seq", "seq"), ("a1", "value"), ("a2", "value")],
     "Replaces a wave of the sequence's bank's first wave archive with the program's wave"),
    ([("seq", "seq")], "Plays the program's own sequence and bank as a sequence's"),
]

# The headers the scripts include, and the prefix of the constants of each kind
CONSTANTS = {"seq": ("constants/sound.h", "SEQ_")}
# The VM's own codes, which the macros define
OWN_POKE = 0xFFFFFFFF  # STA_SCRIPT_OWN_POKE in script_command.c
CODES = {"poke": {OWN_POKE: "STA_SCRIPT_OWN_POKE"}, "bool": {0: "FALSE", 1: "TRUE"}}

# The names of a musical program's scripts, by their entry in the table (StaActing_StartScripts)
PROGRAM_ENTRIES = {0: "Main", 2: "Position0", 3: "Position1", 4: "Position2", 5: "Position3"}
PROGRAM_COMMENTS = {0: "0: the program's script", 1: "1: not read",
                    2: "2 to 5: the four positions' scripts, run beside the first where they differ from it",
                    6: "6 to 13: not read"}


def command_names() -> list[str]:
    """The commands' names, from their handlers in script_command.c's STA_SCRIPT_COMMANDS."""
    table = re.search(r"STA_SCRIPT_COMMANDS\[\] = \{(.*?)\};", SOURCE.read_text(), re.DOTALL)[1]
    handlers = [h.strip() for h in table.split(",") if h.strip()]
    if len(handlers) != len(COMMANDS):
        sys.exit(f"STA_SCRIPT_COMMANDS has {len(handlers)} commands, and COMMANDS {len(COMMANDS)}")
    return [h.removeprefix(HANDLER_PREFIX) for h in handlers]


def load_constants() -> dict[str, dict[int, str]]:
    constants = {kind: dict(codes) for kind, codes in CODES.items()}
    for kind, (header, prefix) in CONSTANTS.items():
        names = {}
        for match in re.finditer(rf"^#define ({prefix}\w*) (0x[0-9a-fA-F]+|\d+)$", header_text(header), re.MULTILINE):
            names.setdefault(int(match[2], 0), match[1])
        constants[kind] = names
    return constants


def write_inc(path: Path):
    headers = sorted({header for header, _ in CONSTANTS.values()})
    lines = [
        "// Macros for the musical stage's scripts, written by tools/script/sta_script.py inc",
        "",
        *(f'#include "{header}"' for header in headers),
        "",
        "    // In .data, which LLVM pads with zeros: in .text, clang 18 pads `.balign 4, 0` with ARM NOPs",
        "    .data",
        "",
        "    .set FALSE, 0",
        "    .set TRUE, 1",
        "",
        "    // The Pokémon the script was started for, or that SetPokeMask set (StaScript_GetPokeMask)",
        "    .set STA_SCRIPT_OWN_POKE, -1",
        "",
        "    .macro StaScriptCommand id",
        "    .2byte \\id",
        "    .endm",
        "",
        "    // The start of a script file, followed by a ScriptEntry for each of its scripts",
        "    .macro ScriptHeader",
        "ScriptStart:",
        "    .endm",
        "",
        "    // A script's offset in the file",
        "    .macro ScriptEntry label",
        "    .4byte \\label - ScriptStart",
        "    .endm",
        "",
    ]
    for cmd_id, (name, (params, doc)) in enumerate(zip(command_names(), COMMANDS)):
        if doc:
            lines.append(f"    // {doc}")
        lines.append(f"    .macro {name}{' ' if params else ''}{', '.join(param for param, _ in params)}")
        lines.append(f"    StaScriptCommand {cmd_id}")
        lines += [f"    .4byte \\{param}" for param, _ in params]
        lines.append("    .endm")
        lines.append("")
    path.write_text("\n".join(lines))


def s32(value: int) -> int:
    return value - (1 << 32) if value & 0x80000000 else value


def format_arg(kind: str, value: int, constants: dict[str, dict[int, str]]) -> str:
    if kind == "fx":
        signed = s32(value)
        return "0" if signed == 0 else f"-{-signed:#x}" if signed < 0 else f"{signed:#x}"
    name = constants.get(kind, {}).get(value)
    if name is not None:
        return name
    signed = s32(value)
    return str(signed) if -0x10000 < signed < 0x10000 else f"{value:#x}"


def disassemble(data: bytes, program: bool = False) -> str:
    """Returns a script file as source; a program's script names its scripts as the game runs them."""
    constants = load_constants()
    names = command_names()
    first = struct.unpack_from("<I", data, 0)[0]
    if first % 4 or not 4 <= first <= len(data):
        raise ValueError(f"the table of scripts can't end at {first:#x}")
    entries = list(struct.unpack_from(f"<{first // 4}I", data, 0))
    labels: dict[int, str] = {}
    for index, offset in enumerate(entries):
        if not first <= offset < len(data):
            raise ValueError(f"script {index} starts at {offset:#x}, outside the file")
        name = PROGRAM_ENTRIES.get(index, f"Script{index}") if program else f"Script{index}"
        labels.setdefault(offset, name)
    out = ["    ScriptHeader"]
    for index, offset in enumerate(entries):
        comment = PROGRAM_COMMENTS.get(index) if program else None
        out.append(f"    ScriptEntry {labels[offset]}" + (f"  // {comment}" if comment else ""))
    pc = first
    while pc < len(data):
        if pc in labels:
            out += ["", f"{labels[pc]}:"]
        cmd_id = struct.unpack_from("<H", data, pc)[0] if pc + 2 <= len(data) else len(COMMANDS)
        params = COMMANDS[cmd_id][0] if cmd_id < len(COMMANDS) else None
        end = pc + 2 + 4 * len(params) if params is not None else 0
        if params is None or end > len(data) or any(o in labels for o in range(pc + 1, end)):
            out.append(f"    .byte {data[pc]:#04x}")
            pc += 1
            continue
        args = struct.unpack_from(f"<{len(params)}I", data, pc + 2)
        text = ", ".join(format_arg(kind, value, constants) for (_, kind), value in zip(params, args))
        out.append(f"    {names[cmd_id]}{' ' if text else ''}{text}")
        pc = end
    return "\n".join(out) + "\n"


def sync(dry_run: bool = False):
    """Writes the macros again from STA_SCRIPT_COMMANDS, and renames the commands whose handlers were renamed in the
    scripts of res/musicals; with dry_run, only lists the renames. sync_handler_names.py runs it too."""
    old = {}
    if INC.exists():
        for match in re.finditer(r"^    \.macro (\w+).*\n    StaScriptCommand (\d+)$", INC.read_text(), re.MULTILINE):
            old[int(match[2])] = match[1]
    names = command_names()
    renames = {old[i]: name for i, name in enumerate(names) if i in old and old[i] != name}
    for old_name, new_name in renames.items():
        print(f"{old_name} -> {new_name}")
    if dry_run:
        return
    write_inc(INC)
    if not renames:
        return
    pattern = re.compile(rf"^(\s*)({'|'.join(map(re.escape, renames))})\b", re.MULTILINE)
    for path in sorted(SCRIPTS.rglob("*.s")):
        text = path.read_text()
        renamed = pattern.sub(lambda m: m[1] + renames[m[2]], text)
        if renamed != text:
            path.write_text(renamed)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    inc = commands.add_parser("inc")
    inc.add_argument("output", type=Path)
    disasm = commands.add_parser("disasm")
    disasm.add_argument("script", type=Path, help="a script file, such as a musical program archive's member 2")
    disasm.add_argument("output", type=Path)
    disasm.add_argument("--program", type=int, help="the musical program it belongs to, to name its scripts")
    sync_parser = commands.add_parser("sync")
    sync_parser.add_argument("--dry-run", action="store_true", help="list the renames and change nothing")
    args = parser.parse_args()

    if args.command == "inc":
        args.output.parent.mkdir(parents=True, exist_ok=True)
        write_inc(args.output)
    elif args.command == "disasm":
        header = f"// Musical program {args.program}'s stage script\n\n" if args.program is not None else ""
        source = disassemble(args.script.read_bytes(), args.program is not None)
        args.output.write_text(f'{header}#include "asm/sta_script.inc"\n\n{source}')
    else:
        sync(args.dry_run)


if __name__ == "__main__":
    main()

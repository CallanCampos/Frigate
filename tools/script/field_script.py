#!/usr/bin/env python3
"""Disassemble the field scripts, archive a/0/5/6, and write the macros they are assembled with.

    field_script.py inc include/asm/field_script.inc
    field_script.py disasm extract/b2_us OUTPUT_DIR

The archive holds a script file and a map script table for each zone, and the global scripts. A script file starts
with the offsets of its scripts, relative to the end of each offset and ended by 0xFD13, followed by the scripts: a
16-bit command ID, then its arguments, as tools/script/field_commands.json gives them (see field_command_table.py).
Commands from 1000 up belong to the script plugin of the zone. Scripts also hold movement data for ActorCmdExec, pairs
of an action and a count ended by action 0xFE.

A map script table lists the scripts a zone runs at points such as its loading: a 16-bit type and a 32-bit script ID
each, ended by 0. Type 1 instead points to a table of conditions: a variable, a value and a script ID each, ended by 0.
"""
import argparse
import collections
import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.gen_constants import header_text  # noqa: E402
from tools.data.gen_constants import ordered_files  # noqa: E402
from tools.text.text_ids import message_numbers  # noqa: E402
from tools.text.msgdata import read_msgdata  # noqa: E402
from tools.data.narc import read_narc  # noqa: E402

TABLE = Path(__file__).with_name("field_commands.json")
ARCHIVE = "files/a/0/5/6"
ZONE_ARCHIVE = "files/a/0/1/2"
MESSAGE_ARCHIVE = "files/a/0/0/3"
# The headers the scripts include, and the constants they give for each meaning of an argument
CONSTANTS = {
    "comparison": ("constants/field_script.h", "CMP_(?!STACK)"),
    "condition": ("constants/field_script.h", "CMP_(?!OR|AND)"),
    "message_file": ("constants/field_script.h", "MSGFILE_"),
    "item": ("constants/items.h", "ITEM_"),
    "move": ("constants/moves.h", "MOVE_"),
    "species": ("constants/species.h", "SPECIES_"),
    "ability": ("constants/abilities.h", "ABILITY_"),
    "type": ("constants/types.h", "TYPE_"),
    "sound": ("constants/sound.h", "SEQ_"),
    "trainer": ("constants/trainers.h", "TRAINER_"),
    "zone": ("constants/zones.h", "ZONE_"),
    "trade": ("constants/trades.h", "TRADE_"),
    "flag": ("constants/flags.h", "EVENT_FLAG_"),
    # Not a meaning: the names of the event work variables, wherever an argument takes a variable
    "var": ("constants/vars.h", "EVENT_WORK_"),
}
ZONE_TEXT = 10  # offset of the text file in a zone header
ZONE_SIZE = 48
ZONE_SCRIPTS = 6  # offsets of the script file and the map script table in a zone header
ZONE_MAP_SCRIPTS = 8
ENTRIES_END = 0xFD13
VARS_START, VARS_END = 0x4000, 0xC000  # IDs of script variables, see constants/field_script.h
MSGFILE_SCRIPT = 0x400
MOVEMENT_END = 0xFE
# Bounds on unreferenced movement data, above what the referenced data uses
MOVEMENT_MAX_ACTION = 0xFF
MOVEMENT_MAX_COUNT = 0xFF
SIZES = {"u8": 1, "u16": 2, "any": 2, "var": 2, "u32": 4}
DIRECTIVES = {1: ".byte", 2: ".2byte", 4: ".4byte"}
# The arguments that are offsets, relative to their end: code, movement data and other data
CODE_REFERENCES = {"VMCall": 0, "VMJump": 0, "VMJumpIf": 1, "VMCallIf": 1}
MOVEMENT_REFERENCES = {"ActorCmdExec": 1}
DATA_REFERENCES = {"ElevatorSetTablePtr": 0}
# Commands after which the script doesn't go on
ENDS = {"VMHalt", "VMReturn", "VMJump", "RTEndGlobal"}


# Names for commands that swan does not name, where the function the handler calls tells what it does
COMMAND_NAMES = {
    0x172: "IsFestMissionAvailable",      # isFesMissionAvailable
    0x216: "IsOneShotDRObtained",         # isOneShotDRObtained
    0x2F1: "SetOneShotDRObtained",        # setOneShotDRObtained
    0x2F2: "IsOneShotDRObtained",         # isOneShotDRObtained
    0x233: "UnityTowerGetVisitorCountry",  # UnityTowerVisitor_GetCountry
    0x2D9: "UnityTowerGetVisitorParam",   # UnityTower_GetVisitorParam
    0x23A: "PokeVoicePlay",               # starts the event of EventPokeVoicePlay_Callback
    0x22D: "FieldEffect",                 # EventFieldEffect_Create
}


class Command:
    def __init__(self, cmd: int, name: str, entry: dict):
        self.id = cmd
        self.name = name
        self.handler = entry["handler"]
        self.kinds = list(entry["args"])
        self.meanings = entry.get("meanings", [None] * len(self.kinds))
        self.references = {}
        for table, kind in ((CODE_REFERENCES, "code"), (MOVEMENT_REFERENCES, "movement"), (DATA_REFERENCES, "data")):
            if name in table:
                self.references[table[name]] = kind
        self.ends = name in ENDS or "end" in entry.get("flow", [])
        # Unloads one overlay and loads another, which changes the plugin's commands
        self.swap = entry.get("swap")

    @property
    def size(self) -> int:
        return 2 + sum(SIZES[k] for k in self.kinds)


def load_commands() -> tuple[dict[int, Command], dict[int, dict[int, Command]], dict[int, list[int]], list[dict]]:
    table = json.loads(TABLE.read_text())
    names = collections.Counter()
    base = {}
    for cmd, entry in sorted(table["commands"].items(), key=lambda x: int(x[0])):
        cmd = int(cmd)
        name = re.sub(r"^s[0-9A-F]{4}_", "", entry["handler"])
        if cmd in COMMAND_NAMES:
            name = COMMAND_NAMES[cmd]
        elif name.startswith("func_"):
            name = f"{entry.get('area', '')}Cmd_{cmd:04X}"
        names[name] += 1
        if names[name] > 1:
            name = f"{name}_{cmd:04X}"
        base[cmd] = Command(cmd, name, entry)
    # Each plugin's commands, by the overlay swapped in ("" for none). A command is named after its handler once the
    # handler has a name; plugins that share a handler share the command
    def plugin_name(entry: dict, fallback: str) -> str:
        return fallback if entry["handler"].startswith("func_") else entry["handler"]

    plugins, zones = {}, {}
    for number, plugin in table["plugins"].items():
        number = int(number)
        plugins[number] = {"": {int(cmd): Command(int(cmd), plugin_name(entry, f"Plugin{number}_Cmd{int(cmd)}"), entry)
                                for cmd, entry in plugin["commands"].items()}}
        for overlay, commands in plugin.get("variants", {}).items():
            plugins[number][overlay] = {
                int(cmd): Command(int(cmd), plugin_name(entry, f"Plugin{number}Ov{overlay}_Cmd{int(cmd)}"), entry)
                for cmd, entry in commands.items()}
        zones[number] = plugin["zones"]
    return base, plugins, zones, table["global_scripts"]


def load_constants() -> dict[str, dict[int, str]]:
    constants = {}
    for meaning, (header, prefix) in CONSTANTS.items():
        names = {}
        for match in re.finditer(rf"^#define ({prefix}\w*) (\w+)", header_text(header), re.MULTILINE):
            names.setdefault(int(match[2], 0), match[1])
        constants[meaning] = names
    return constants


def write_inc(path: Path):
    base, plugins, _, _ = load_commands()
    lines = [
        "// Macros for the field scripts, written by tools/script/field_script.py inc",
        "",
        *(f'#include "{header}"' for header in sorted({h for h, _ in CONSTANTS.values()})),
        "",
        "    // In .data, which LLVM pads with zeros: in .text, clang 18 pads `.balign 4, 0` with ARM NOPs",
        "    .data",
        "",
        "    .macro FieldCommand id",
        "    .2byte \\id",
        "    .endm",
        "",
        "    // The offset of a script, at the start of a script file",
        "    .macro ScriptEntry label",
        "    .4byte \\label - (. + 4)",
        "    .endm",
        "",
        "    .macro ScriptEntriesEnd",
        f"    .2byte {ENTRIES_END:#x}",
        "    .endm",
        "",
        "    // Movement data for ActorCmdExec",
        "    .macro Move action, count",
        "    .2byte \\action",
        "    .2byte \\count",
        "    .endm",
        "",
        "    .macro MoveEnd",
        f"    .2byte {MOVEMENT_END:#x}",
        "    .2byte 0",
        "    .endm",
        "",
        "    // A map script table",
        "    .macro MapScript type, script",
        "    .2byte \\type",
        "    .4byte \\script",
        "    .endm",
        "",
        "    .macro MapScriptConditions label",
        "    .2byte 1",
        "    .4byte \\label - (. + 4)",
        "    .endm",
        "",
        "    .macro MapScriptsEnd",
        "    .2byte 0",
        "    .endm",
        "",
        "    .macro MapScriptCondition var, value, script",
        "    .2byte \\var",
        "    .2byte \\value",
        "    .2byte \\script",
        "    .endm",
        "",
        "    .macro MapScriptConditionsEnd",
        "    .2byte 0",
        "    .endm",
        "",
    ]

    def macro(command: Command):
        params = [f"a{i}" for i in range(len(command.kinds))]
        out = [f"    // {command.handler}" if command.name.startswith(("Cmd_", "Plugin")) else None,
               f"    .macro {command.name}{' ' if params else ''}{', '.join(params)}",
               f"    FieldCommand {command.id}"]
        for i, (param, kind) in enumerate(zip(params, command.kinds)):
            if i in command.references:
                out.append(f"    .4byte \\{param} - (. + 4)")
            else:
                out.append(f"    {DIRECTIVES[SIZES[kind]]} \\{param}")
        out += ["    .endm", ""]
        return [line for line in out if line is not None]

    for command in base.values():
        lines += macro(command)
    defined = {}
    for number, variants in plugins.items():
        for overlay, commands in variants.items():
            lines.append(f"    // Plugin {number}" + (f", with overlay {overlay} loaded" if overlay else ""))
            lines.append("")
            for command in commands.values():
                text = macro(command)
                if command.name in defined:
                    if defined[command.name] != text:
                        sys.exit(f"{command.name} is defined differently in two plugins")
                    continue
                defined[command.name] = text
                lines += text
    path.write_text("\n".join(lines))


def u16(data: bytes, offset: int) -> int:
    return struct.unpack_from("<H", data, offset)[0]


def s32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<i", data, offset)[0]


def script_header(data: bytes) -> tuple[list[int], int, bool] | None:
    """Returns the offsets of the scripts, the end of the header and whether 0xFD13 ends it, or None if this is not a
    script file. Without 0xFD13, the header ends where the first script starts."""
    entries = []
    pos = 0
    while pos + 2 <= len(data):
        if u16(data, pos) == ENTRIES_END:
            return entries, pos + 2, True
        if entries and pos == min(entries):
            return entries, pos, False
        if pos + 4 > len(data):
            return None
        target = pos + 4 + s32(data, pos)
        if not 0 <= target < len(data):
            return None
        entries.append(target)
        pos += 4
    return None


class ScriptFile:
    def __init__(self, data: bytes, base: dict[int, Command], plugin: dict[str, dict[int, Command]] | None,
                 constants: dict | None = None, messages=None, message_ids=None):
        self.data = data
        self.constants = constants or {}
        # A function from (text file or None for the script's own, message ID) to the text, for comments
        self.messages = messages
        # A function from the same to the message's bank and ID name (text_ids.py), and the banks named so far
        self.message_ids = message_ids
        self.banks: set[str] = set()
        self.base = base
        self.plugin = plugin or {"": {}}
        self.instructions: dict[int, tuple[Command, list[int], int]] = {}
        self.movements: dict[int, int] = {}  # start -> end
        self.data_refs: set[int] = set()
        self.labels: dict[int, str] = {}
        self.errors = collections.Counter()

    def command(self, cmd: int, state: str = "") -> Command | None:
        return self.base.get(cmd) if cmd < 1000 else self.plugin.get(state, {}).get(cmd)

    def after(self, command: Command, state: str) -> str:
        """The overlay state after a command, which may swap the plugin's overlays."""
        if command.swap:
            unloaded, loaded = (str(o) for o in command.swap)
            if loaded in self.plugin:
                return loaded
            if unloaded == state:
                return ""
        return state

    def decode(self, pc: int, state: str = ""):
        if pc + 2 > len(self.data):
            return None
        command = self.command(u16(self.data, pc), state)
        if command is None or pc + command.size > len(self.data):
            return None
        args = []
        pos = pc + 2
        for i, kind in enumerate(command.kinds):
            size = SIZES[kind]
            if i in command.references:
                value = pos + 4 + s32(self.data, pos)
                if not 0 <= value < len(self.data):
                    return None
            else:
                value = int.from_bytes(self.data[pos:pos + size], "little")
            args.append(value)
            pos += size
        return command, args, pos

    def trace(self, starts: list[int], limit: tuple[int, int] | None = None, commit: bool = True) -> bool:
        """Follows the code from the starts. With a limit, fails when the code leaves it or overlaps what is known."""
        found = {}
        movements = {}
        data_refs = set()
        pending = [(start, "") for start in starts]
        known = self.covered() if limit else set()
        while pending:
            pc, state = pending.pop()
            while pc not in self.instructions and pc not in found:
                if limit and (not limit[0] <= pc < limit[1] or pc in known):
                    return False
                decoded = self.decode(pc, state)
                if decoded is None:
                    if limit:
                        return False
                    self.errors["undecodable"] += 1
                    break
                command, args, end = decoded
                if limit and (end > limit[1] or any(o in known for o in range(pc, end))):
                    return False
                found[pc] = decoded
                state = self.after(command, state)
                for i, kind in command.references.items():
                    if kind == "code":
                        pending.append((args[i], state))
                    elif kind == "movement":
                        movements[args[i]] = None
                    else:
                        data_refs.add(args[i])
                if command.ends:
                    break
                pc = end
        if commit:
            self.instructions.update(found)
            for start in movements:
                self.read_movement(start)
            self.data_refs |= data_refs
        return True

    def read_movement(self, start: int):
        pos = start
        while pos + 4 <= len(self.data):
            action = u16(self.data, pos)
            pos += 4
            if action == MOVEMENT_END:
                self.movements[start] = pos
                return
        self.errors["movement without end"] += 1

    def covered(self) -> set[int]:
        covered = set()
        for pc, (_, _, end) in self.instructions.items():
            covered.update(range(pc, end))
        for start, end in self.movements.items():
            covered.update(range(start, end))
        return covered

    def fill_gaps(self, header_end: int):
        """Decodes what nothing reaches: code that ends cleanly within the gap, and movement data, which starts at a
        multiple of 4 like all the referenced movement data. Other bytes stay raw."""
        covered = self.covered()
        pos = header_end
        while pos < len(self.data):
            if pos in covered:
                pos += 1
                continue
            end = pos
            while end < len(self.data) and end not in covered:
                end += 1
            start = pos
            while start < end:
                if self.trace([start], (start, end)) or (start % 4 == 0 and self.try_movement(start, end)):
                    covered = self.covered()
                    while start < end and start in covered:
                        start += 1
                else:
                    start += 1
            pos = end

    def try_movement(self, start: int, end: int) -> bool:
        pos = start
        while pos + 4 <= end:
            action, count = struct.unpack_from("<HH", self.data, pos)
            pos += 4
            if action == MOVEMENT_END:
                if count != 0 or pos == start + 4:
                    return False
                self.movements[start] = pos
                return True
            if action > MOVEMENT_MAX_ACTION or count > MOVEMENT_MAX_COUNT:
                return False
        return False

    def disassemble(self, entries: list[int], header_end: int, terminated: bool, entry_names: list[str]) -> str:
        self.trace(entries)
        self.fill_gaps(header_end)
        for offset, name in zip(entries, entry_names):
            self.labels.setdefault(offset, name)
        for pc, (command, args, _) in sorted(self.instructions.items()):
            for i, kind in command.references.items():
                prefix = {"code": "L", "movement": "Movement", "data": "Data"}[kind]
                self.labels.setdefault(args[i], f"{prefix}_{args[i]:04X}")
        covered = self.covered()
        starts = set(self.instructions) | set(self.movements) | {o for o in range(len(self.data)) if o not in covered}
        for start, end in self.movements.items():
            starts.update(range(start, end, 4))
        starts.add(len(self.data))
        placeable = starts

        out = []
        for offset, name in zip(entries, entry_names):
            out.append(f"    ScriptEntry {self.labels[offset]}" if offset in placeable
                       else f"    ScriptEntry . + 4 + {offset - len(out) * 4 - 4}")
        if terminated:
            out.append("    ScriptEntriesEnd")

        def label(offset):
            if offset in self.labels and offset in placeable:
                out.append("")
                out.append(f"{self.labels[offset]}:")

        def target(value, field_end):
            if value in placeable:
                return self.labels[value]
            return f". + 4 + {value - field_end}"

        pos = header_end
        while pos < len(self.data):
            label(pos)
            if pos in self.instructions:
                command, args, end = self.instructions[pos]
                parts = []
                field = pos + 2
                out.extend(self.message_comments(command, args))
                text_file = self.command_text_file(command, args)
                for i, (kind, value) in enumerate(zip(command.kinds, args)):
                    field += SIZES[kind]
                    if i in command.references:
                        parts.append(target(value, field))
                    elif command.meanings[i] == "message" and self.message_ids and not VARS_START <= value < VARS_END:
                        named = self.message_ids(text_file, value)
                        if named:
                            self.banks.add(named[0])
                        parts.append(named[1] if named else self.format_value(kind, "message", value))
                    else:
                        parts.append(self.format_value(kind, command.meanings[i], value))
                text = ", ".join(parts)
                out.append(f"    {command.name}{' ' if text else ''}{text}")
                pos = end
            elif pos in self.movements:
                end = self.movements[pos]
                for entry in range(pos, end, 4):
                    if entry != pos:
                        label(entry)
                    action, count = struct.unpack_from("<HH", self.data, entry)
                    out.append("    MoveEnd" if action == MOVEMENT_END else f"    Move {action}, {count}")
                pos = end
            else:
                # Zeros up to a multiple of 4, before movement data or the end, are alignment
                aligned = (pos + 3) & ~3
                if (pos % 4 and aligned <= len(self.data) and not any(self.data[pos:aligned])
                        and (aligned in self.movements or aligned == len(self.data))
                        and not any(o in self.labels for o in range(pos + 1, aligned))):
                    out.append("    .balign 4, 0")
                    pos = aligned
                    continue
                out.append(f"    .byte {self.data[pos]:#04x}")
                pos += 1
        label(len(self.data))
        return "\n".join(out) + "\n"


    def format_value(self, kind: str, meaning: str | None, value: int) -> str:
        if kind in ("any", "var") and VARS_START <= value < VARS_END:
            return self.constants["var"].get(value, f"{value:#06x}")
        if meaning and value in self.constants.get(meaning, {}):
            return self.constants[meaning][value]
        if kind == "u32" and value >= 0x10000:
            return f"{value:#x}"
        return str(value)

    @staticmethod
    def command_text_file(command: Command, args: list[int]) -> int | None:
        """The text file of a command's messages, when it names one other than the script's own."""
        for meaning, value in zip(command.meanings, args):
            if meaning == "message_file" and value != MSGFILE_SCRIPT:
                return value
        return None

    def message_comments(self, command: Command, args: list[int]) -> list[str]:
        """Returns the text of the messages that a command shows, as comments."""
        if self.messages is None:
            return []
        text_file = self.command_text_file(command, args)
        comments = []
        for meaning, value in zip(command.meanings, args):
            if meaning == "message" and not VARS_START <= value < VARS_END:
                text = self.messages(text_file, value)
                if text is not None:
                    comments.append("    // " + json.dumps(text, ensure_ascii=False))
        return comments


def disassemble_map_scripts(data: bytes) -> str | None:
    """Returns the map script table as source, or None if the file isn't one."""
    out = []
    pos = 0
    tables = {}
    while True:
        if pos + 2 > len(data):
            return None
        kind = u16(data, pos)
        if kind == 0:
            out.append("    MapScriptsEnd")
            pos += 2
            break
        if pos + 6 > len(data):
            return None
        if kind == 1:
            target = pos + 6 + s32(data, pos + 2)
            tables[target] = f"Conditions_{target:04X}"
            out.append(f"    MapScriptConditions {tables[target]}")
        else:
            out.append(f"    MapScript {kind}, {struct.unpack_from('<I', data, pos + 2)[0]}")
        pos += 6
    for target in sorted(tables):
        if target < pos:
            return None
        while pos < target:
            out.append(f"    .byte {data[pos]:#04x}")
            pos += 1
        out.append("")
        out.append(f"{tables[target]}:")
        while True:
            if pos + 2 > len(data):
                return None
            var = u16(data, pos)
            if var == 0:
                out.append("    MapScriptConditionsEnd")
                pos += 2
                break
            if pos + 6 > len(data):
                return None
            value, script = struct.unpack_from("<HH", data, pos + 2)
            out.append(f"    MapScriptCondition {var:#06x}, {value}, {script}")
            pos += 6
    out.extend(f"    .byte {b:#04x}" for b in data[pos:])
    return "\n".join(out) + "\n"


def map_script_ids(data: bytes) -> list[int]:
    """Returns the scripts that a map script table starts."""
    ids = []
    pos = 0
    while pos + 6 <= len(data) and (kind := u16(data, pos)) != 0:
        if kind == 1:
            table = pos + 6 + s32(data, pos + 2)
            while 0 <= table and table + 6 <= len(data) and u16(data, table) != 0:
                ids.append(u16(data, table + 4))
                table += 6
        else:
            ids.append(struct.unpack_from("<I", data, pos + 2)[0])
        pos += 6
    return ids


def global_file(global_scripts: list[dict], script_id: int) -> int | None:
    for entry in global_scripts:
        if entry["first"] <= script_id <= entry["last"]:
            return entry["file"]
    return None


def plugin_of_files(files: list[bytes], zones: bytes, base, plugins, plugin_zones, global_scripts) -> dict[int, tuple]:
    """Works out the plugin of each script file. The zone sets the plugin, so a file gets the plugin of the zones that
    use it, or else of the zones that start its scripts, from their map scripts or with RTCallGlobal. A file that no
    zone with a plugin uses gets the only plugin that it decodes with, if there is one."""
    plugin_of_zone = {z: number for number, zone_list in plugin_zones.items() for z in zone_list}
    used = collections.defaultdict(set)
    started = collections.defaultdict(set)
    for z in range(len(zones) // ZONE_SIZE):
        number = plugin_of_zone.get(z)
        script_file = u16(zones, z * ZONE_SIZE + ZONE_SCRIPTS)
        used[script_file].add(number)
        started_ids = map_script_ids(files[u16(zones, z * ZONE_SIZE + ZONE_MAP_SCRIPTS)])
        if script_header(files[script_file]) is not None:
            script = ScriptFile(files[script_file], base, plugins.get(number))
            script.trace(script_header(files[script_file])[0])
            started_ids += [args[0] for command, args, _ in script.instructions.values()
                            if command.name in ("RTCallGlobal", "RTCallGlobalAsync")]
        for script_id in started_ids:
            target = global_file(global_scripts, script_id)
            if target is not None:
                started[target].add(number)
    chosen = {}
    for index, data in enumerate(files):
        if script_header(data) is None:
            continue
        for source, how in ((used, "the zones that use this file"), (started, "the zones that start its scripts")):
            numbers = source.get(index, set()) - {None}
            if len(numbers) == 1:
                chosen[index] = (next(iter(numbers)), how)
                break
        if index in chosen or not any(u16(data, pc) >= 1000 for pc in trace_commands(data, base, None)):
            continue
        fits = []
        for number, commands in plugins.items():
            script = ScriptFile(data, base, commands)
            script.trace(script_header(data)[0])
            if not script.errors:
                fits.append(number)
        if len(fits) == 1:
            chosen[index] = (fits[0], "the only plugin whose commands it decodes with")
    return chosen


def trace_commands(data: bytes, base, plugin) -> list[int]:
    """Returns the offsets where tracing stopped at a command it doesn't know."""
    script = ScriptFile(data, base, plugin)
    entries = script_header(data)[0]
    script.trace(entries)
    stops = []
    for pc, (command, args, end) in script.instructions.items():
        if not command.ends and end not in script.instructions and end + 2 <= len(data):
            stops.append(end)
    return stops


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    inc = commands.add_parser("inc")
    inc.add_argument("output", type=Path)
    disasm = commands.add_parser("disasm")
    disasm.add_argument("extract", type=Path, help="an extracted ROM, such as extract/b2_us")
    disasm.add_argument("output", type=Path)
    args = parser.parse_args()

    if args.command == "inc":
        args.output.parent.mkdir(parents=True, exist_ok=True)
        write_inc(args.output)
        return

    base, plugins, plugin_zones, global_scripts = load_commands()
    files = read_narc((args.extract / ARCHIVE).read_bytes())
    # The files are named after their constants in data/constants/field_scripts.txt, in its order
    paths = ordered_files("field_scripts", "SCRIPTS_", args.output, ".s")
    if len(paths) != len(files):
        sys.exit(f"{len(files)} files in the archive and {len(paths)} in field_scripts.txt")
    zones = read_narc((args.extract / ZONE_ARCHIVE).read_bytes())[0]
    plugin_of = plugin_of_files(files, zones, base, plugins, plugin_zones, global_scripts)
    constants = load_constants()
    message_files = read_narc((args.extract / MESSAGE_ARCHIVE).read_bytes())
    text_of = {entry["file"]: entry["text_file"] for entry in global_scripts}
    for z in range(len(zones) // ZONE_SIZE):
        text_of.setdefault(u16(zones, z * ZONE_SIZE + ZONE_SCRIPTS), u16(zones, z * ZONE_SIZE + ZONE_TEXT))
    text_cache = {}

    def messages_for(index):
        def lookup(text_file, message):
            text_file = text_of.get(index) if text_file is None else text_file
            if text_file is None or text_file >= len(message_files):
                return None
            if text_file not in text_cache:
                text_cache[text_file] = read_msgdata(message_files[text_file])
            lines = text_cache[text_file]
            return lines[message] if message < len(lines) else None
        return lookup

    # The IDs of the script message files' messages, from data/text/script/ (text_ids.py), by file and message
    bank_paths = ordered_files("script_text_banks", "SCRIPT_TEXT_", Path("data/text/script"), ".txt")
    id_cache: dict[int, dict[int, str]] = {}

    def ids_for(index):
        def lookup(text_file, message):
            text_file = text_of.get(index) if text_file is None else text_file
            if text_file is None or text_file >= len(bank_paths):
                return None
            if text_file not in id_cache:
                id_cache[text_file] = {n: name for name, n in message_numbers(bank_paths[text_file])}
            name = id_cache[text_file].get(message)
            return (bank_paths[text_file].stem, name) if name else None
        return lookup
    args.output.mkdir(parents=True, exist_ok=True)
    stats = collections.Counter()
    for index, data in enumerate(files):
        header = script_header(data)
        lines = ['#include "asm/field_script.inc"', ""]
        if header is not None:
            entries, header_end, terminated = header
            number, how = plugin_of.get(index, (None, None))
            if number is not None:
                lines.append(f"// Script plugin {number}, from {how}")
                lines.append("")
            script = ScriptFile(data, base, plugins.get(number), constants, messages_for(index), ids_for(index))
            names = [f"Script_{i + 1}" for i in range(len(entries))]
            lines.append(script.disassemble(entries, header_end, terminated, names).rstrip("\n"))
            # The headers of the message IDs the scripts name
            lines[1:1] = [f'#include "text/script/{bank}.h"' for bank in sorted(script.banks)]
            stats["script"] += 1
            stats["raw bytes"] += sum(1 for line in lines[-1].split("\n") if line.startswith("    .byte"))
        else:
            text = disassemble_map_scripts(data)
            if text is None:
                text = "\n".join(f"    .byte {b:#04x}" for b in data) + "\n"
                stats["raw"] += 1
            else:
                stats["map scripts"] += 1
            lines.append(text.rstrip("\n"))
        (args.output / paths[index].name).write_text("\n".join(lines) + "\n")
    print(dict(stats))


if __name__ == "__main__":
    main()

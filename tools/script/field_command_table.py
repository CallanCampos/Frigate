#!/usr/bin/env python3
"""Work out the arguments of every field script command from its handler, and write them to a JSON table.

    field_command_table.py tools/script/field_commands.json

It needs dsd's disassembly in build/asm (see docs/decompiling.md). The field script VM runs the commands in EVCMD_TABLE
(overlay 12), and from ID 1000 the commands of the script plugin that the zone loads (SCRIPT_PLUGIN_TABLE). A handler
reads its arguments from the script through VM_Read16, VM_Read32, ScriptReadAny (a value or a variable), ScriptReadVar
(a variable) and loads through the VM's pc, directly or in the functions it calls with the VM. Reads that only happen
on some paths are marked, as are the commands that jump, call or end the script.
"""
import argparse
import json
import re
import struct
import sys
from functools import lru_cache
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[2]
EXTRACT = ROOT / "extract" / "b2_us"
CONFIG = ROOT / "config" / "b2_us" / "arm9"
ASM = ROOT / "build" / "asm"

# The overlays that are loaded in the field, at their own addresses, and hold the command handlers
FIELD_OVERLAYS = [12, 27, 33, 36, 104]
EVCMD_TABLE = "EVCMD_TABLE"
EVCMD_MAX = "EVCMD_MAX"
SCRIPT_PLUGIN_TABLE = "SCRIPT_PLUGIN_TABLE"
# Script IDs from 2000 up are global scripts, in the files this table gives
GLOBAL_SCRIPT_TABLE = "GLOBAL_SCRIPT_TABLE"
GLOBAL_SCRIPT_COUNT = 60
PLUGIN_COUNT = 17
PLUGIN_FIRST_ID = 1000

PRIMITIVES = {"VM_Read16": "u16", "VM_Read32": "u32", "ScriptReadAny": "any", "ScriptReadVar": "var"}
FLOW = {"VM_Jump": "jump", "VM_Call": "call", "VM_SetNativeCallback": "wait", "VM_Halt": "end", "VM_Return": "return"}
CONDITIONAL_BRANCH = re.compile(r"^\s+b(eq|ne|hi|ls|lo|hs|cs|cc|ge|lt|gt|le|mi|pl)\s")
LABEL = re.compile(r"^(\.L_\w+):")


class Memory:
    def __init__(self):
        overlays = yaml.safe_load((EXTRACT / "arm9_overlays" / "overlays.yaml").read_text())["overlays"]
        self.overlays = {o["id"]: (o["base_address"], (EXTRACT / "arm9_overlays" / o["file_name"]).read_bytes())
                         for o in overlays}
        self.symbol_cache = {}

    def read(self, overlay: int, address: int, size: int) -> bytes:
        base, data = self.overlays[overlay]
        assert base <= address and address + size <= base + len(data), (overlay, hex(address))
        return data[address - base:address - base + size]

    def u32(self, overlay: int, address: int) -> int:
        return struct.unpack("<I", self.read(overlay, address, 4))[0]

    def contains(self, overlay: int, address: int) -> bool:
        base, data = self.overlays[overlay]
        return base <= address < base + len(data)

    def symbols(self, overlay: int) -> dict[int, tuple[str, str]]:
        if overlay not in self.symbol_cache:
            table = {}
            path = CONFIG / "overlays" / f"ov{overlay:03d}" / "symbols.txt"
            for line in path.read_text().splitlines():
                m = re.match(r"(\S+) kind:(\w+)\S* addr:0x([0-9a-f]+)", line)
                if m:
                    table[int(m[3], 16)] = (m[1], m[2])
            self.symbol_cache[overlay] = table
        return self.symbol_cache[overlay]

    def find(self, overlay: int, name: str) -> int:
        for address, (symbol, _) in self.symbols(overlay).items():
            if symbol == name:
                return address
        raise KeyError(name)


def load_functions() -> dict[str, list[str]]:
    functions = {}
    for path in ASM.rglob("*.s"):
        text = path.read_text()
        for m in re.finditer(r"\n    (?:thumb|arm)_func_start (\S+)\n(.*?)\n    (?:thumb|arm)_func_end \1\n", text, re.S):
            functions[m[1]] = m[2].split("\n")
    return functions


FUNCTIONS: dict[str, list[str]] = {}


@lru_cache(None)
def analyse(name: str, depth: int = 0) -> tuple[tuple[tuple[str, str | None], ...], bool, tuple[str, ...]]:
    """Returns the reads of the VM that the function gets in r0, in instruction order, as (kind, meaning), whether any
    read may be skipped or repeated, and the calls that control the VM. The VM pointer is followed through registers
    and stack slots, and each value read through the functions it is passed to."""
    lines = FUNCTIONS.get(name)
    if lines is None or depth > 6:
        return (), False, ()
    labels = {m[1]: k for k, line in enumerate(lines) if (m := LABEL.match(line))}
    pool = {m[1]: m[2] for line in lines if (m := re.match(r"^(\.L_\w+): \.word (\w+)", line))}
    loaded = {}
    reads, flow, read_lines = [], [], []
    vm, slots = {"r0"}, set()
    branched = conditional = False
    for k, line in enumerate(lines):
        if LABEL.match(line):
            continue
        if CONDITIONAL_BRANCH.match(line) or "add pc" in line:
            branched = True
        m = re.match(r"\s+b\w* (\.L_\w+)", line)
        if m and m[1] in labels and labels[m[1]] < k and any(labels[m[1]] < r for r in read_lines):
            conditional = True  # a loop around a read
        m = re.match(r"\s+ldr (r\d+), (\.L_\w+)$", line)
        if m and m[2] in pool:
            loaded[m[1]] = pool[m[2]]
        callee = None
        if m := re.search(r"\sblx? (\w+)", line):
            callee = m[1]
        elif (m := re.match(r"\s+bx (r\d+)$", line)) and m[1] in loaded:
            callee = loaded[m[1]]  # a tail call through the literal pool
        elif (m := re.match(r"\s+b ([A-Za-z_]\w*)$", line)) and m[1] in FUNCTIONS:
            callee = m[1]  # a tail call
        if callee:
            if "r0" in vm:
                if callee in PRIMITIVES:
                    conditional |= branched
                    meaning = None if callee == "ScriptReadVar" else meaning_of(lines, {"r0"}, k + 1)
                    reads.append((PRIMITIVES[callee], meaning))
                    read_lines.append(k)
                elif callee in FLOW:
                    flow.append(FLOW[callee])
                elif callee in FUNCTIONS:
                    sub_reads, sub_conditional, sub_flow = analyse(callee, depth + 1)
                    if sub_reads:
                        conditional |= branched or sub_conditional
                        reads.extend(sub_reads)
                        read_lines.append(k)
                    flow.extend(sub_flow)
            vm -= {"r0", "r1", "r2", "r3", "r12", "lr"}
            continue
        if m := re.match(r"\s+str (r\d+), \[sp, #(0x[0-9a-f]+)\]", line):
            (slots.add if m[1] in vm else slots.discard)(m[2])
            continue
        if m := re.match(r"\s+ldr (r\d+), \[sp, #(0x[0-9a-f]+)\]", line):
            (vm.add if m[2] in slots else vm.discard)(m[1])
            continue
        if m := re.match(r"\s+(?:mov|movs|adds) (r\d+), (r\d+)$", line):
            (vm.add if m[2] in vm else vm.discard)(m[1])
            continue
        if (m := re.match(r"\s+ldr (r\d+), \[(r\d+), #0x20\]", line)) and m[2] in vm:
            # Loads through the VM's pc, until the register is replaced
            base = m[1]
            inline = {}
            for following in lines[k + 1:k + 16]:
                if LABEL.match(following) or re.search(r"\sbl|\sb\w* ", following):
                    break
                if n := re.match(rf"\s+ldr(b|h) (r\d+), \[{base}, #0x([0-9a-f]+)\]", following):
                    at = lines.index(following, k + 1)
                    inline[int(n[3], 16)] = ("u8" if n[1] == "b" else "u16", meaning_of(lines, {n[2]}, at + 1))
                if re.match(rf"\s+(?!str|cmp|tst)\w+ {base}, ", following):
                    break
            for offset in sorted(inline):
                conditional |= branched
                reads.append(inline[offset])
                read_lines.append(k)
        if (m := re.match(r"\s+\w+ (r\d+), ", line)) and not line.strip().startswith(("str", "cmp", "tst", "cmn")):
            vm.discard(m[1])
    return tuple(reads), conditional, tuple(flow)


# What an argument is, from the function it ends up in: (function, parameter) -> meaning
SINKS = {
    ("EventWork_FlagGet", 1): "flag", ("EventWork_FlagSet", 1): "flag", ("EventWork_FlagReset", 1): "flag",
    ("BagSave_AddItem", 1): "item", ("BagSave_SubItem", 1): "item", ("BagSave_CheckAmount", 1): "item",
    ("BagSave_CheckAvailItemSpace", 1): "item", ("BagSave_GetItemCountByID", 1): "item",
    ("BagSave_GetActualItemPocket", 1): "item", ("GetItemParam", 0): "item", ("loadItemNameToStrbuf", 2): "item",
    ("loadItemsNameToStrbuf", 2): "item", ("loadItemTextNameToStrbuf", 2): "item", ("PML_ItemGetTMWazaID", 0): "item",
    ("isMoveMachine", 0): "item",
    ("loadMoveNameToStrbuf", 2): "move",
    ("PokeDex_IsCaught", 1): "species", ("PokeDex_IsSeen", 1): "species",
    ("loadAbilityNameToStrbuf", 2): "ability",
    ("loadTypeTextToStrbuf", 2): "type",
    ("LoadFieldScriptMessage", 1): "message_file", ("LoadFieldScriptMessage", 2): "message",
    ("ScriptWork_AddVM", 2): "script", ("FieldStatus_ReserveScript", 1): "script", ("SetActorSCRID", 1): "script",
    ("GFL_SndSEPlay", 0): "sound", ("EventBGMPlay_Create", 1): "sound", ("EventBGMPlayPush_Create", 1): "sound",
    ("EventMEPlay_Create", 1): "sound", ("GFL_SndBGMIsPlaying", 0): "sound",
    ("TrainerData_GetParam", 0): "trainer", ("TrainerFlagGet", 1): "trainer", ("setTrainerBattleFlag", 1): "trainer",
    ("clearTrainerBattleFlag", 1): "trainer", ("TrainerMsg_Load", 1): "trainer",
    ("EventMapChangeWarp_CreateGrid", 2): "zone", ("EventMapChangeWarp_CreateRail", 2): "zone",
    ("EventMapChange_CreateRail", 2): "zone", ("EventMapChange_CreateGrid", 3): "zone",
    ("EventMapChange_CreateGridDefault", 2): "zone", ("EventMapChangeQuicksand_Create", 3): "zone",
    ("EventMapChangeDiveIn_Create", 1): "zone", ("EventMapChangeWarpPad_Create", 2): "zone",
    ("EventEntralinkWarpIn_Create", 1): "zone", ("EventMapChangeFakeWarp_Create", 2): "zone",
    ("EventMapChangeEnding_Create", 2): "zone",
    ("FieldTradeInput_Create", 1): "trade",
}


# The save data that a handler's calls get, which tells the area of an unnamed command
AREAS = {
    "getTrainerCardDataBlkAddress": "TrainerCard", "getTrainerCardInfoBlkAddress": "TrainerCard",
    "getTrainerCardData_wrapper": "TrainerCard", "getTrainerGameInfoAddress": "TrainerGameInfo",
    "getMusicalInfoBlkAddress": "Musical", "getAddressOfMusicalDataInfo": "Musical",
    "GetTrialHouseWkPPtr": "TrialHouse", "getUnityTower_SurveySaveBlkAddrress": "UnityTower",
    "getHighLinkBlockAddress": "HighLink", "getDreamWorldStuffAddress": "DreamWorld",
    "mysteryGiftBlock": "MysteryGift", "getKeyInfoSaveBlk": "Keys", "getKeyDataBlkAddress": "Keys",
    "getHollow_RivalBlk": "HollowRival", "getTrainerDataBlkAddress": "TrainerData",
    "getTimeSigBlkAddress": "TimeSig", "getAreaNPCData": "AreaNPC",
}


def area_of(name: str) -> str | None:
    """Returns the area of a handler, when all the save data it gets belongs to one."""
    areas = {AREAS[m[1]] for line in FUNCTIONS.get(name, []) if (m := re.search(r"\sblx? (\w+)", line)) and m[1] in AREAS}
    return areas.pop() if len(areas) == 1 else None


# Meanings that the dataflow does not find, by handler
MEANING_OVERRIDES = {
    # The first argument is read and not used, and the message is the second or the third, by the player's gender or
    # the version
    "s0048_ActorMsgGendered": [None, "message", "message", None, None, None],
    "s0049_ActorMsgVersioned": [None, "message", "message", None, None, None],
    "s0011_VMStackCmp": ["comparison"],
    "s001F_VMJumpIf": ["condition", None],
    "s0020_VMCallIf": ["condition", None],
}


@lru_cache(None)
def parameter_meanings(name: str, parameter: int, depth: int = 0) -> frozenset[str]:
    """Returns the meanings of a function's parameter, from the functions it is passed on to."""
    if (name, parameter) in SINKS:
        return frozenset([SINKS[(name, parameter)]])
    lines = FUNCTIONS.get(name)
    if lines is None or depth > 4 or parameter > 3:
        return frozenset()
    meanings = set()
    for callee, index in value_uses(lines, {f"r{parameter}"}):
        meanings |= parameter_meanings(callee, index, depth + 1)
    return frozenset(meanings)


def value_uses(lines: list[str], holders: set[str], after: int = 0) -> list[tuple[str, int]]:
    """Follows a value from the registers that hold it at a line, and returns the (function, parameter) pairs that it
    is passed to. Stack arguments are those stored since the previous call."""
    holders = set(holders)
    slots, fresh, uses = set(), set(), []
    for line in lines[after:]:
        if LABEL.match(line):
            continue
        if m := re.search(r"\sblx? (\w+)", line):
            uses += [(m[1], int(r[1])) for r in ("r0", "r1", "r2", "r3") if r in holders]
            uses += [(m[1], 4 + int(o, 16) // 4) for o in fresh & slots if int(o, 16) < 0x10]
            fresh = set()
            holders -= {"r0", "r1", "r2", "r3", "r12", "lr"}
            if not holders and not slots:
                break
            continue
        if m := re.match(r"\s+strh? (r\d+), \[sp, #(0x[0-9a-f]+)\]", line):
            fresh.add(m[2])
            (slots.add if m[1] in holders else slots.discard)(m[2])
            continue
        if m := re.match(r"\s+ldrh? (r\d+), \[sp, #(0x[0-9a-f]+)\]", line):
            (holders.add if m[2] in slots else holders.discard)(m[1])
            continue
        if m := re.match(r"\s+(?:mov|movs|adds|lsl|lsr|asr) (r\d+), (r\d+)(?:, #0x[0-9a-f]+)?$", line):
            (holders.add if m[2] in holders else holders.discard)(m[1])
            continue
        if (m := re.match(r"\s+\w+ (r\d+), ", line)) and not line.strip().startswith(("str", "cmp", "tst", "cmn")):
            holders.discard(m[1])
    return uses


def meaning_of(lines: list[str], holders: set[str], after: int) -> str | None:
    """Returns what a value is, if all the functions it is passed to agree."""
    found = set()
    for callee, index in value_uses(lines, holders, after):
        found |= parameter_meanings(callee, index)
    return found.pop() if len(found) == 1 else None


def overlay_swap(name: str) -> list[int] | None:
    """Returns [unloaded, loaded] for a handler that unloads one overlay and loads another in its place, such as the
    Join Avenue shop commands, which change the handlers of the plugin's commands."""
    lines = FUNCTIONS.get(name, [])
    pool = {m[1]: m[2] for line in lines if (m := re.match(r"^(\.L_\w+): \.word (\w+)", line))}
    registers, calls = {}, []
    for line in lines:
        if m := re.match(r"\s+ldr (r\d+), (\.L_\w+)$", line):
            registers[m[1]] = pool.get(m[2])
        if m := re.search(r"bl (GFL_OvlUnload|GFL_OvlLoad)\b", line):
            calls.append((m[1], registers.get("r0")))
    if [c for c, _ in calls] == ["GFL_OvlUnload", "GFL_OvlLoad"] and all(v and v.startswith("0x") for _, v in calls):
        return [int(calls[0][1], 16), int(calls[1][1], 16)]
    return None


def handler_entry(memory: Memory, address: int, overlays: list[int]) -> dict | None:
    if address == 0:
        return None
    for overlay in overlays:
        if memory.contains(overlay, address & ~1):
            symbol = memory.symbols(overlay).get(address & ~1)
            if symbol is None:
                return None
            reads, conditional, flow = analyse(symbol[0])
            entry = {"handler": symbol[0], "overlay": overlay, "args": [kind for kind, _ in reads]}
            if symbol[0].startswith("func_") and (area := area_of(symbol[0])):
                entry["area"] = area
            meanings = MEANING_OVERRIDES.get(symbol[0], [meaning for _, meaning in reads])
            if any(meanings):
                entry["meanings"] = meanings
            if conditional:
                entry["conditional"] = True
            if flow:
                entry["flow"] = sorted(set(flow))
            if swap := overlay_swap(symbol[0]):
                entry["swap"] = swap
            return entry
    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    if not ASM.exists():
        sys.exit("build/asm not found, run dsd dis first (see docs/decompiling.md)")
    FUNCTIONS.update(load_functions())
    memory = Memory()

    table = memory.find(12, EVCMD_TABLE)
    count = memory.u32(12, memory.find(12, EVCMD_MAX))
    commands = {}
    for cmd in range(count):
        entry = handler_entry(memory, memory.u32(12, table + 4 * cmd), FIELD_OVERLAYS)
        if entry:
            commands[cmd] = entry

    plugins = {}
    plugin_table = memory.find(12, SCRIPT_PLUGIN_TABLE)
    for number in range(1, PLUGIN_COUNT):
        table, zones, zone_count, overlay1, overlay2 = struct.unpack(
            "<5I", memory.read(12, plugin_table + 0x14 * number, 0x14))
        # The table is in the first overlay, and entries past it point into the second
        overlays = [overlay1] + ([overlay2] if overlay2 != 0xFFFFFFFF else [])
        def plugin_commands(overlays):
            result = {}
            address = table
            while (value := memory.u32(overlay1, address)) != 0xFFFFFFFF:
                entry = handler_entry(memory, value, overlays)
                if entry:
                    result[PLUGIN_FIRST_ID + (address - table) // 4] = entry
                address += 4
            return result

        plugins[number] = {
            "overlays": overlays,
            "zones": [struct.unpack("<H", memory.read(12, zones + 2 * z, 2))[0] for z in range(zone_count)],
            "commands": plugin_commands(overlays),
        }
        # The same commands with an overlay swapped in by a base command
        for entry in commands.values():
            if "swap" in entry and entry["swap"][0] in overlays:
                swapped = [entry["swap"][1] if o == entry["swap"][0] else o for o in overlays]
                plugins[number].setdefault("variants", {})[entry["swap"][1]] = plugin_commands(swapped)

    global_scripts = []
    table = memory.find(12, GLOBAL_SCRIPT_TABLE)
    for i in range(GLOBAL_SCRIPT_COUNT):
        first, last, script_file, text_archive, text_file = struct.unpack("<5H", memory.read(12, table + 10 * i, 10))
        global_scripts.append({"first": first, "last": last, "file": script_file, "text_archive": text_archive,
                               "text_file": text_file})

    table = {"commands": commands, "plugins": plugins, "global_scripts": global_scripts}
    args.output.write_text(json.dumps(table, indent=1) + "\n")


if __name__ == "__main__":
    main()

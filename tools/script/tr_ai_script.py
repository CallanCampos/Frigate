#!/usr/bin/env python3
"""Disassemble and describe the trainer AI scripts, archive 169 (files/a/1/6/9).

Each AI flag has a script file, which runs once per usable move with the move's score. A command is a 16-bit ID
followed by 32-bit arguments; the argument layout of every command comes from its handler in src/ov170/tr_ai.c.

    tr_ai_script.py inc include/asm/tr_ai.inc      # write the assembler macros
    tr_ai_script.py disasm ARCHIVE OUTPUT_DIR      # write one .s file per script

The .s files go through the C preprocessor, so that they can use the constants in include/constants, and assemble
back to the original bytes. The scripts in data/tr_ai are edited by hand since, so disassembling is for reference.
"""
import argparse
import json
import re
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.data.gen_constants import header_text  # noqa: E402
from tools.data.gen_constants import ordered_files  # noqa: E402
from tools.data.narc import read_narc  # noqa: E402

# Commands, in ID order: the macro name, the parameters with their kinds, and what the command does. The names follow
# pokeplatinum's for the commands that Gen 4 has too, and its style for the others. Parameter kinds:
#   value   a 32-bit value
#   result  a value compared with the result, written as the kind of value that the result holds
#   jump    a jump target, relative to the end of this argument, which ends the command
#   list    a list of values ending with 0xffffffff (TABLE_END), relative to the end of this argument
#   table   a jump table indexed by the move's effect, relative to the end of this argument
# and the kinds in CONSTANTS, which are values written as constants.
#
# The result is the value that the Load commands and a few others set (see RESULTS), and that IfLoadedEqualTo and
# the other comparisons of the loaded value read.
COMMANDS = [
    ("IfRandomLessThan", [("value", "value"), ("jump", "jump")], "Jumps if a random number below 256 is less than value"),
    ("IfRandomGreaterThan", [("value", "value"), ("jump", "jump")], ""),
    ("IfRandomEqualTo", [("value", "value"), ("jump", "jump")], ""),
    ("IfRandomNotEqualTo", [("value", "value"), ("jump", "jump")], ""),
    ("AddToMoveScore", [("value", "value")], "Adds to the move's score, which stays at least 0"),
    ("IfHPPercentLessThan", [("battler", "battler"), ("percent", "value"), ("jump", "jump")], ""),
    ("IfHPPercentGreaterThan", [("battler", "battler"), ("percent", "value"), ("jump", "jump")], ""),
    ("IfHPPercentEqualTo", [("battler", "battler"), ("percent", "value"), ("jump", "jump")], ""),
    ("IfHPPercentNotEqualTo", [("battler", "battler"), ("percent", "value"), ("jump", "jump")], ""),
    ("IfStatus", [("battler", "battler"), ("jump", "jump")], "Jumps if the battler has any major status condition. Gen 4's "
     "takes a mask of conditions"),
    ("IfNotStatus", [("battler", "battler"), ("jump", "jump")], ""),
    ("IfCondition", [("battler", "battler"), ("condition", "condition"), ("jump", "jump")], "CONDITION_*"),
    ("IfNotCondition", [("battler", "battler"), ("condition", "condition"), ("jump", "jump")], ""),
    ("IfBadlyPoisoned", [("battler", "battler"), ("jump", "jump")], ""),
    ("IfNotBadlyPoisoned", [("battler", "battler"), ("jump", "jump")], ""),
    ("IfConditionFlag", [("battler", "battler"), ("flag", "condition_flag"), ("jump", "jump")], ""),
    ("IfNotConditionFlag", [("battler", "battler"), ("flag", "condition_flag"), ("jump", "jump")], ""),
    ("IfSideCondition", [("battler", "battler"), ("condition", "side_condition"), ("jump", "jump")],
     "Jumps if the condition is active on the battler's side"),
    ("IfNotSideCondition", [("battler", "battler"), ("condition", "side_condition"), ("jump", "jump")], ""),
    ("IfLoadedLessThan", [("value", "result"), ("jump", "jump")], ""),
    ("IfLoadedGreaterThan", [("value", "result"), ("jump", "jump")], ""),
    ("IfLoadedEqualTo", [("value", "result"), ("jump", "jump")], ""),
    ("IfLoadedNotEqualTo", [("value", "result"), ("jump", "jump")], ""),
    ("IfLoadedMask", [("mask", "value"), ("jump", "jump")], "Jumps if the loaded value has any of the bits"),
    ("IfLoadedNotMask", [("mask", "value"), ("jump", "jump")], ""),
    ("IfMoveEqualTo", [("move", "move"), ("jump", "jump")], "Compares the move being scored"),
    ("IfMoveNotEqualTo", [("move", "move"), ("jump", "jump")], ""),
    ("IfLoadedInTable", [("table", "list"), ("jump", "jump")], ""),
    ("IfLoadedNotInTable", [("table", "list"), ("jump", "jump")], ""),
    ("IfAttackerHasDamagingMoves", [("jump", "jump")], "Jumps if the attacker has a move with power"),
    ("IfAttackerHasNoDamagingMoves", [("jump", "jump")], ""),
    ("LoadTurnCount", [], ""),
    ("LoadTypeFrom", [("target", "type_target")], "LOAD_*"),
    ("LoadMovePower", [], "The base power of the move being scored"),
    ("FlagMoveDamageScore", [("damage", "damage_roll")], "Loads AI_MOVE_DEALS_NO_DAMAGE, AI_NOT_HIGHEST_DAMAGE if "
     "another of the attacker's moves deals more damage, or AI_MOVE_IS_HIGHEST_DAMAGE"),
    ("LoadBattlerPreviousMove", [("battler", "battler")], ""),
    ("IfTempEqualTo", [("value", "result"), ("jump", "jump")], "The same as IfLoadedEqualTo, as in Gen 4"),
    ("IfTempNotEqualTo", [("value", "result"), ("jump", "jump")], "The same as IfLoadedNotEqualTo"),
    ("IfSpeedCompareEqualTo", [("compare", "compare_speed"), ("jump", "jump")], "Compares the attacker's speed with "
     "the defender's, COMPARE_SPEED_*"),
    ("CountAlivePartyBattlers", [("battler", "battler")], "Loads the number of the party's Pokemon that aren't in "
     "battle and can battle"),
    ("LoadCurrentMove", [], ""),
    ("LoadCurrentMoveEffect", [], ""),
    ("LoadBattlerAbility", [("battler", "battler")], "Loads the ability that the AI knows the battler to have, or its "
     "guess"),
    ("Dummy2B", [], "Does nothing. Gen 4's calculates the highest type effectiveness"),
    ("IfMoveEffectivenessEquals", [("effectiveness", "effectiveness"), ("jump", "jump")], "TYPE_EFFECTIVENESS_* of "
     "the move being scored"),
    ("IfPartyMemberNotStatus", [("battler", "battler"), ("jump", "jump")], "Jumps if one of the party's Pokemon that "
     "aren't in battle has no major status condition"),
    ("IfPartyMemberStatus", [("battler", "battler"), ("jump", "jump")], "Jumps if one of the party's Pokemon that "
     "aren't in battle has a major status condition"),
    ("LoadCurrentWeather", [], ""),
    ("IfCurrentMoveEffectEqualTo", [("effect", "effect"), ("jump", "jump")], ""),
    ("IfCurrentMoveEffectNotEqualTo", [("effect", "effect"), ("jump", "jump")], ""),
    ("IfStatStageLessThan", [("battler", "battler"), ("stat", "stat"), ("stage", "value"), ("jump", "jump")],
     "Stages go from 0 to 12, and 6 is neutral"),
    ("IfStatStageGreaterThan", [("battler", "battler"), ("stat", "stat"), ("stage", "value"), ("jump", "jump")], ""),
    ("IfStatStageEqualTo", [("battler", "battler"), ("stat", "stat"), ("stage", "value"), ("jump", "jump")], ""),
    ("IfStatStageNotEqualTo", [("battler", "battler"), ("stat", "stat"), ("stage", "value"), ("jump", "jump")], ""),
    ("IfCurrentMoveKills", [("unused", "value"), ("jump", "jump")], "Jumps if the move's damage with USE_MIN_DAMAGE "
     "is at least the defender's HP. The first argument is not used"),
    ("IfCurrentMoveDoesNotKill", [("unused", "value"), ("jump", "jump")], ""),
    ("IfMoveKnown", [("battler", "battler"), ("move", "move"), ("jump", "jump")], "For the defender, only the moves "
     "it was seen to use"),
    ("IfMoveNotKnown", [("battler", "battler"), ("move", "move"), ("jump", "jump")], ""),
    ("IfMoveEffectKnown", [("battler", "battler"), ("effect", "effect"), ("jump", "jump")], ""),
    ("IfMoveEffectNotKnown", [("battler", "battler"), ("effect", "effect"), ("jump", "jump")], ""),
    ("Dummy3C", [], "Does nothing"),
    ("Escape", [], "Flees from the battle"),
    ("Dummy3E", [], "Does nothing. Gen 4's takes an argument"),
    ("Dummy3F", [], "Does nothing"),
    ("LoadHeldItem", [("battler", "battler")], ""),
    ("LoadHeldItemEffect", [("battler", "battler")], "HOLD_EFFECT_*"),
    ("LoadGender", [("battler", "battler")], ""),
    ("LoadIsFirstTurnInBattle", [("battler", "battler")], "Loads TRUE if condition flag 0 is clear, which is when "
     "Fake Out works"),
    ("LoadStockpileCount", [("battler", "battler")], ""),
    ("LoadBattleStyle", [], "BTL_STYLE_*"),
    ("LoadBattleType", [], ""),
    ("LoadRecycleItem", [("battler", "battler")], "Loads the item that the battler consumed"),
    ("Dummy48", [], "Does nothing"),
    ("LoadPowerOfLoadedMove", [], "Replaces the loaded move with its power"),
    ("LoadEffectOfLoadedMove", [], "Replaces the loaded move with its effect"),
    ("LoadProtectChain", [("battler", "battler")], "Loads how many times in a row the battler used Protect, Detect "
     "or Endure"),
    ("GoTo", [("jump", "jump")], ""),
    ("End", [], "Ends the script for this move"),
    ("IfLevel", [("compare", "compare_level"), ("jump", "jump")], "Compares the attacker's level with the "
     "defender's, CHECK_*"),
    ("IfTargetIsTaunted", [("jump", "jump")], ""),
    ("IfTargetIsNotTaunted", [("jump", "jump")], ""),
    ("IfTargetIsPartner", [("jump", "jump")], ""),
    ("FlagBattlerIsType", [("battler", "battler"), ("type", "type")], "Loads TRUE if the battler has the type"),
    ("CheckBattlerAbility", [("battler", "battler"), ("ability", "ability")], "Loads TRUE if the ability that "
     "LoadBattlerAbility would load is this one"),
    ("IfActivatedFlashFire", [("battler", "battler"), ("jump", "jump")], ""),
    ("IfHeldItemEqualTo", [("battler", "battler"), ("item", "item"), ("jump", "jump")], ""),
    ("IfFieldCondition", [("condition", "field_condition"), ("jump", "jump")], "Gen 4's takes a mask of conditions"),
    ("LoadSideCondition", [("battler", "battler"), ("condition", "side_condition")], "Loads the value of the "
     "condition on the battler's side, such as the layers of Spikes"),
    ("IfAnyPartyMemberIsWounded", [("battler", "battler"), ("jump", "jump")], "Meant to jump if one of the party's "
     "Pokemon that aren't in battle has lost HP, but checks the battler's HP instead"),
    ("IfAnyPartyMemberUsedPP", [("battler", "battler"), ("jump", "jump")], "Meant to jump if one of the party's "
     "Pokemon that aren't in battle has used PP, but checks the battler's PP instead"),
    ("LoadFlingPower", [("battler", "battler")], ""),
    ("LoadCurrentMovePP", [], ""),
    ("IfCanUseLastResort", [("battler", "battler"), ("jump", "jump")], ""),
    ("LoadCurrentMoveClass", [], "MOVE_CATEGORY_*"),
    ("LoadDefenderLastUsedMoveClass", [], ""),
    ("LoadBattlerSpeedRank", [("battler", "battler")], "Loads the battler's place in the order of the turn"),
    ("LoadBattlerUnk60", [("battler", "battler")], "Loads a 16-bit value of the battler. Gen 4's command here loads "
     "the turns since the battler switched in"),
    ("IfPartyMemberDealsMoreDamage", [("damage", "damage_roll"), ("jump", "jump")], "Jumps if one of the party's "
     "Pokemon that aren't in battle deals more damage to the defender than the attacker"),
    ("IfHasSuperEffectiveMove", [("jump", "jump")], ""),
    ("IfBattlerDealsMoreDamage", [("battler", "battler"), ("damage", "damage_roll"), ("jump", "jump")], "Jumps if "
     "the battler's previous move deals more damage to the defender than the attacker's moves"),
    ("SumPositiveStatStages", [("battler", "battler")], ""),
    ("DiffStatStages", [("battler", "battler"), ("stat", "stat")], "Loads the battler's stat stage minus the "
     "attacker's"),
    ("Dummy66", [], "Does nothing"),
    ("Dummy67", [], "Does nothing"),
    ("Dummy68", [], "Does nothing"),
    ("CheckIfHighestDamageWithPartner", [("damage", "damage_roll")], "Like FlagMoveDamageScore, with the moves of "
     "the attacker's partners too"),
    ("IfBattlerFainted", [("battler", "battler"), ("jump", "jump")], ""),
    ("IfBattlerNotFainted", [("battler", "battler"), ("jump", "jump")], ""),
    ("LoadAbility", [("battler", "battler")], "Loads the battler's actual ability"),
    ("IfBattlerHasSubstitute", [("battler", "battler"), ("jump", "jump")], ""),
    ("LoadSpecies", [("battler", "battler")], ""),
    ("IfTurnRandomLessThan", [("value", "value"), ("jump", "jump")], "Like IfRandomLessThan, with a random number "
     "that stays the same for the turn"),
    ("IfTurnRandomGreaterThan", [("value", "value"), ("jump", "jump")], ""),
    ("IfTurnRandomEqualTo", [("value", "value"), ("jump", "jump")], ""),
    ("IfTurnRandomNotEqualTo", [("value", "value"), ("jump", "jump")], ""),
    ("GoToByMoveEffect", [("mode", "value"), ("max", "value"), ("table", "table")], "Jumps to the table's entry for "
     "the move's effect, or ends the script for effects above max. Modes other than 0 end the script"),
    ("IfUnk74", [("battler", "battler"), ("jump", "jump")], "Jumps if position effect 3 is active at the battler's "
     "position"),
    ("IfAttackLessThanSpAttack", [("battler", "battler"), ("jump", "jump")], "Meant to compare the battler's Attack "
     "with its Special Attack, but compares its position with its Special Attack"),
    ("IfAttackGreaterThanSpAttack", [("battler", "battler"), ("jump", "jump")], ""),
    ("IfAttackEqualToSpAttack", [("battler", "battler"), ("jump", "jump")], ""),
]

# The headers that the scripts include, and the prefix of the constants for each kind of value
CONSTANTS = {
    "battler": ("constants/tr_ai.h", "AI_BATTLER_"),
    "type_target": ("constants/tr_ai.h", "LOAD_"),
    "damage_roll": ("constants/tr_ai.h", "(?:USE_MIN_DAMAGE|ROLL_FOR_DAMAGE)"),
    "damage_rank": ("constants/tr_ai.h", "AI_(?:MOVE_DEALS_NO|NOT_HIGHEST|MOVE_IS_HIGHEST)_"),
    "compare_speed": ("constants/tr_ai.h", "COMPARE_SPEED_"),
    "compare_level": ("constants/tr_ai.h", "CHECK_"),
    "move": ("constants/moves.h", "MOVE_"),
    "ability": ("constants/abilities.h", "ABILITY_"),
    "item": ("constants/items.h", "ITEM_"),
    "species": ("constants/species.h", "SPECIES_"),
    "type": ("constants/types.h", "TYPE_"),
    "effect": ("constants/move_effects.h", "BATTLE_EFFECT_"),
    "hold_effect": ("constants/hold_effects.h", "HOLD_EFFECT_"),
    "effectiveness": ("constants/battle.h", "TYPE_EFFECTIVENESS_"),
    "battle_style": ("constants/battle.h", "BTL_STYLE_"),
    "category": ("constants/battle.h", "MOVE_CATEGORY_"),
    "condition": ("constants/battle.h", "CONDITION_"),
    "stat": ("constants/battle.h", "BATTLEMON_(?!STAT_)\\w+_STAGE"),
    "weather": ("constants/battle.h", "BTL_WEATHER_"),
    "side_condition": ("constants/battle.h", "SIDE_CONDITION_"),
    "field_condition": ("constants/battle.h", "FIELD_CONDITION_"),
    "gender": ("constants/pokemon.h", "GENDER_"),
}
# Values that are not in a header
BOOLEANS = {0: "FALSE", 1: "TRUE"}
# The commands that set the result, with the kind of value they set where it is one of the kinds in CONSTANTS
RESULTS = {
    "LoadTurnCount": None,
    "LoadTypeFrom": "type",
    "LoadMovePower": None,
    "FlagMoveDamageScore": "damage_rank",
    "LoadBattlerPreviousMove": "move",
    "CountAlivePartyBattlers": None,
    "LoadCurrentMove": "move",
    "LoadCurrentMoveEffect": "effect",
    "LoadBattlerAbility": "ability",
    "LoadCurrentWeather": "weather",
    "LoadHeldItem": "item",
    "LoadHeldItemEffect": "hold_effect",
    "LoadGender": "gender",
    "LoadIsFirstTurnInBattle": "bool",
    "LoadStockpileCount": None,
    "LoadBattleStyle": "battle_style",
    "LoadBattleType": None,
    "LoadRecycleItem": "item",
    "LoadPowerOfLoadedMove": None,
    "LoadEffectOfLoadedMove": "effect",
    "LoadProtectChain": None,
    "FlagBattlerIsType": "bool",
    "CheckBattlerAbility": "bool",
    "LoadSideCondition": None,
    "LoadFlingPower": None,
    "LoadCurrentMovePP": None,
    "LoadCurrentMoveClass": "category",
    "LoadDefenderLastUsedMoveClass": "category",
    "LoadBattlerSpeedRank": None,
    "LoadBattlerUnk60": None,
    "SumPositiveStatStages": None,
    "DiffStatStages": None,
    "CheckIfHighestDamageWithPartner": "damage_rank",
    "LoadAbility": "ability",
    "LoadSpecies": "species",
}
# The scripts, in archive order, which is the bit of the AI flag that runs each (AI_FLAG_* in constants/tr_ai.h)
# Commands after which the script doesn't continue: jump, end and jump_by_move_effect
NO_FALLTHROUGH = {76, 77, 115}
REFERENCES = ("jump", "list", "table")
LIST_END = 0xFFFFFFFF


def load_constants() -> dict[str, dict[int, str]]:
    constants = {}
    for kind, (header, prefix) in CONSTANTS.items():
        names = {}
        for match in re.finditer(rf"^#define ({prefix}\w*) (\d+)$", header_text(header), re.MULTILINE):
            names.setdefault(int(match[2]), match[1])
        constants[kind] = names
    constants["bool"] = BOOLEANS
    return constants


def write_inc(path: Path):
    headers = sorted({header for header, _ in CONSTANTS.values()})
    lines = [
        "// Macros for the trainer AI scripts, written by tools/script/tr_ai_script.py inc",
        "",
        *(f'#include "{header}"' for header in headers),
        "",
        "    // In .data, which LLVM pads with zeros: in .text, clang 18 pads `.balign 4, 0` with ARM NOPs",
        "    .data",
        "",
        "    .set FALSE, 0",
        "    .set TRUE, 1",
        "",
        "    .macro AICommand id",
        "    .2byte \\id",
        "    .endm",
        "",
        "    // An entry of a table for IfLoadedInTable, which ends with TABLE_END",
        "    .macro TableEntry entry",
        "    .4byte \\entry",
        "    .endm",
        "",
        "    // An entry of a table for GoToByMoveEffect: the label to jump to, from the table's start",
        "    .macro LabelDistance dst, src",
        "    .4byte \\dst - \\src",
        "    .endm",
        "",
    ]
    for cmd_id, (name, params, doc) in enumerate(COMMANDS):
        if doc:
            lines.append(f"    // {doc}")
        lines.append(f"    .macro {name}{' ' if params else ''}{', '.join(param for param, _ in params)}")
        lines.append(f"    AICommand {cmd_id}")
        for param, kind in params:
            if kind in REFERENCES:
                lines.append(f"    .4byte \\{param} - (. + 4)")
            else:
                lines.append(f"    .4byte \\{param}")
        lines.append("    .endm")
        lines.append("")
    path.write_text("\n".join(lines))


def s32(value: int) -> int:
    return value - (1 << 32) if value & 0x80000000 else value


class Script:
    def __init__(self, data: bytes, label_prefix: str, constants: dict[str, dict[int, str]] | None = None,
                 names: dict[int, str] | None = None):
        self.data = data
        self.prefix = label_prefix
        self.constants = constants or {}
        # Label names by offset, for labels that would otherwise be named after their offset
        self.names = names or {}
        self.instructions: dict[int, tuple[int, list[tuple[str, int]], int]] = {}
        self.labels: set[int] = {0}
        self.lists: dict[int, int] = {}  # start -> end
        self.tables: dict[int, int] = {}  # start -> entry count

    def label(self, offset: int) -> str:
        return self.names.get(offset, f"{self.prefix}_{offset:04X}")

    def decode(self, pc: int):
        if pc + 2 > len(self.data):
            return None
        cmd_id = struct.unpack_from("<H", self.data, pc)[0]
        if cmd_id >= len(COMMANDS):
            return None
        kinds = [kind for _, kind in COMMANDS[cmd_id][1]]
        end = pc + 2 + 4 * len(kinds)
        if end > len(self.data):
            return None
        args = []
        for i, kind in enumerate(kinds):
            field = pc + 2 + 4 * i
            value = struct.unpack_from("<I", self.data, field)[0]
            if kind in REFERENCES:
                target = field + 4 + s32(value)
                # A target outside the file means these bytes aren't this command
                if not 0 <= target < len(self.data):
                    return None
                args.append((kind, target))
            else:
                args.append((kind, value))
        return cmd_id, args, end

    def trace(self):
        pending = [0]
        while pending:
            pc = pending.pop()
            while pc not in self.instructions:
                decoded = self.decode(pc)
                if decoded is None:
                    break
                cmd_id, args, end = decoded
                self.instructions[pc] = decoded
                for kind, target in args:
                    if kind == "jump":
                        self.labels.add(target)
                        pending.append(target)
                    elif kind == "list":
                        self.labels.add(target)
                        self.trace_list(target)
                    elif kind == "table":
                        self.labels.add(target)
                        mode, highest = args[0][1], args[1][1]
                        if mode == 0:
                            self.tables[target] = highest + 1
                            for i in range(highest + 1):
                                entry = struct.unpack_from("<I", self.data, target + 4 * i)[0]
                                self.labels.add(target + entry)
                                pending.append(target + entry)
                if cmd_id in NO_FALLTHROUGH:
                    break
                pc = end

    def trace_list(self, start: int):
        pc = start
        while pc + 4 <= len(self.data):
            value = struct.unpack_from("<I", self.data, pc)[0]
            pc += 4
            if value == LIST_END:
                break
        self.lists[start] = pc

    def fill_gaps(self):
        """Decodes what no path reaches. Each gap is taken in order: commands where they trace cleanly from the next
        undecoded offset, lists where the values end with 0xffffffff, and raw bytes for the rest."""
        for gap_start, gap_end in self.gaps():
            pos = gap_start
            while pos < gap_end:
                if self.try_code(pos, gap_end) or self.try_list(pos, gap_end):
                    covered = self.covered()
                    while pos < gap_end and pos in covered:
                        pos += 1
                else:
                    break

    def gaps(self) -> list[tuple[int, int]]:
        covered = self.covered()
        gaps = []
        pc = 0
        while pc < len(self.data):
            if pc in covered:
                pc += 1
                continue
            start = pc
            while pc < len(self.data) and pc not in covered:
                pc += 1
            gaps.append((start, pc))
        return gaps

    def try_code(self, start: int, gap_end: int) -> bool:
        """Traces from start, keeping the result only if it stays inside the gap and its jumps land on the starts
        of commands."""
        trial = Script(self.data, self.prefix)
        trial.instructions = dict(self.instructions)
        trial.lists = dict(self.lists)
        trial.tables = dict(self.tables)
        trial.labels = set(self.labels)
        covered = self.covered()
        pending = [start]
        while pending:
            pc = pending.pop()
            while pc not in trial.instructions:
                if not start <= pc < gap_end or pc in covered:
                    return False
                decoded = trial.decode(pc)
                if decoded is None or any(o in covered for o in range(pc, decoded[2])):
                    return False
                cmd_id, args, end = decoded
                trial.instructions[pc] = decoded
                for kind, target in args:
                    if kind in REFERENCES:
                        trial.labels.add(target)
                    if kind == "jump":
                        pending.append(target)
                    elif kind == "list":
                        if target not in trial.lists:
                            trial.trace_list(target)
                    elif kind == "table":
                        return False
                if cmd_id in NO_FALLTHROUGH:
                    break
                pc = end
        if any(not start <= o < gap_end for o in trial.covered() - covered):
            return False
        starts = set(trial.instructions)
        for pc, (_, args, _) in trial.instructions.items():
            if pc not in self.instructions:
                if any(kind == "jump" and target not in starts for kind, target in args):
                    return False
        self.instructions, self.lists, self.labels = trial.instructions, trial.lists, trial.labels
        return True

    def try_list(self, start: int, gap_end: int) -> bool:
        """Takes 32-bit values up to and including a 0xffffffff inside the gap as a list."""
        pc = start
        while pc + 4 <= gap_end:
            value = struct.unpack_from("<I", self.data, pc)[0]
            pc += 4
            if value == LIST_END:
                self.lists[start] = pc
                return True
        return False

    def covered(self) -> set[int]:
        covered = set()
        for pc, (_, _, end) in self.instructions.items():
            covered.update(range(pc, end))
        for start, end in self.lists.items():
            covered.update(range(start, end))
        for start, count in self.tables.items():
            covered.update(range(start, start + 4 * count))
        return covered

    def boundaries(self) -> set[int]:
        """The offsets where a label can go: the start of each command, list or table entry and raw byte."""
        covered = self.covered()
        starts = set(self.instructions)
        for start, end in self.lists.items():
            starts.update(range(start, end, 4))
        for start, count in self.tables.items():
            starts.update(range(start, start + 4 * count, 4))
        starts.update(o for o in range(len(self.data)) if o not in covered)
        starts.add(len(self.data))
        return starts

    def successors(self, pc: int) -> list[int]:
        cmd_id, args, end = self.instructions[pc]
        targets = [target for kind, target in args if kind == "jump"]
        for kind, table in args:
            if kind == "table" and table in self.tables:
                targets += [table + struct.unpack_from("<I", self.data, table + 4 * i)[0]
                            for i in range(self.tables[table])]
        if cmd_id not in NO_FALLTHROUGH:
            targets.append(end)
        return [target for target in targets if target in self.instructions]

    def result_kinds(self) -> dict[int, str | None]:
        """Returns the kind of value in the result when each command runs, where every way to the command agrees."""
        predecessors = {pc: [] for pc in self.instructions}
        for pc in self.instructions:
            for target in self.successors(pc):
                predecessors[target].append(pc)
        before = {pc: None for pc, sources in predecessors.items() if pc == 0 or not sources}
        pending = list(before)
        while pending:
            pc = pending.pop()
            name = COMMANDS[self.instructions[pc][0]][0]
            after = RESULTS[name] if name in RESULTS else before[pc]
            for target in self.successors(pc):
                if target not in before:
                    before[target] = after
                elif before[target] != after and before[target] is not None:
                    before[target] = None
                else:
                    continue
                pending.append(target)
        return before

    def format_arg(self, kind: str, value: int) -> str:
        if kind in REFERENCES:
            if value not in self.placeable:
                # The target is inside another command, so give it as an offset from this argument's end
                return f". + 4 + {value - self.current_field - 4}"
            return self.label(value)
        if kind == "result":
            kind = self.current_result
        name = self.constants.get(kind, {}).get(value)
        if name is not None:
            return name
        signed = s32(value)
        return str(signed) if -0x10000 < signed < 0x10000 else f"{value:#x}"

    def disassemble(self) -> str:
        self.trace()
        self.fill_gaps()
        self.labels.update(self.lists)
        self.placeable = self.boundaries()
        self.labels.update(o for o in self.names if o in self.placeable)
        results = self.result_kinds()
        # A list holds the kind of value that the result holds where the list is used, if that is always the same
        list_kinds = {}
        for pc, (cmd_id, args, _) in self.instructions.items():
            for kind, target in args:
                if kind == "list":
                    list_kinds.setdefault(target, set()).add(results.get(pc))
        # The files are padded with zeros to a multiple of 4 bytes
        covered = self.covered()
        padded = len(self.data)
        size = max(covered) + 1 if covered else 0
        if not (0 < padded - size < 4 and padded % 4 == 0 and all(o in covered for o in range(size))
                and not any(self.data[size:])):
            size = padded
        out = []
        pc = 0
        while pc < size:
            if pc in self.instructions:
                self.emit_label(out, pc)
                cmd_id, args, end = self.instructions[pc]
                self.current_result = results.get(pc)
                parts = []
                for i, (kind, value) in enumerate(args):
                    self.current_field = pc + 2 + 4 * i
                    parts.append(self.format_arg(kind, value))
                text = ", ".join(parts)
                out.append(f"    {COMMANDS[cmd_id][0]}{' ' if text else ''}{text}")
                pc = end
            elif pc in self.lists:
                kinds = list_kinds.get(pc, set())
                self.current_result = next(iter(kinds)) if len(kinds) == 1 else None
                for element in range(pc, self.lists[pc], 4):
                    self.emit_label(out, element)
                    value = struct.unpack_from("<I", self.data, element)[0]
                    entry = "TABLE_END" if value == LIST_END else self.format_arg("result", value)
                    out.append(f"    TableEntry {entry}")
                pc = self.lists[pc]
            elif pc in self.tables:
                table = pc
                for entry in range(table, table + 4 * self.tables[table], 4):
                    self.emit_label(out, entry)
                    target = table + struct.unpack_from("<I", self.data, entry)[0]
                    out.append(f"    LabelDistance {self.label(target)}, {self.label(table)}")
                pc = table + 4 * self.tables[table]
            else:
                self.emit_label(out, pc)
                out.append(f"    .byte {self.data[pc]:#04x}")
                pc += 1
        if size < padded:
            out.append("    .balign 4")
        self.emit_label(out, len(self.data))
        return "\n".join(out) + "\n"

    def emit_label(self, out: list[str], offset: int):
        if offset in self.labels:
            if out and out[-1]:
                out.append("")
            out.append(f"{self.label(offset)}:")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    inc = commands.add_parser("inc")
    inc.add_argument("output", type=Path)
    disasm = commands.add_parser("disasm")
    disasm.add_argument("archive", type=Path)
    disasm.add_argument("output", type=Path)
    disasm.add_argument("--labels", type=Path, help="JSON of label names by script and offset")
    args = parser.parse_args()

    if args.command == "inc":
        args.output.parent.mkdir(parents=True, exist_ok=True)
        write_inc(args.output)
    else:
        args.output.mkdir(parents=True, exist_ok=True)
        constants = load_constants()
        labels = json.loads(args.labels.read_text()) if args.labels else {}
        # The files are named after their constants in data/constants/tr_ai_scripts.txt, in its order
        paths = ordered_files("tr_ai_scripts", "TR_AI_SCRIPT_", args.output, ".s")
        for i, data in enumerate(read_narc(args.archive.read_bytes())):
            names = {int(offset): name for offset, name in labels.get(str(i), {}).items()}
            script = Script(data, f"TrAI{i:02d}", constants, names)
            text = f'#include "asm/tr_ai.inc"\n\n{script.disassemble()}'
            (args.output / paths[i].name).write_text(text)


if __name__ == "__main__":
    main()

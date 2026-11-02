#!/usr/bin/env python3
"""Disassemble the battle effect VM's scripts, archives a/0/6/5 (moves) and a/0/6/6 (other effects), and write the
macros they are assembled with.

    btlv_effvm_script.py inc include/asm/btlv_effvm.inc           # write the assembler macros
    btlv_effvm_script.py disasm ARCHIVE OUTPUT_DIR moves|effects  # write one .s file per script
    btlv_effvm_script.py sync [--dry-run]                         # rename the commands after their handlers

src/ov168/btlv_effvm.c runs them (BtlvEffvm_Start). Effect numbers below BTLV_EFFECT_FIRST are moves, whose script is
the member of a/0/6/5 with the move's number; the rest are the members of a/0/6/6 from BTLV_EFFECT_FIRST up. A script
file starts with the number of its variants, the turns of a two-turn move or the cases of a move that looks different
(BtlvEffvmParam's variant), and the offset of each variant's code from the file's start, 14 times over: the game
reads only the first. A command is a 16-bit ID, the index of its handler in btlv_effvm.c's MOVE_SCRCMD, followed by
32-bit arguments, which COMMANDS gives from what each handler reads. Jumps are relative to the end of the command.

The commands are named after their handlers, as the field scripts' are: BtlvEffvm_CmdCameraMove is CameraMove, and
the macros follow MOVE_SCRCMD, so that a handler renamed with rename_symbol.py renames its command with `sync`, which
writes the macros again and renames the command in res/battle/animations.

The .s files go through the C preprocessor, so that they can use the constants in include/constants, and assemble
back to the original bytes. The scripts in res/battle/animations are source, edited by hand; disassembling them again
is for reference.
"""
import argparse
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools.data.gen_constants import header_text  # noqa: E402
from tools.data.gen_constants import load  # noqa: E402
from tools.data.gen_constants import ordered_names  # noqa: E402
from tools.data.narc import read_narc  # noqa: E402

SOURCE = ROOT / "src" / "ov168" / "btlv_effvm.c"
INC = ROOT / "include" / "asm" / "btlv_effvm.inc"
SCRIPTS = ROOT / "res" / "battle" / "animations"
# Each variant's offset is written this many times; BtlvEffvm_Start reads the first
VARIANT_COPIES = 14
# A handler is BtlvEffvm_Cmd<Name>, or BtlvEffect_<Name> for the one swan names, and the command is its <Name>
HANDLER_PREFIX = re.compile(r"^(?:BtlvEffvm_Cmd|BtlvEffvm_|BtlvEffect_)")

# The arguments of each command, in ID order, with their kinds, and what the command does. The kinds:
#   value    a 32-bit value
#   fx       an fx32, 0x1000 for 1, written in hex
#   color    a color, RGB555, written in hex
#   jump     a jump target, relative to the end of the command
#   effect   an effect number: a move (MOVE_*) or BTLV_EFFECT_FIRST + the member of a/0/6/6
#   var      a script variable, BtlvEffvm_GetVariable's ID (EFFVM_VAR_*)
#   operand  a value compared with a variable, written as TRUE or FALSE where the variable is a flag
# and the kinds in CONSTANTS. The particle resources are the members of ARCID_BTLV_EFFVM_PARTICLES
# (res/particles/btlv_effvm_particles), and the emitters their emitters, by number.
CAMERA_MOVE = [("frames", "value"), ("wait", "value"), ("brake", "value")]
EMIT_SCALES = [("radius_scale", "fx"), ("life_scale", "fx"), ("scale_scale", "fx"), ("camera_scale", "fx")]
MCSS_STEPS = [("frames", "value"), ("wait", "value"), ("times", "value")]
COMMANDS = [
    ([("move_type", "value"), ("camera", "value"), *CAMERA_MOVE], "Moves the camera to one of its preset places"),
    ([("move_type", "value"), ("x", "fx"), ("y", "fx"), ("z", "fx"), ("target_x", "fx"), ("target_y", "fx"),
      ("target_z", "fx"), *CAMERA_MOVE], "Moves the camera and its target"),
    ([("move_type", "value"), ("pitch", "value"), ("yaw", "value"), *CAMERA_MOVE], ""),
    ([("dir", "value"), ("amplitude", "fx"), ("unused", "value"), ("frames", "value"), ("wait", "value"),
      ("count", "value")], ""),
    ([("type", "value"), ("attacker_only", "value")], "0 for attacker_only sets how the Pokemon face the camera"),
    ([], "Saves the camera's place, which camera preset 13 returns to"),
    ([("particle", "value")], "Loads a particle resource"),
    ([("particle", "value"), ("emitter", "value"), ("from", "emit"), ("to", "emit"), ("offset_y", "fx"),
      ("unk18", "value"), ("unused", "value"), *EMIT_SCALES], "Adds an emitter; to 8 is the same as from"),
    ([("particle", "value"), ("emitter", "value"), ("from_x", "fx"), ("from_y", "fx"), ("from_z", "fx"),
      ("to_x", "fx"), ("to_y", "fx"), ("to_z", "fx"), ("offset_y", "fx"), ("unk18", "value"), ("unused", "value"),
      *EMIT_SCALES], "Adds an emitter between two points"),
    ([("particle", "value"), ("emitter", "value"), ("from", "emit"), ("to", "emit"), ("offset_x", "fx"),
      ("offset_y", "fx"), ("offset_z", "fx"), *EMIT_SCALES], ""),
    ([("particle", "value"), ("from", "emit"), ("to", "emit"), ("offset_y", "fx"), ("unk18", "value"),
      ("ortho", "bool"), *EMIT_SCALES], "Adds every emitter of the resource"),
    ([("particle", "value")], "Deletes a particle resource and its emitters"),
    ([("particle", "value"), ("emitter", "value"), ("motion", "value"), ("from", "emit"), ("to", "emit"),
      ("offset_y", "fx"), ("frames", "fx"), ("height", "fx"), ("life_scale", "fx"), ("camera_scale", "fx"),
      ("turns", "fx")], "Adds an emitter that moves from from to to"),
    ([("particle", "value"), ("emitter", "value"), ("motion", "value"), ("from_x", "fx"), ("from_y", "fx"),
      ("from_z", "fx"), ("to", "emit"), ("offset_y", "fx"), ("frames", "fx"), ("height", "fx"), ("life_scale", "fx"),
      ("camera_scale", "fx"), ("turns", "fx")], ""),
    ([("particle", "value"), ("emitter", "value"), ("motion", "value"), ("from", "emit"), ("to", "emit"),
      ("offset_y", "fx"), ("frames", "fx"), ("height", "fx"), ("life_scale", "fx"), ("camera_scale", "fx"),
      ("turns", "fx")], ""),
    ([("particle", "value"), ("emitter", "value"), ("motion", "value"), ("from_x", "fx"), ("from_y", "fx"),
      ("from_z", "fx"), ("to", "emit"), ("offset_y", "fx"), ("frames", "fx"), ("height", "fx"), ("life_scale", "fx"),
      ("camera_scale", "fx"), ("scale_scale", "fx")], ""),
    ([("particle", "value"), ("emitter", "value"), ("type", "value"), ("radius_x", "fx"), ("radius_z", "fx"),
      ("offset_y", "fx"), ("frames", "value"), ("wait", "value"), ("unk30", "value"), ("delay", "value")],
     "Adds an emitter that circles the attacker (type 0, 1), the defender (2, 3) or the origin (4, 5)"),
    ([("particle", "value"), ("emitter", "value"), ("type", "value"), ("radius_x", "fx"), ("radius_z", "fx"),
      ("offset_y", "fx"), ("frames", "value"), ("wait", "value"), ("unk30", "value"), ("delay", "value")], ""),
    ([("targets", "targets"), ("type", "value"), ("x", "fx"), ("y", "fx"), *MCSS_STEPS], "Moves the Pokemon"),
    ([("targets", "targets"), ("mode", "value"), ("start", "value"), ("radius1", "fx"), ("radius2", "fx"),
      ("frames", "fx"), ("wait", "fx"), ("count", "fx"), ("turn_wait", "value")], "Moves the Pokemon around a circle"),
    ([("targets", "targets"), ("axis", "value"), ("start_angle", "fx"), ("end_angle", "fx"), ("amplitude", "fx"),
      ("frames", "value")], ""),
    ([("targets", "targets"), ("type", "value"), ("x", "fx"), ("y", "fx"), *MCSS_STEPS], ""),
    ([("targets", "targets"), ("type", "value"), ("angle", "value"), *MCSS_STEPS], ""),
    ([("targets", "targets"), ("type", "value"), ("alpha", "value"), *MCSS_STEPS], ""),
    ([("targets", "targets"), ("type", "value"), ("level", "value"), *MCSS_STEPS], ""),
    ([("targets", "targets"), ("type", "value"), ("wait", "value"), ("times", "value")], ""),
    ([("targets", "targets"), ("vanish", "bool")], "Hides or shows the Pokemon"),
    ([("targets", "targets"), ("start_evy", "value"), ("end_evy", "value"), ("wait", "value"), ("color", "color")],
     ""),
    ([("targets", "targets"), ("mode", "value")], ""),
    ([("targets", "targets"), ("value", "value")], ""),
    ([("targets", "targets"), ("type", "value"), ("x", "fx"), ("y", "fx"), *MCSS_STEPS], ""),
    ([("targets", "targets")], "Hides the Pokemon's gauges"),
    ([("trainer_type", "value"), ("target", "target"), ("x", "value"), ("y", "value"), ("z", "value")],
     "Shows a trainer at a position; trainer_type -1 takes the script's value"),
    ([("trainer", "value"), ("type", "value"), ("x", "fx"), ("y", "fx"), ("z", "fx"), *MCSS_STEPS], ""),
    ([("pos", "value"), ("anim", "value")], ""),
    ([("trainer", "value")], ""),
    ([("file", "value")], "Loads a background into BG 3"),
    ([("mode", "value"), ("x", "value"), ("y", "value"), ("frames", "value"), ("wait", "value"), ("count", "value")],
     ""),
    ([("mode", "value"), ("amplitude", "fx"), ("step", "value"), ("frames", "value"), ("fade_mode", "value"),
      ("fade_frames", "value")], ""),
    ([("file", "value"), ("loops", "value")], "Animates BG 3's palette from a file of ARCID_BTLV_EFFVM_PAL_ANIM"),
    ([("priority", "value")], "Sets BG 3's priority"),
    ([("unused0", "value"), ("unused1", "value"), ("alpha", "value"), ("unused3", "value"), ("unused4", "value"),
      ("unused5", "value")], ""),
    ([("fade", "value"), ("evy", "value"), ("target_evy", "value"), ("wait", "value"), ("color", "value")],
     "Fades the stage's, the field's or the palettes' colors (BtlvEffect_StartPaletteFade)"),
    ([("what", "value"), ("hide", "value")], "Hides or shows the stage, the field or BG 3 (BtlvEffect_SetVanish)"),
    ([("dir", "value"), ("win_h", "value"), ("win_v", "value"), ("win_in_out", "value"), ("count", "value"),
      ("wait", "value"), ("keep_window", "value")], "Opens or closes window 0 step by step"),
    ([("index", "value"), ("file", "value"), ("target", "target"), ("offset_x", "fx"), ("offset_y", "fx"),
      ("scale_x", "fx"), ("scale_y", "fx")], "Adds a cell actor"),
    ([("index", "value"), ("type", "value"), ("x", "fx"), ("y", "fx"), ("frames", "value"), ("wait", "value"),
      ("count", "value")], ""),
    ([("index", "value"), ("type", "value"), ("x", "fx"), ("y", "fx"), ("frames", "value"), ("wait", "value"),
      ("count", "value")], ""),
    ([("index", "value"), ("sequence", "value")], ""),
    ([("index", "value"), ("start", "value"), ("end", "value"), ("delay", "value"), ("color", "color")], ""),
    ([("index", "value")], ""),
    ([("mode", "value"), ("target", "value")], ""),
    ([("se", "se"), ("player", "value"), ("pan", "pan"), ("wait", "value"), ("pitch", "value"), ("volume", "value"),
      ("mod_depth", "value"), ("mod_speed", "value"), ("unused", "value")],
     "Plays a sound effect after wait frames; player 5 is the sound's own"),
    ([("player", "value")], ""),
    ([("player", "value"), ("type", "value"), ("from", "target"), ("to", "target"), ("delay", "value"),
      ("frames", "value"), ("step_wait", "value"), ("count", "value")], "Pans a sound from one side to the other"),
    ([("player", "value"), ("type", "value"), ("param", "value"), ("start", "value"), ("end", "value"),
      ("delay", "value"), ("frames", "value"), ("step_wait", "value"), ("count", "value")], ""),
    ([("what", "wait")], "Waits until what is done (EFFVM_WAIT_*)"),
    ([("frames", "value")], ""),
    ([("yield", "bool")], "Whether the commands return to the battle each frame"),
    ([("var", "var"), ("cmp", "cmp"), ("value", "operand"), ("jump", "jump")],
     "Jumps if the variable compares with the value so (EFFVM_CMP_*)"),
    ([("var", "var"), ("cmp", "cmp"), ("var2", "var"), ("jump", "jump")], ""),
    ([("pos", "value"), ("exists", "bool"), ("jump", "jump")], ""),
    ([("value", "value")], "Sets the script's value, variable EFFVM_VAR_VALUE"),
    ([("var", "var")], "Loads the variable into the script's value"),
    ([("var", "var"), ("value", "operand")], "Sets the variables that come from BtlvEffvmParam, and 53 to 56"),
    ([("type", "value"), ("targets", "targets"), ("arg", "value")], ""),
    ([], "Gives the attacker the defender's sprite, as Transform does"),
    ([("targets", "targets"), ("speed", "value"), ("volume", "value"), ("chorus_volume", "value"),
      ("chorus_speed", "value"), ("reverse", "value"), ("delay", "value")], "Plays the Pokemon's cries"),
    ([("source", "value")], "Whose ball the ball particles show: 8 for the effect's item, 9 the attacker's"),
    ([("index", "value"), ("target", "target"), ("offset_x", "fx"), ("offset_y", "fx"), ("scale_x", "fx"),
      ("scale_y", "fx")], ""),
    ([("effect", "effect"), ("attacker", "target"), ("defender", "target")], "Runs another effect's script and comes "
     "back; EFFVM_ATTACKER and EFFVM_DEFENDER keep the attacker and the defender"),
    ([], "Returns from Call"),
    ([("jump", "jump")], ""),
    ([], "Waits for btlv_effect.c to resume the script"),
    ([("effect", "effect")], "Goes on with another effect's script"),
    ([("side", "value")], ""),
    ([("check", "bool")], ""),
    ([], "Ends the script"),
]
# The commands after which the script doesn't go on: Return, Jump, ChangeScript and End
NO_FALLTHROUGH = {71, 72, 74, 77}

# The headers the scripts include, and the prefix of the constants of each kind
CONSTANTS = {
    "effect": ("constants/moves.h", "MOVE_"),
    "se": ("constants/sound.h", "SEQ_"),
}
# The VM's own codes, which the macros define: the positions BtlvEffvm_GetTargetPositions (targets) and
# BtlvEffvm_GetTargetPosition (target) take besides the positions 0 to 13 themselves, the emitters' positions of
# BtlvEffvm_InitEmitter (emit, 0 to 7 the positions, 8 the command's own point), the sides of BtlvEffvm_PlaySEAt's pan
# (or a target), the waits of BtlvEffvm_WaitNative, the comparisons of BtlvEffvm_JumpIf, and the variables of
# BtlvEffvm_GetVariable, where a variable of each position comes first and the attacker's after the eight
TARGETS = {14: "EFFVM_ATTACKER", 15: "EFFVM_ATTACKER_PARTNER", 16: "EFFVM_DEFENDER", 17: "EFFVM_DEFENDER_PARTNER"}
CODES = {
    "target": TARGETS,
    "targets": {**TARGETS, 18: "EFFVM_ALL", 19: "EFFVM_PLAYER_SIDE", 20: "EFFVM_ENEMY_SIDE"},
    "emit": {9: "EFFVM_EMIT_ATTACKER", 10: "EFFVM_EMIT_ATTACKER_MIRRORED", 11: "EFFVM_EMIT_DEFENDER",
             12: "EFFVM_EMIT_DEFENDER_MIRRORED"},
    "pan": {0: "EFFVM_PAN_LEFT", 1: "EFFVM_PAN_RIGHT", 2: "EFFVM_PAN_CENTER", **TARGETS},
    "wait": {0: "EFFVM_WAIT_ALL", 1: "EFFVM_WAIT_CAMERA", 2: "EFFVM_WAIT_PARTICLES", 3: "EFFVM_WAIT_MCSS",
             4: "EFFVM_WAIT_TRAINER_ANIM", 5: "EFFVM_WAIT_BG", 6: "EFFVM_WAIT_STAGE_FADE", 7: "EFFVM_WAIT_FIELD_FADE",
             8: "EFFVM_WAIT_STAGE_FIELD_FADE", 9: "EFFVM_WAIT_PALETTE_FADE", 10: "EFFVM_WAIT_SE",
             15: "EFFVM_WAIT_SE_PLAYER_0", 11: "EFFVM_WAIT_SE_PLAYER_1", 12: "EFFVM_WAIT_SE_PLAYER_2",
             14: "EFFVM_WAIT_SE_PLAYER_3", 13: "EFFVM_WAIT_SE_PLAYER_4", 16: "EFFVM_WAIT_CRY", 17: "EFFVM_WAIT_WINDOW",
             18: "EFFVM_WAIT_PAL_ANIM", 19: "EFFVM_WAIT_BG_WAVE"},
    "cmp": {0: "EFFVM_CMP_EQ", 1: "EFFVM_CMP_NE", 2: "EFFVM_CMP_LT", 3: "EFFVM_CMP_GT", 4: "EFFVM_CMP_LE",
            5: "EFFVM_CMP_GE"},
    "bool": {0: "FALSE", 1: "TRUE"},
}
# The variables: the name, the first ID, the number of IDs (8 for one per position) and whether it is a flag
VARIABLES = [
    ("EFFVM_VAR_WEIGHT", 0, 8, False),  # BtlvMcss_GetWeight
    ("EFFVM_VAR_ATTACKER_WEIGHT", 8, 1, False),
    ("EFFVM_VAR_TARGET", 9, 1, False),  # BtlvEffvmParam's fields, which SetVar sets
    ("EFFVM_VAR_VARIANT", 10, 1, False),
    ("EFFVM_VAR_PARAM_2", 11, 1, False),
    ("EFFVM_VAR_SHAKES", 12, 1, False),
    ("EFFVM_VAR_CAUGHT", 13, 1, False),
    ("EFFVM_VAR_CRITICAL", 14, 1, False),
    ("EFFVM_VAR_ITEM", 15, 1, False),
    ("EFFVM_VAR_VALUE", 16, 1, False),  # the script's value, which SetValue and LoadVar set
    ("EFFVM_VAR_ATTACKER", 17, 1, False),
    ("EFFVM_VAR_ATTACKER_VANISH", 18, 1, True),
    ("EFFVM_VAR_ATTACKER_SIDE", 19, 1, False),
    ("EFFVM_VAR_RARE", 20, 8, True),  # BTLV_MCSS_FLAG_RARE
    ("EFFVM_VAR_ATTACKER_RARE", 28, 1, True),
    ("EFFVM_VAR_NO_BOUNCE", 29, 8, True),  # BtlvMcss_IsNoBounce
    ("EFFVM_VAR_ATTACKER_NO_BOUNCE", 37, 1, True),
    ("EFFVM_VAR_IS_MULTI", 38, 1, True),
    ("EFFVM_VAR_BATTLE_STYLE", 39, 1, False),
    ("EFFVM_VAR_TRAINER_TYPE", 40, 4, False),  # BtlvEffect_GetTrainerType of each trainer
    ("EFFVM_VAR_MCSS_SCALED", 53, 1, True),
    ("EFFVM_VAR_CAMERA_SAVED", 54, 1, True),
    ("EFFVM_VAR_CAMERA_LOCKED", 56, 1, True),
    ("EFFVM_VAR_DEFENDER", 57, 1, False),
    ("EFFVM_VAR_N_POKEMON", 58, 8, True),  # BTLV_MCSS_FLAG_N_POKEMON
    ("EFFVM_VAR_ATTACKER_N_POKEMON", 66, 1, True),
    ("EFFVM_VAR_FAMOUS", 67, 8, True),  # BTLV_MCSS_FLAG_FAMOUS
    ("EFFVM_VAR_ATTACKER_FAMOUS", 75, 1, True),
]


def effect_first() -> int:
    """BTLV_EFFECT_FIRST, the first effect number of a/0/6/6, from btlv_effvm.c."""
    return int(re.search(r"^#define BTLV_EFFECT_FIRST (\w+)", SOURCE.read_text(), re.MULTILINE)[1], 0)


def command_names() -> list[str]:
    """The commands' names, from their handlers in btlv_effvm.c's MOVE_SCRCMD."""
    table = re.search(r"MOVE_SCRCMD\[\] = \{(.*?)\};", SOURCE.read_text(), re.DOTALL)[1]
    handlers = [h.strip() for h in table.split(",") if h.strip()]
    if len(handlers) != len(COMMANDS):
        sys.exit(f"MOVE_SCRCMD has {len(handlers)} commands, and COMMANDS {len(COMMANDS)}")
    return [HANDLER_PREFIX.sub("", h) for h in handlers]


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
    names = command_names()
    lines = [
        "// Macros for the battle effect VM's scripts, written by tools/script/btlv_effvm_script.py inc",
        "",
        *(f'#include "{header}"' for header in headers),
        "",
        "    // In .data, which LLVM pads with zeros: in .text, clang 18 pads `.balign 4, 0` with ARM NOPs",
        "    .data",
        "",
        "    .set FALSE, 0",
        "    .set TRUE, 1",
        "",
        "    // The first effect number of a/0/6/6 (btlv_effvm.c); those below it are moves",
        f"    .set BTLV_EFFECT_FIRST, {effect_first():#x}",
        "",
    ]
    comments = {
        "target": "// The targets besides the positions 0 to 13; ALL and the sides only for several "
                  "(BtlvEffvm_GetTargetPositions)",
        "emit": "// The emitters' positions besides 0 to 7 (BtlvEffvm_InitEmitter); MIRRORED mirrors the spin axis",
        "pan": "// The sides of PlaySE's pan, which takes a position too",
        "wait": "// What Wait waits for (BtlvEffvm_WaitNative)",
        "cmp": "// The comparisons of IfVarConst and IfVarVar (BtlvEffvm_JumpIf)",
    }
    done = set()
    for kind, comment in comments.items():
        lines.append(f"    {comment}")
        for value, name in sorted(CODES[kind if kind != "target" else "targets"].items()):
            if name not in done:
                lines.append(f"    .set {name}, {value}")
                done.add(name)
        lines.append("")
    lines.append("    // The variables (BtlvEffvm_GetVariable); those of each position take it added, from 0 to 7")
    lines += [f"    .set {name}, {first}" for name, first, _, _ in VARIABLES]
    lines += [
        "",
        "    .macro EffvmCommand id",
        "    .2byte \\id",
        "    .endm",
        "",
        "    // The start of a script file: the number of its variants, each followed by ScriptVariant",
        "    .macro ScriptHeader variants",
        "ScriptStart:",
        "    .4byte \\variants",
        "    .endm",
        "",
        f"    // A variant's code, whose offset is written {VARIANT_COPIES} times; the game reads the first",
        "    .macro ScriptVariant label",
        f"    .rept {VARIANT_COPIES}",
        "    .4byte \\label - ScriptStart",
        "    .endr",
        "    .endm",
        "",
    ]
    for cmd_id, (name, (params, doc)) in enumerate(zip(names, COMMANDS)):
        if doc:
            lines.append(f"    // {doc}")
        lines.append(f"    .macro {name}{' ' if params else ''}{', '.join(param for param, _ in params)}")
        lines.append(f"    EffvmCommand {cmd_id}")
        for param, kind in params:
            if kind == "jump":
                lines.append(f"    .4byte \\{param} - (. + 4)")
            else:
                lines.append(f"    .4byte \\{param}")
        lines.append("    .endm")
        lines.append("")
    path.write_text("\n".join(lines))


def s32(value: int) -> int:
    return value - (1 << 32) if value & 0x80000000 else value


class Script:
    def __init__(self, data: bytes, prefix: str, constants: dict[str, dict[int, str]], names: list[str], first: int):
        self.data = data
        self.prefix = prefix
        self.constants = constants
        self.names = names
        self.first = first
        self.variants = struct.unpack_from("<I", data, 0)[0]
        self.header_end = 4 + self.variants * 4 * VARIANT_COPIES
        self.starts = [struct.unpack_from("<I", data, 4 + v * 4 * VARIANT_COPIES)[0] for v in range(self.variants)]
        self.instructions: dict[int, tuple[int, list[tuple[str, int]], int]] = {}
        self.labels: set[int] = set(self.starts)

    def label(self, offset: int) -> str:
        if offset in self.starts:
            variant = self.starts.index(offset)
            return self.prefix if variant == 0 else f"{self.prefix}_Variant{variant}"
        return f"{self.prefix}_{offset:04X}"

    def decode(self, pc: int):
        if pc < self.header_end or pc + 2 > len(self.data):
            return None
        cmd_id = struct.unpack_from("<H", self.data, pc)[0]
        if cmd_id >= len(COMMANDS):
            return None
        kinds = [kind for _, kind in COMMANDS[cmd_id][0]]
        end = pc + 2 + 4 * len(kinds)
        if end > len(self.data):
            return None
        args = []
        for i, kind in enumerate(kinds):
            value = struct.unpack_from("<I", self.data, pc + 2 + 4 * i)[0]
            if kind == "jump":
                value = end + s32(value)
                # A target outside the code means these bytes aren't this command
                if not self.header_end <= value < len(self.data):
                    return None
            args.append((kind, value))
        return cmd_id, args, end

    def trace(self, starts: list[int], limit: tuple[int, int] | None = None) -> bool:
        """Follows the code from the starts. With a limit, it must stay inside it, on bytes no command holds yet, and
        nothing is kept unless it does."""
        instructions = dict(self.instructions)
        covered = self.covered()
        pending = list(starts)
        while pending:
            pc = pending.pop()
            while pc not in instructions:
                decoded = self.decode(pc)
                if decoded is None:
                    if limit:
                        return False
                    break
                cmd_id, args, end = decoded
                if limit and (not limit[0] <= pc < end <= limit[1] or any(o in covered for o in range(pc, end))):
                    return False
                instructions[pc] = decoded
                pending += [target for kind, target in args if kind == "jump"]
                if cmd_id in NO_FALLTHROUGH:
                    break
                pc = end
        if limit:
            # Jumps from the new code must land on the starts of commands
            for pc, (_, args, _) in instructions.items():
                if pc not in self.instructions and any(k == "jump" and t not in instructions for k, t in args):
                    return False
        self.instructions = instructions
        return True

    def covered(self) -> set[int]:
        covered = set(range(self.header_end))
        for pc, (_, _, end) in self.instructions.items():
            covered.update(range(pc, end))
        return covered

    def fill_gaps(self):
        """Decodes what no path reaches: commands where they trace cleanly from the gap's next undecoded byte, and
        raw bytes for the rest."""
        pos = self.header_end
        while pos < len(self.data):
            covered = self.covered()
            if pos in covered:
                pos += 1
                continue
            gap_end = pos
            while gap_end < len(self.data) and gap_end not in covered:
                gap_end += 1
            while pos < gap_end and self.trace([pos], (pos, gap_end)):
                covered = self.covered()
                while pos < gap_end and pos in covered:
                    pos += 1
            pos = gap_end

    def format_arg(self, kind: str, value: int, operand_kind: str | None) -> str:
        if kind == "jump":
            if value not in self.instructions:
                return f". + 4 + {value - self.current_field - 4}"
            return self.label(value)
        if kind in ("fx", "color"):
            signed = s32(value)
            return "0" if signed == 0 else f"-{-signed:#x}" if signed < 0 else f"{signed:#x}"
        if kind == "effect":
            if value >= self.first:
                return f"BTLV_EFFECT_FIRST + {value - self.first}"
        if kind == "var":
            for name, first, count, _ in VARIABLES:
                if first <= value < first + count:
                    return name if value == first else f"{name} + {value - first}"
        if kind == "operand":
            kind = operand_kind or "value"
        name = self.constants.get(kind, {}).get(value)
        if name is not None:
            return name
        signed = s32(value)
        return str(signed) if -0x10000 < signed < 0x10000 else f"{value:#x}"

    @staticmethod
    def variable_kind(value: int) -> str | None:
        for _, first, count, flag in VARIABLES:
            if first <= value < first + count:
                return "bool" if flag else None
        return None

    def disassemble(self) -> str:
        self.trace(self.starts)
        self.fill_gaps()
        for _, args, _ in self.instructions.values():
            self.labels.update(target for kind, target in args if kind == "jump" and target in self.instructions)
        # The files are padded with zeros to a multiple of 4 bytes
        covered = self.covered()
        size = max(covered) + 1
        if not (0 < len(self.data) - size < 4 and all(o in covered for o in range(size))
                and not any(self.data[size:])):
            size = len(self.data)
        out = [f"    ScriptHeader {self.variants}"]
        out += [f"    ScriptVariant {self.label(start)}" for start in self.starts]
        pc = self.header_end
        while pc < size:
            if pc in self.instructions:
                self.emit_label(out, pc)
                cmd_id, args, end = self.instructions[pc]
                # A value compared with or given to a variable is written as the variable's kind
                operand_kind = next((self.variable_kind(v) for k, v in args if k == "var"), None)
                parts = []
                for i, (kind, value) in enumerate(args):
                    self.current_field = pc + 2 + 4 * i
                    parts.append(self.format_arg(kind, value, operand_kind))
                text = ", ".join(parts)
                out.append(f"    {self.names[cmd_id]}{' ' if text else ''}{text}")
                pc = end
            else:
                self.emit_label(out, pc)
                out.append(f"    .byte {self.data[pc]:#04x}")
                pc += 1
        if size < len(self.data):
            out.append("    .balign 4")
        return "\n".join(out) + "\n"

    def emit_label(self, out: list[str], offset: int):
        if offset in self.labels:
            if out and out[-1]:
                out.append("")
            out.append(f"{self.label(offset)}:")


def camel(name: str) -> str:
    return "".join(part.capitalize() for part in name.split("_"))


def script_names(kind: str, count: int) -> list[str]:
    """The files' names, without .s: the moves' after generated/moves.txt (configure.py's ARCHIVE_LISTS) and the
    members past the last move by number, and the effects by number."""
    names = [constant.removeprefix("MOVE_").lower() for constant in ordered_names("moves")] if kind == "moves" else []
    return (names + [f"{i:03d}" for i in range(len(names), count)])[:count]


def disassemble(archive: Path, output: Path, kind: str):
    output.mkdir(parents=True, exist_ok=True)
    constants = load_constants()
    names = command_names()
    first = effect_first()
    files = read_narc(archive.read_bytes())
    move_names = {value: name for name, value in reversed(list(load("moves").items()))}
    for i, (data, name) in enumerate(zip(files, script_names(kind, len(files)))):
        if kind == "moves":
            prefix = camel(name) if not name.isdigit() else f"Move{name}"
            what = f"{move_names[i]}'s animation" if i in move_names else f"Move {i}, past the last move"
            header = f"// {what}: member {i} of a/0/6/5"
        else:
            prefix = f"Effect{name}"
            header = f"// Effect BTLV_EFFECT_FIRST + {i} ({first + i:#x}): member {i} of a/0/6/6"
        script = Script(data, prefix, constants, names, first)
        text = f'{header}\n\n#include "asm/btlv_effvm.inc"\n\n{script.disassemble()}'
        (output / f"{name}.s").write_text(text)


def sync(dry_run: bool = False):
    """Writes the macros again from MOVE_SCRCMD, and renames the commands whose handlers were renamed in the
    scripts of res/battle/animations; with dry_run, only lists the renames. sync_handler_names.py runs it too."""
    old = {}
    if INC.exists():
        for match in re.finditer(r"^    \.macro (\w+).*\n    EffvmCommand (\d+)$", INC.read_text(), re.MULTILINE):
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
    disasm.add_argument("archive", type=Path)
    disasm.add_argument("output", type=Path)
    disasm.add_argument("kind", choices=["moves", "effects"])
    sync_parser = commands.add_parser("sync")
    sync_parser.add_argument("--dry-run", action="store_true", help="list the renames and change nothing")
    args = parser.parse_args()

    if args.command == "inc":
        args.output.parent.mkdir(parents=True, exist_ok=True)
        write_inc(args.output)
    elif args.command == "disasm":
        disassemble(args.archive, args.output, args.kind)
    else:
        sync(args.dry_run)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Generates build.ninja for the Pokémon Black 2 decompilation.

Run this once after cloning (and again after adding source files), then run `ninja`.
"""
import argparse
import ast
import hashlib
import io
import json
import os
import platform
import shlex
import shutil
import stat
import subprocess
import sys
import tempfile
import tomllib
import urllib.request
import zipfile
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).parent.resolve()
sys.path.insert(0, str(ROOT))
from tools.build.deadstrip import load_manifest as load_deadstrip  # noqa: E402
from tools.data.gen_constants import ordered_files  # noqa: E402
from tools.data.graphics_archive import GRAPHICS, directory_archive  # noqa: E402
from tools.text.pms_words import CATEGORIES as PMS_WORD_CATEGORIES  # noqa: E402

VERSIONS = {
    "b2_us": {"sha1": "e51e6dfb8678a3d19dcd2a10691b96a569ca0abb", "rom": "pokeblack2_us.nds", "defines": ["BLACK2"]},
    "w2_us": {"sha1": "b5d7490be7b415b8f1e672a53e978a9cc667e56a", "rom": "pokewhite2_us.nds", "defines": ["WHITE2"]},
}

# Upstream 1.2.0 predates the guest x87 initialization in decompals/wibo e8f4795. The fork's main is upstream
# 420a75b, which has it, plus the imports that sjiswrap needs to load the compiler
WIBO_REPO = "https://github.com/fuddlesworth/wibo"
WIBO_VERSION = "pokebw2-sjis-1"
WIBO_SHA256 = {
    "wibo-x86_64": "d708570bd64cc8c176273a2bb7b5082f4fa849139d0ffe6cd0b96e26f3bc1ef2",
    "wibo-macos": "d11a80c3e5dfa85b8097d1968640e12158630f86e95ef29029dcbb22b57dad89",
}
# sjiswrap runs the compiler on its sources converted from UTF-8 to Shift JIS, the encoding of the game's own text, so
# that a string such as an assertion's Japanese expression is written as is. The fork's pokebw2 branch warns only for
# a character that Shift JIS can't encode outside a comment, such as the é of a "Pokémon" in code
SJISWRAP_REPO = "https://github.com/fuddlesworth/sjiswrap"
SJISWRAP_VERSION = "v1.2.2-pokebw2.1"
SJISWRAP_SHA256 = "4e9c917ba3f245da74a5a48ec91b459e956189cc1411233aa40046e0f7ac294f"
OBJDIFF_VERSION = "v3.8.1"
# Release binaries of wibo and objdiff-cli for macOS, by (sys.platform, machine); other platforms get the Linux x86-64
# ones. wibo's macOS build is x86-64 only, and runs under Rosetta 2 on Apple silicon.
WIBO_BINARIES = {
    ("darwin", "arm64"): "wibo-macos",
    ("darwin", "x86_64"): "wibo-macos",
}
OBJDIFF_BINARIES = {
    ("darwin", "arm64"): "objdiff-cli-macos-arm64",
    ("darwin", "x86_64"): "objdiff-cli-macos-x86_64",
}
# decomp.me name of the dsi/1.1p1 compiler (build 1024), for objdiff's scratch button
DECOMP_ME_COMPILER = "mwcc_40_1024"
MWCCARM_URL = "https://decomp.aetias.com/files/mwccarm.zip"
# dsd with DSi hybrid ROM support comes from a fork of ds-decomp (and of ds-rom, which its Cargo.toml pins) until the
# changes are upstreamed. Its dsi-hybrid branch is released as DSD_VERSION; after a new release, bump it and
# configure.py replaces tools/dsd.
DSD_REPO = "https://github.com/fuddlesworth/ds-decomp"
DSD_VERSION = "v0.12.1-dsi.6"
# Release binary of each platform, by (sys.platform, machine)
DSD_BINARIES = {
    ("linux", "x86_64"): "dsd-linux-x86_64",
    ("darwin", "arm64"): "dsd-macos-arm64",
    ("darwin", "x86_64"): "dsd-macos-x86_64",
}
# Compiler for decompiled code. The game code needs dsi/1.1p1 or later: after a store to a field, dsi/1.1 reuses the
# stored register where the game reloads the field. dsi/1.1p1 to dsi/1.3p1 generate identical code for everything
# tested so far, while dsi/1.6 does not match the game.
MWCC_VERSION = "dsi/1.1p1"
# The linker places objects and generates DS Protect's legacy interworking glue
MWLD_VERSION = "dsi/1.1"

# The compiler reads its sources as Shift JIS, from sjiswrap: in the default ASCII, a character whose second byte is
# 0x5c, such as 表, would end in a backslash that escapes a closing quote or continues a // comment
ENCODING_FLAG = "-enc SJIS"

CC_FLAGS = [
    "-O4,p",               # Optimize for speed
    "-proc arm946e",       # ARM9 processor
    "-thumb",              # Most game code is Thumb
    "-interworking",       # ARM/Thumb interworking
    "-enum int",           # Enums are int-sized
    "-char signed",        # char is signed
    "-fp soft",            # Software floating point
    "-lang=c99",
    "-Cpp_exceptions off", # No exception tables
    "-gccext,on",          # GCC extensions
    "-gccinc",             # #include "..." and <...> search the same paths
    "-inline on,noauto",   # Only inline functions marked inline
    "-ipa file",           # Interprocedural analysis within each file
    "-sym on",             # Debug info for objdiff
    "-requireprotos",      # Calling an undeclared function is an error, instead of an implicit declaration
    "-nolink",
    "-msgstyle gcc",
    ENCODING_FLAG,
]

# Libraries built apart from the game, each with its own compiler and flags: lib/<name>/ holds a library's public
# headers in include/, its sources in src/, and its compiler and flags in library.toml. A library without sources, such
# as one whose headers are all that is decompiled so far, needs no library.toml.
LIB_DIR = ROOT / "lib"


# The assembler flags of each library that has assembly sources, from library.toml's asm_flags: a library's hand-written
# assembly (MSL's runtime, fp_fastI.s) is assembled by the mwasmarm of its compiler's version
LIBRARY_ASM_FLAGS: dict[str, list[str]] = {}


def load_libraries() -> dict[str, tuple[str, list[str]]]:
    """Returns the compiler and flags of each library with sources, keyed by its source directory ("lib/spl/src/")."""
    libraries = {}
    for lib in sorted(p for p in LIB_DIR.iterdir() if p.is_dir()):
        settings = lib / "library.toml"
        if not settings.exists():
            if (lib / "src").exists():
                sys.exit(f"{lib.relative_to(ROOT)} has sources but no library.toml")
            continue
        with settings.open("rb") as f:
            library = tomllib.load(f)
        src = f"{(lib / 'src').relative_to(ROOT).as_posix()}/"
        libraries[src] = (library["compiler"], [*library["flags"], ENCODING_FLAG])
        if "asm_flags" in library:
            LIBRARY_ASM_FLAGS[src] = library["asm_flags"]
        for path, flags in library.get("file_flags", {}).items():
            FILE_FLAGS[src + path] = flags
        for path, compiler in library.get("file_compilers", {}).items():
            FILE_COMPILERS[src + path] = compiler
    return libraries


# Per-source overrides from library.toml and config/compiler_flags.toml: each flag
# replaces the default flag of the same option ("-ipa function" replaces "-ipa file", "-thumb" replaces "-nothumb",
# "-O2,p" replaces "-O4,p")
FILE_FLAGS: dict[str, list[str]] = {}
# Library sources built with another compiler than the rest of their library, from library.toml's file_compilers
FILE_COMPILERS: dict[str, str] = {}


def flag_option(flag: str) -> str:
    """Returns the option a compiler flag sets: its first word, with -thumb and -nothumb as one option, and every
    optimization level (-O4,p, -O2,p) as one."""
    option = flag.split()[0]
    if option.startswith("-O"):
        return "-O"
    return "-thumb" if option == "-nothumb" else option


def file_flags(source: Path, flags: list[str]) -> list[str]:
    """Returns source defaults with the file's overrides in place of the same options."""
    try:
        relative = (ROOT / source).resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return list(flags)
    overrides = FILE_FLAGS.get(relative, [])
    options = {flag_option(flag) for flag in overrides}
    return [flag for flag in flags if flag_option(flag) not in options] + overrides


def load_source_flags(path: Path) -> dict[str, list[str]]:
    """Loads game source overrides; library overrides stay in their library.toml files."""
    with path.open("rb") as stream:
        manifest = tomllib.load(stream)
    if set(manifest) - {"file_flags"}:
        raise ValueError(f"{path}: unknown compiler settings")
    overrides = manifest.get("file_flags", {})
    if not isinstance(overrides, dict):
        raise ValueError(f"{path}: file_flags must be a table")
    for source, flags in overrides.items():
        relative = Path(source)
        if (not relative.parts or relative.is_absolute() or relative.as_posix() != source or ".." in relative.parts
                or relative.parts[0] != "src" or relative.suffix != ".c"
                or not (ROOT / relative).is_file()
                or (ROOT / relative).resolve().relative_to(ROOT).as_posix() != source):
            raise ValueError(f"{path}: invalid game source path {source!r}")
        if (not isinstance(flags, list) or any(not isinstance(flag, str) or not flag.strip()
                or not flag.startswith("-") for flag in flags)):
            raise ValueError(f"{path}: {source}: flags must be nonempty option strings")
        for flag in flags:
            words = shlex.split(flag)
            if any(word.startswith("-") for word in words[1:]):
                raise ValueError(f"{path}: {source}: each flag must set one option")
        options = [flag_option(flag) for flag in flags]
        if len(set(options)) != len(options):
            raise ValueError(f"{path}: {source}: repeated compiler option")
    return overrides


LIBRARIES = load_libraries()
# The dead symbols of each source file whose original link dead-stripped some of its code, from config/deadstrip.toml:
# its object goes through tools/build/deadstrip.py before the link and objdiff take it
DEADSTRIP = load_deadstrip(ROOT / "config/deadstrip.toml", ROOT)
# Every compiler that a library source is built with, each with a rule of its own
LIBRARY_COMPILERS = list(dict.fromkeys([*(compiler for compiler, _ in LIBRARIES.values()), *FILE_COMPILERS.values()]))
FILE_FLAGS.update(load_source_flags(ROOT / "config/compiler_flags.toml"))
# Header search path of every source file, the game's and the libraries': the game's headers, each library's public
# headers, and the headers the build generates from the constant lists in generated/ (see
# tools/data/gen_constants.py). A library's private headers sit beside its sources.
GENERATED_INCLUDE_DIR = "build/include/generated"
INCLUDE_DIRS = ["include", *(p.relative_to(ROOT).as_posix() for p in sorted(LIB_DIR.glob("*/include"))),
                GENERATED_INCLUDE_DIR]
# Binaries that the game embeds as they are, which no compiler of ours can build: the DSP's programs. The build takes
# their bytes from the base ROM (tools/build/rom_blob.py) into an include file under rom/ of the generated headers, which
# the source file that defines the array includes as its initializer. Each maps that include file to the extracted
# module, the module's config directory and the array's symbol. Both versions have the same bytes, so they come from
# the first version built, and the other version's SHA1 check covers its own.
ROM_BLOBS = {
    "rom/dsp/dsp_g711_image.inc": ("dsi/ltd_autoload_0.bin", "ltd_autoload_0", "DSP_G711Image"),
    "rom/dsp/dsp_graphics_image.inc": ("dsi/ltd_autoload_0.bin", "ltd_autoload_0", "DSP_GraphicsImage"),
}


def library_of(source: Path) -> tuple[str, list[str]] | None:
    """Returns the compiler and flags of the library a source file belongs to, or None for the game's own code."""
    try:
        relative = (ROOT / source).resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return None
    library = next((library for prefix, library in LIBRARIES.items() if relative.startswith(prefix)), None)
    return (FILE_COMPILERS.get(relative, library[0]), file_flags(source, library[1])) if library else None


def source_assembler(source: Path) -> tuple[str, list[str]]:
    """Returns the compiler version whose mwasmarm assembles a library's assembly source, and its flags."""
    relative = (ROOT / source).resolve().relative_to(ROOT).as_posix()
    prefix = next((prefix for prefix in LIBRARIES if relative.startswith(prefix)), None)
    if prefix is None or prefix not in LIBRARY_ASM_FLAGS:
        sys.exit(f"{relative}: assembly sources are built only in a library whose library.toml has asm_flags")
    return FILE_COMPILERS.get(relative, LIBRARIES[prefix][0]), LIBRARY_ASM_FLAGS[prefix]


@dataclass(frozen=True)
class SourceCompiler:
    compiler: str
    flags: tuple[str, ...]
    is_library: bool


def source_compiler(source: Path) -> SourceCompiler:
    """Returns effective build settings without changing library membership for game files."""
    library = library_of(source)
    if library:
        return SourceCompiler(library[0], tuple(library[1]), True)
    return SourceCompiler(MWCC_VERSION, tuple(file_flags(source, CC_FLAGS)), False)


# Archives assembled from source, which replace their extracted counterparts in the ROM: the scripts. Each maps its path
# under files/ to the directory of its members, one assembly file each, in archive order.
ARCHIVES = {
    "a/0/5/6": "res/field/scripts",  # Field scripts, see tools/script/field_script.py
    "a/1/6/9": "res/trainers/ai",  # Trainer AI scripts, see tools/script/tr_ai_script.py
}

# The constant list that names and orders each assembled archive's files, and the prefix of its constants
ARCHIVE_LISTS = {"a/0/5/6": ("field_scripts", "SCRIPTS_"), "a/1/6/9": ("tr_ai_scripts", "TR_AI_SCRIPT_")}

# Archives packed from JSON (and CSV) data by a script's pack command: the script, its data directory, the archives it
# writes, in the order it takes them, and whether it packs each version apart, which passes it the version's name
# (black2 or white2); and any other data directories it reads. The data's constants come from the lists and headers,
# so those are inputs too.
DATA_PACKS = [
    # Species data, level-up moves, evolutions, baby species, experience tables and egg moves
    ("tools/data/species_data.py", "res/pokemon",
     ["a/0/1/6", "a/0/1/8", "a/0/1/9", "a/0/2/0", "a/0/1/7", "a/1/2/4"], False),
    ("tools/data/pokemon_graphics.py", "res/pokemon",
     ["a/0/0/4", "a/0/0/7", "a/0/5/1", "a/1/6/5", "a/2/2/9"], True),
    ("tools/data/pokemon_cries.py", "res/pokemon", ["a/1/9/1"], True),  # The cries' volumes
    # The sound archive: sequences, banks, wave archives, players, groups and streams, and the Pokémon's cries
    ("tools/data/sound_data.py", "res/sound", ["swan_sound_data.sdat"], False, ["res/pokemon"]),
    ("tools/data/move_data.py", "res/moves", ["a/0/2/1"], False),  # Move data
    ("tools/data/item_data.py", "res/items", ["a/0/2/4"], False),  # Item data
    ("tools/data/item_icon_data.py", "res/items", ["a/0/2/5"], True),  # Item icons
    # The field actors' models: their table and their sprites' and 3D models' graphics
    ("tools/data/field_actor_data.py", "res/field/actors", ["a/0/4/7", "a/0/4/8"], False),
    # Trainers, their parties, and the table of their messages with its offsets
    ("tools/data/trainer_data.py", "res/trainers/data", ["a/0/9/1", "a/0/9/2", "a/0/8/9", "a/0/9/0"], False),
    # Trainer sprites, front and back, and a/0/6/3, a copy of the back sprites
    ("tools/data/trainer_graphics.py", "res/trainers/sprites", ["a/0/7/1", "a/0/7/2", "a/0/6/3"], True),
    ("tools/data/encounter_data.py", "res/field/encounters", ["a/1/2/7"], True),  # Wild encounters
    # Zone headers, which give each zone the number of its events in res/field/events/order.json
    ("tools/data/zone_data.py", "res/field/zones", ["a/0/1/2"], False, ["res/field/events"]),
    ("tools/data/trade_data.py", "res/field/trades", ["a/1/6/3"], False),  # In-game trades
    ("tools/data/map_matrix_data.py", "res/field/matrices", ["a/0/0/9"], False),  # Map matrices
    # Map models, whose GLBs show the map texture sets' PNGs, which name the textures they bind
    ("tools/data/map_model_data.py", "res/field/maps/models", ["a/0/0/8"], True, ["res/field/maps/textures"]),
    # Field prop models, which their bundles' textures go with in the same way
    ("tools/data/field_prop_model_data.py", "res/field/props/exterior/models", ["a/2/2/5"], True,
     ["res/field/props/exterior/textures", "res/field/props/field_prop_model.schema.json"]),
    ("tools/data/field_prop_model_data.py", "res/field/props/interior/models", ["a/2/2/6"], True,
     ["res/field/props/interior/textures", "res/field/props/field_prop_model.schema.json"]),
    ("tools/data/map_texture_data.py", "res/field/maps/textures", ["a/0/1/4"], True),  # Map texture PNG sources
    ("tools/data/map_texture_data.py", "res/field/props/exterior/textures", ["a/1/7/4"], True),
    ("tools/data/map_texture_data.py", "res/field/props/interior/textures", ["a/1/7/5"], True),
    ("tools/data/area_data.py", "res/field/areas", ["a/0/1/3"], False),  # Areas, a file of records rather than an archive
    ("tools/data/light_data.py", "res/lighting", ["a/0/6/0", "a/0/6/1"], False),  # Field and battle lighting
    # The town map's places, whose texts are messages of town_map.txt
    ("tools/data/town_map_data.py", "res/town_map", ["a/0/8/5"], True, ["res/text/system/town_map.txt"]),
    # The battle facilities' trainers and Pokémon; the Battle Subway's are also the Trial House's
    ("tools/data/facility_data.py", "res/trainers/facilities/battle_subway", ["a/2/1/2", "a/2/1/1"], False,
     ["res/trainers/facilities"]),
    # The Black Tower, White Treehollow in White 2
    ("tools/data/facility_data.py", "res/trainers/facilities/black_tower", ["a/2/6/2", "a/2/6/1"], False,
     ["res/trainers/facilities"]),
    # The Pokémon World Tournament's three pools of trainers, each with the single sets' archive, and its rental sets
    ("tools/data/facility_data.py", "res/trainers/facilities/pwt_regular", ["a/2/4/9", "a/2/5/0", "a/2/4/8"], False,
     ["res/trainers/facilities"]),
    ("tools/data/facility_data.py", "res/trainers/facilities/pwt_leaders", ["a/2/5/2", "a/2/5/3", "a/2/5/1"], False,
     ["res/trainers/facilities"]),
    ("tools/data/facility_data.py", "res/trainers/facilities/pwt_masters", ["a/2/5/5", "a/2/5/6", "a/2/5/4"], False,
     ["res/trainers/facilities"]),
    ("tools/data/facility_data.py", "res/trainers/facilities/pwt_rental", ["a/2/5/7"], False, ["res/trainers/facilities"]),
    # The zones' events, in the order of res/field/events/order.json
    ("tools/data/event_data.py", "res/field/events", ["a/1/2/6"], False),
]
# The 2D graphics archives, each a directory of res/graphics/ named after its ARCID_* constant, which gives its file
DATA_PACKS += [("tools/data/graphics_archive.py", f"res/graphics/{directory.name}", [directory_archive(directory)], True)
               for directory in sorted(GRAPHICS.iterdir()) if directory.is_dir()]
# What every packer reads besides its data: the scripts, and the move tutors' tables, which name the species data's
# tutor bits (tools/data/species_data.py). A packer also depends on the modules of tools/ that it imports
# (script_imports).
DATA_PACK_TOOLS = ["tools/data/datajson.py", "tools/text/text_ids.py", "tools/data/gen_constants.py", "tools/data/narc.py",
                   "tools/text/text_sources.py", "tools/text/text_data.py", "tools/text/msgdata.py",
                   "src/ov036/scrcmd_shop.c", "tools/data/facility_data.py", "tools/data/make_constants.py"]
# The C files a packer reads its tables from, besides scrcmd_shop.c
DATA_PACK_SOURCES = {"tools/data/pokemon_graphics.py": ["src/system/poke_icon.c"],
                     "tools/data/pokemon_cries.py": ["src/system/poke_icon.c"]}


def script_imports(script: str) -> list[str]:
    """Returns the modules of tools/ that a script imports, and theirs: as tools.data.narc, or as narc from the
    script's own directory. Imports inside functions are left out, since those are the one-off commands'."""
    found: list[str] = []
    pending = [ROOT / script]
    while pending:
        path = pending.pop()
        nodes = list(ast.parse(path.read_text()).body)
        while nodes:
            node = nodes.pop()
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef, ast.ClassDef)):
                continue
            nodes.extend(ast.iter_child_nodes(node))
            names = ([alias.name for alias in node.names] if isinstance(node, ast.Import) else
                     [node.module] if isinstance(node, ast.ImportFrom) and node.module and not node.level else [])
            for name in names:
                for candidate in (ROOT / (name.replace(".", "/") + ".py"), path.parent / f"{name}.py"):
                    if candidate.is_file() and candidate.is_relative_to(ROOT / "tools"):
                        module = str(candidate.relative_to(ROOT))
                        if module not in found and candidate != ROOT / script:
                            found.append(module)
                            pending.append(candidate)
                        break
    return sorted(found)

# The data in JSON that the text takes messages from, with \from{...} lines (tools/text/text_sources.py)
TEXT_DATA_DIRS = ["res/pokemon", "res/moves", "res/items", "res/trainers/data", "res/abilities", "res/types",
                  "res/field/trades", "res/trainers/facilities", "res/trainers/classes", "res/natures", "res/field/places"]

# Text archives built from source, by tools/text/text_data.py: each maps its path under files/ to the directory of its
# message files, one text file each, in archive order
TEXT_ARCHIVES = {
    "a/0/0/2": "res/text/system",  # System messages
    "a/0/0/3": "res/text/script",  # Script messages
}

LD_FLAGS = [
    "-proc arm946e",
    "-nodead",             # Dead-stripping is on by default and would drop unreferenced delinked objects
    "-nostdlib",
    "-interworking",
    "-map closure,unused",
    "-msgstyle gcc",
    "-m _start",
]


class Writer:
    """Minimal ninja file writer."""

    def __init__(self):
        self.out = io.StringIO()

    def comment(self, text):
        self.out.write(f"# {text}\n")

    def variable(self, key, value):
        self.out.write(f"{key} = {value}\n")

    def rule(self, name, command, description=None, **kwargs):
        self.out.write(f"rule {name}\n  command = {command}\n")
        if description:
            self.out.write(f"  description = {description}\n")
        for key, value in kwargs.items():
            self.out.write(f"  {key} = {value}\n")
        self.out.write("\n")

    def build(self, outputs, rule, inputs=(), implicit=(), variables=None, implicit_outputs=(), order_only=()):
        def esc(paths):
            return " ".join(str(p).replace("$", "$$").replace(" ", "$ ").replace(":", "$:") for p in paths)

        line = f"build {esc(outputs)}"
        if implicit_outputs:
            line += f" | {esc(implicit_outputs)}"
        line += f": {rule} {esc(inputs)}"
        if implicit:
            line += f" | {esc(implicit)}"
        if order_only:
            line += f" || {esc(order_only)}"
        self.out.write(line + "\n")
        for key, value in (variables or {}).items():
            self.out.write(f"  {key} = {value}\n")
        self.out.write("\n")

    def default(self, targets):
        self.out.write(f"default {' '.join(str(t) for t in targets)}\n")


def download_pinned(name: str, url: str, expected_hash: str, path: Path):
    """Downloads a pinned release binary to path unless it is already there with the expected SHA256, replacing an older
    cached one."""
    if path.exists() and hashlib.sha256(path.read_bytes()).hexdigest() == expected_hash:
        return
    print(f"Downloading {name}")
    with urllib.request.urlopen(url) as response:
        data = response.read()
    if hashlib.sha256(data).hexdigest() != expected_hash:
        raise ValueError(f"Downloaded {name} has an unexpected SHA256: {url}")
    with tempfile.NamedTemporaryFile(dir=path.parent, prefix=f"{path.name}-", delete=False) as temporary:
        temporary.write(data)
        temporary_path = Path(temporary.name)
    try:
        temporary_path.chmod(temporary_path.stat().st_mode | stat.S_IEXEC)
        os.replace(temporary_path, path)
    finally:
        temporary_path.unlink(missing_ok=True)


def mwcc_command(mwcc: Path, tools_dir: Path = ROOT / "tools") -> list[str]:
    """Returns the command that runs a Metrowerks compiler under wibo, through sjiswrap."""
    return [str(tools_dir / "wibo"), str(tools_dir / "sjiswrap.exe"), str(mwcc)]


def download_tools(tools_dir: Path):
    host = (sys.platform, platform.machine().lower())
    binary = WIBO_BINARIES.get(host, "wibo-x86_64")
    download_pinned(f"wibo {WIBO_VERSION}", f"{WIBO_REPO}/releases/download/{WIBO_VERSION}/{binary}",
                    WIBO_SHA256[binary], tools_dir / "wibo")
    download_pinned(f"sjiswrap {SJISWRAP_VERSION}",
                    f"{SJISWRAP_REPO}/releases/download/{SJISWRAP_VERSION}/sjiswrap-windows-x86.exe",
                    SJISWRAP_SHA256, tools_dir / "sjiswrap.exe")

    objdiff = tools_dir / "objdiff-cli"
    if not objdiff.exists():
        print(f"Downloading objdiff-cli {OBJDIFF_VERSION}")
        binary = OBJDIFF_BINARIES.get(host, "objdiff-cli-linux-x86_64")
        url = f"https://github.com/encounter/objdiff/releases/download/{OBJDIFF_VERSION}/{binary}"
        urllib.request.urlretrieve(url, objdiff)
        objdiff.chmod(objdiff.stat().st_mode | stat.S_IEXEC)

    mwccarm = tools_dir / "mwccarm"
    compilers = ["dsi", *LIBRARY_COMPILERS]
    if not all((mwccarm / compiler).exists() for compiler in compilers):
        print("Downloading mwccarm")
        with urllib.request.urlopen(MWCCARM_URL) as response:
            archive = zipfile.ZipFile(io.BytesIO(response.read()))
        versions = tuple(f"mwccarm/{compiler}/" for compiler in compilers)
        members = [m for m in archive.namelist() if m.startswith(versions)]
        archive.extractall(tools_dir, members)


def get_dsd(tools_dir: Path, from_source: bool) -> Path:
    """Downloads DSD_VERSION of dsd, or builds it from that tag with cargo, unless tools/dsd is already that version. A
    tools/dsd without tools/dsd.rev was built by hand, from a working copy of the fork, and is kept."""
    dsd = tools_dir / "dsd"
    stamp = tools_dir / "dsd.rev"
    if dsd.exists() and (not stamp.exists() or stamp.read_text().strip() == DSD_VERSION):
        return dsd

    binary = DSD_BINARIES.get((sys.platform, platform.machine().lower()))
    if binary and not from_source:
        print(f"Downloading dsd {DSD_VERSION}")
        urllib.request.urlretrieve(f"{DSD_REPO}/releases/download/{DSD_VERSION}/{binary}", dsd)
        dsd.chmod(dsd.stat().st_mode | stat.S_IEXEC)
    else:
        if not shutil.which("cargo"):
            sys.exit("building dsd from source needs cargo, see README.md")
        repo = tools_dir / "src" / "ds-decomp"
        if repo.exists():
            shutil.rmtree(repo)
        print(f"Building dsd {DSD_VERSION} from source")
        subprocess.run(["git", "-c", "advice.detachedHead=false", "clone", "--quiet", "--depth", "1",
                        "--branch", DSD_VERSION, DSD_REPO, str(repo)], check=True)
        subprocess.run(["cargo", "build", "--release", "--locked"], cwd=repo, check=True)
        shutil.copy2(repo / "target" / "release" / "dsd", dsd)
    stamp.write_text(DSD_VERSION + "\n")
    return dsd


def add_version(n: Writer, version: str, dsd: Path, bugfix: bool, shift: int) -> tuple[list[Path], list[str]]:
    """Adds the build steps of one version. Returns its check targets and dsd config files. A build with the bugs fixed
    or with its code shifted does not match, so its only targets are the ROM and its archives."""
    matching = not bugfix and not shift
    baserom = Path("orig") / f"baserom_{version}.nds"
    extract_dir = Path("extract") / version
    config_dir = Path("config") / version
    build_dir = Path("build") / version
    arm9_config = config_dir / "arm9" / "config.yaml"
    rom = Path("build") / VERSIONS[version]["rom"]
    sha1_file = Path(f"{version}.sha1")
    sha1_file.write_text(f"{VERSIONS[version]['sha1']}  {rom}\n")

    dsd_configs = sorted(str(p) for p in config_dir.rglob("*.txt")) + [str(arm9_config)]

    # The delink targets are known ahead of time from the configs
    result = subprocess.run(
        [str(dsd), "json", "delinks", "--config-path", str(arm9_config)],
        capture_output=True,
        text=True,
        cwd=ROOT,
    )
    if result.returncode != 0:
        sys.exit(f"dsd json delinks failed for {version}:\n{result.stderr}")
    delinks = json.loads(result.stdout)
    files = delinks["files"]
    lcf_file = delinks["arm9_lcf_file"]
    objects_file = delinks["arm9_objects_file"]

    n.comment(version)
    stamp_dir = build_dir / "stamps"
    baserom_ok = stamp_dir / "baserom.ok"
    n.build([baserom_ok], "check_baserom", [baserom], variables={"sha1": VERSIONS[version]["sha1"]})
    n.build([extract_dir / "config.yaml"], "extract", [baserom], implicit=[baserom_ok],
            variables={"extract_dir": str(extract_dir)})

    delink_outputs = sorted({f["delink_file"] for f in files})
    n.build(delink_outputs, "delink", dsd_configs, implicit=[extract_dir / "config.yaml"],
            variables={"config": str(arm9_config)})
    # The linker script also defines NitroSDK's linker symbols, as the SDK's template does, from linker_symbols.toml
    n.build([lcf_file, objects_file], "lcf", dsd_configs,
            implicit=[extract_dir / "config.yaml", "config/linker_symbols.toml"], variables={"config": str(arm9_config)})
    link_lcf = lcf_file
    if shift:
        # Padding in the linker script moves the code and data after it, see tools/build/shift_lcf.py
        shifted_lcf = build_dir / "arm9_shifted.lcf"
        n.build([shifted_lcf], "shift_lcf", [link_lcf], implicit=["tools/build/shift_lcf.py"],
                variables={"amount": hex(shift)})
        link_lcf = shifted_lcf

    # Source files are listed in delinks.txt by their path. Complete files are linked from the compiled object, and
    # incomplete ones are still compiled so objdiff can compare them. A file in config/deadstrip.toml is compiled to an
    # object under unstripped/, which the strip turns into the one at the usual path.
    version_defines = VERSIONS[version]["defines"] + (["BUGFIX"] if bugfix else [])
    defines = " ".join(f"-d {define}" for define in version_defines)
    as_defines = " ".join(f"-D{define}" for define in version_defines)
    objects = []
    compiled = []
    deadstripped = set()
    for f in files:
        source = Path(f["name"])
        if source.suffix in (".c", ".cpp") and source.exists():
            obj = build_dir / source.with_suffix(".o")
            settings = source_compiler(source)
            rule = f"mwcc_{LIBRARY_COMPILERS.index(settings.compiler)}" if settings.is_library else "mwcc"
            stripped = source.as_posix() in DEADSTRIP
            compiler_obj = build_dir / "unstripped" / source.with_suffix(".o") if stripped else obj
            variables = {"defines": defines, "dep": compiler_obj.with_suffix(".d")}
            variables["flags"] = " ".join(settings.flags)
            n.build([compiler_obj], rule, [source], variables=variables, order_only=["constants_headers"])
            if stripped:
                n.build([obj], "deadstrip", [compiler_obj],
                        implicit=["tools/build/deadstrip.py", "config/deadstrip.toml"],
                        variables={"source": source.as_posix()})
                deadstripped.add(source.as_posix())
            compiled.append(obj)
        elif source.suffix == ".s" and source.exists():
            obj = build_dir / source.with_suffix(".o")
            compiler, asm_flags = source_assembler(source)
            n.build([obj], f"mwasm_{LIBRARY_COMPILERS.index(compiler)}", [source],
                    variables={"flags": " ".join(asm_flags)})
            compiled.append(obj)
        objects.append(f["object_to_link"])
    # An entry for a file the version doesn't compile would strip nothing, unnoticed
    unbuilt = sorted(set(DEADSTRIP) - deadstripped)
    if unbuilt:
        sys.exit(f"config/deadstrip.toml lists {', '.join(unbuilt)}, which {version} doesn't compile")
    # Nothing in the link may refer to what the strip removed, since the linker would send such a call to address 0
    # rather than fail (tools/build/check_deadstrip.py). It compares nothing with the ROM, so it runs in a build that
    # doesn't match too
    deadstrip_checks = []
    if DEADSTRIP:
        deadstrip_ok = stamp_dir / "deadstrip.ok"
        n.build([deadstrip_ok], "check_deadstrip", [*compiled, *delink_outputs],
                implicit=[*(build_dir / "unstripped" / Path(source).with_suffix(".o") for source in sorted(DEADSTRIP)),
                          "tools/build/check_deadstrip.py", "tools/build/deadstrip.py", "config/deadstrip.toml",
                          *sorted(str(p) for p in config_dir.rglob("symbols.txt"))],
                variables={"config": str(config_dir), "unstripped": str(build_dir / "unstripped")})
        deadstrip_checks.append(deadstrip_ok)

    # Preserve the LTD autoload grouping in both the private prelink and final link.
    grouped_objects = build_dir / "objects_link.txt"
    n.build([grouped_objects], "ltd_objects", [objects_file, lcf_file],
            implicit=["tools/build/ltd_objects.py"])

    # Keep DS Protect's legacy interworking glue separate from its compiler objects, one derived object per copy of
    # the flashcart test, each step reading the previous step's object list.
    # Objdiff still uses the compiler output; only the final link uses the derived ones.
    link_objects = objects
    link_response = grouped_objects
    veneer_implicit = []
    for source, veneer_dir, section, count in (
            ("lib/dsprot/src/dsprot_detect.o", build_dir / "interworking", "OV165", 8),
            ("lib/dsprot/src/dsprot_detect_multi.o", build_dir / "interworking" / "ov337", "OV337", 16)):
        dsprot_path = build_dir / source
        dsprot_object = next((obj for obj in objects if Path(obj) == dsprot_path), None)
        if dsprot_object is None:
            continue
        veneer_object = veneer_dir / dsprot_path.name
        veneer_response = veneer_dir / "objects.txt"
        veneer_inputs = {"objects": link_response, "lcf": link_lcf,
                         "source_object": dsprot_object, "veneer_object": veneer_object,
                         "response": veneer_response, "scratch": veneer_dir / "prelink",
                         "section": section, "count": count}
        n.build([veneer_object, veneer_response], "dsprot_veneers", objects,
                implicit=[link_lcf, link_response, *veneer_implicit, "tools/build/restore_dsprot_veneers.py",
                          ROOT / "tools/mwccarm" / MWLD_VERSION / "mwldarm.exe"],
                variables={key: shlex.quote(str(value)).replace("$", "$$")
                           for key, value in veneer_inputs.items()})
        veneer_implicit = [veneer_object, veneer_response]
        link_response = veneer_response
        link_objects = [str(veneer_object) if obj == dsprot_object else obj for obj in link_objects]

    arm9_o = build_dir / "arm9.o"
    n.build([arm9_o], "mwld", link_objects, implicit=[link_lcf, link_response],
            variables={"objects": link_response, "lcf": link_lcf})

    # The ROM's file system is the extracted one, with the archives built from source in place of the extracted ones.
    # The tree is made first, so that no archive is written through a link into extract/.
    files_dir = build_dir / "files"
    files_ok = stamp_dir / "files.ok"
    n.build([files_ok], "files_tree", [], implicit=[extract_dir / "config.yaml", "tools/build/files_tree.py"],
            variables={"source": str(extract_dir / "files"), "output": str(files_dir),
                       "built": " ".join([*ARCHIVES, *TEXT_ARCHIVES,
                                          *(a for _, _, pack, *_ in DATA_PACKS for a in pack)])})
    archives = []
    checks = []
    for path, source_dir in ARCHIVES.items():
        members = []
        for source in ordered_files(*ARCHIVE_LISTS[path], ROOT / source_dir, ".s"):
            source = source.relative_to(ROOT)
            obj = build_dir / source.with_suffix(".o")
            n.build([obj], "as", [source], variables={"dep": obj.with_suffix(".d"), "defines": as_defines},
                    order_only=["constants_headers"])
            member = obj.with_suffix(".bin")
            n.build([member], "objcopy_bin", [obj])
            members.append(member)
        archive = files_dir / path
        n.build([archive], "narc", members, implicit=["tools/data/narc.py"], order_only=[files_ok])
        archive_ok = stamp_dir / "files" / f"{path.replace('/', '_')}.ok"
        n.build([archive_ok], "check_file", [archive], implicit=[extract_dir / "config.yaml"],
                variables={"original": str(extract_dir / "files" / path)})
        archives.append(archive)
        checks.append(archive)
        if matching:
            checks.append(archive_ok)

    constant_sources = sorted(str(p.relative_to(ROOT)) for p in [*(ROOT / "generated").glob("*.txt"),
                                                                  *(ROOT / "include" / "constants").glob("*.h")])
    for script, data_dir, paths, per_version, *other_dirs in DATA_PACKS:
        outputs = [files_dir / path for path in paths]
        sources = sorted(str(p.relative_to(ROOT)) for d in [data_dir, *(other_dirs[0] if other_dirs else [])]
                         for p in ([ROOT / d] if (ROOT / d).is_file() else (ROOT / d).rglob("*")) if p.is_file())
        game = VERSIONS[version]["defines"][0].lower() if per_version else ""
        dependencies = sorted({script, *DATA_PACK_TOOLS, *script_imports(script), *DATA_PACK_SOURCES.get(script, []),
                               *constant_sources})
        n.build(outputs, "data_pack", sources, implicit=dependencies,
                order_only=[files_ok], variables={"script": script, "dir": data_dir, "game": game})
        for path, archive in zip(paths, outputs):
            archive_ok = stamp_dir / "files" / f"{path.replace('/', '_')}.ok"
            n.build([archive_ok], "check_file", [archive], implicit=[extract_dir / "config.yaml"],
                    variables={"original": str(extract_dir / "files" / path)})
            archives.append(archive)
            checks.append(archive)
            if matching:
                checks.append(archive_ok)

    # The text takes some of its messages from the data in JSON, with \from{...} lines (tools/text/text_sources.py)
    text_data_sources = sorted(str(p.relative_to(ROOT)) for d in TEXT_DATA_DIRS for p in (ROOT / d).rglob("*.json"))
    for path, source_dir in TEXT_ARCHIVES.items():
        archive = files_dir / path
        n.build([archive], "text_pack", sorted(Path(source_dir).glob("*.txt")),
                implicit=[*DATA_PACK_TOOLS, *text_data_sources, *constant_sources],
                order_only=[files_ok], variables={"dir": source_dir})
        archive_ok = stamp_dir / "files" / f"{path.replace('/', '_')}.ok"
        n.build([archive_ok], "check_file", [archive], implicit=[extract_dir / "config.yaml"],
                variables={"original": str(extract_dir / "files" / path)})
        archives.append(archive)
        checks.append(archive)
        if matching:
            checks.append(archive_ok)

    rom_config = build_dir / "build" / "rom_config.yaml"
    n.build([rom_config], "rom_config", [arm9_o], variables={"config": str(arm9_config)})
    n.build([rom], "rom_build", [rom_config], implicit=[files_ok, *archives])

    modules_ok = stamp_dir / "modules.ok"
    n.build([modules_ok], "check_modules", [rom_config], variables={"config": str(arm9_config)})
    rom_ok = stamp_dir / "rom.ok"
    n.build([rom_ok], "sha1", [sha1_file], implicit=[rom], variables={"rom": str(rom)})
    n.build([version], "phony", [modules_ok, rom_ok] if matching else [rom])

    # Context files for decomp.me scratches, made by objdiff
    for obj in compiled:
        source = obj.relative_to(build_dir).with_suffix(".c")
        if not source.exists():
            continue
        n.build([obj.with_suffix(f".ctx{source.suffix}")], "ctx", [source], variables={"defines": defines},
                order_only=["constants_headers"])

    # Progress report, compares every delinked object with its compiled counterpart
    # objdiff's report, then complete files counted as matched, which the ROM checks prove (tools/build/complete_report.py)
    objdiff_report = build_dir / "report.objdiff.json"
    n.build([objdiff_report], "report", [], implicit=["objdiff.json", *compiled, *delink_outputs])
    report = build_dir / "report.json"
    n.build([report], "complete_report", [objdiff_report], implicit=["tools/build/complete_report.py"])
    n.build([f"{version}_progress"], "progress", [report])
    # The checks also compile every source file, so that an incomplete file, whose object only feeds the report and
    # is never linked, can't stop compiling without the default build noticing.
    if not matching:
        return [rom, *checks, *deadstrip_checks, *compiled], dsd_configs
    return [modules_ok, rom_ok, *checks, *deadstrip_checks, *compiled], dsd_configs


def write_compile_flags():
    """Writes compile_flags.txt, with which clangd checks the code as 32-bit ARM, with the build's include path."""
    flags = ["--target=armv5te-none-eabi", "-ffreestanding", *(f"-I{d}" for d in INCLUDE_DIRS), "-std=c99",
             "-DBLACK2"]
    (ROOT / "compile_flags.txt").write_text("\n".join(flags) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("versions", nargs="*", choices=[[], *VERSIONS],
                        help="versions to build, defaults to every version with a base ROM in orig/")
    parser.add_argument("--dsd", type=Path, default=None,
                        help="path to the dsd executable, defaults to tools/dsd, downloaded as DSD_VERSION")
    parser.add_argument("--dsd-from-source", action="store_true",
                        help="build DSD_VERSION of dsd with cargo instead of downloading it")
    parser.add_argument("--wine", default=None, help="run the Metrowerks tools with this instead of wibo")
    parser.add_argument("--no-download", action="store_true", help="do not download or build missing tools")
    parser.add_argument("--shift", type=lambda s: int(s, 0), default=0, metavar="BYTES",
                        help="pad the code so that it and everything after it moves, to test that a mod can change "
                             "code sizes; the ROMs no longer match. A multiple of 32, such as 0x100")
    parser.add_argument("--bugfix", action="store_true",
                        help="fix the game's bugs that are marked with BUGFIX in the source; the ROMs no longer match")
    args = parser.parse_args()

    versions = args.versions or [v for v in VERSIONS if (ROOT / "orig" / f"baserom_{v}.nds").exists()]
    if not versions:
        sys.exit("no base ROMs found in orig/, see README.md")

    tools_dir = ROOT / "tools"
    if not args.no_download:
        download_tools(tools_dir)
    if args.dsd:
        dsd = args.dsd.resolve()
    elif args.no_download:
        dsd = tools_dir / "dsd"
    else:
        dsd = get_dsd(tools_dir, args.dsd_from_source)
    if not dsd.exists():
        sys.exit(f"dsd not found at {dsd}, see README.md")
    wine = args.wine or str(tools_dir / "wibo")
    clang = shutil.which("clang")
    llvm_objcopy = shutil.which("llvm-objcopy")
    if not clang or not llvm_objcopy:
        sys.exit("clang and llvm-objcopy not found, see README.md")
    mwcc = tools_dir / "mwccarm" / MWCC_VERSION / "mwccarm.exe"
    mwld = tools_dir / "mwccarm" / MWLD_VERSION / "mwldarm.exe"

    n = Writer()
    n.comment(f"Generated by configure.py for {', '.join(versions)}, do not edit")
    n.variable("dsd", shlex.quote(str(dsd)))
    n.variable("wine", shlex.quote(wine))
    n.variable("sjiswrap", shlex.quote(str(tools_dir / "sjiswrap.exe")))
    n.variable("python", shlex.quote(sys.executable))
    n.variable("includes", " ".join(f"-i {d}" for d in INCLUDE_DIRS))
    n.variable("as_includes", " ".join(f"-I {d}" for d in INCLUDE_DIRS))
    n.out.write("\n")

    n.rule("check_baserom", "echo '$sha1  $in' | sha1sum --quiet -c - && touch $out", "Checking base ROM $in")
    n.rule("extract", "$dsd rom extract --rom $in --output-path $extract_dir", "Extracting $in")
    n.rule("delink", "$dsd delink --config-path $config", "Delinking $config")
    n.rule("lcf", "$dsd lcf --config-path $config --symbols config/linker_symbols.toml",
           "Generating linker script for $config")
    # mwccarm writes the dependency file next to the object, with Windows paths that fix_depfile.py converts
    n.rule("mwcc", f"mkdir -p $$(dirname $out) && $wine $sjiswrap {shlex.quote(str(mwcc))} $flags "
           "$defines -gccdep -MD $includes -o $out $in && $python tools/build/fix_depfile.py $dep", "Compiling $in",
           depfile="$dep", deps="gcc")
    # The older compilers (SPL's 1.2/base) warn when MWCIncludes, their system include path, isn't set. The code finds its
    # headers through -i, so any value does
    for i, compiler in enumerate(LIBRARY_COMPILERS):
        lib_mwcc = tools_dir / "mwccarm" / compiler / "mwccarm.exe"
        n.rule(f"mwcc_{i}", f"mkdir -p $$(dirname $out) && MWCIncludes=. $wine $sjiswrap {shlex.quote(str(lib_mwcc))} "
               "$flags $defines -gccdep -MD $includes -o $out $in && $python tools/build/fix_depfile.py $dep",
               "Compiling $in", depfile="$dep", deps="gcc")
    # A library's assembly sources, with the mwasmarm of its compiler's version
    for i, compiler in enumerate(LIBRARY_COMPILERS):
        lib_mwasm = tools_dir / "mwccarm" / compiler / "mwasmarm.exe"
        n.rule(f"mwasm_{i}", f"mkdir -p $$(dirname $out) && $wine {shlex.quote(str(lib_mwasm))} $flags -o $out $in",
               "Assembling $in")
    # Drops the code the original link dead-stripped from a listed source's object (config/deadstrip.toml)
    n.rule("deadstrip", "$python tools/build/deadstrip.py --source $source $in $out", "Dead-stripping $out")
    n.rule("check_deadstrip", "$python tools/build/check_deadstrip.py --config $config --unstripped $unstripped $out "
           "@$out.rsp", "Checking references to dead-stripped code in $config", rspfile="$out.rsp",
           rspfile_content="$in_newline")
    n.rule("shift_lcf", "$python tools/build/shift_lcf.py $in $out $amount", "Shifting $in")
    n.rule("ltd_objects", "$python tools/build/ltd_objects.py $in $out", "Grouping the LTD autoload's objects in $out")
    n.rule("mwld", f"$wine {shlex.quote(str(mwld))} {' '.join(LD_FLAGS)} @$objects $lcf -o $out", "Linking $out")
    # Pass the runner as one executable path and the normal flags as argv data.
    # The helper changes -proc only for its private prelink.
    veneer_linker = shlex.quote(str(mwld)).replace("$", "$$")
    veneer_flags = shlex.quote(json.dumps(shlex.split(" ".join(LD_FLAGS)))).replace("$", "$$")
    n.rule("dsprot_veneers", "$python tools/build/restore_dsprot_veneers.py "
           "--objects $objects --lcf $lcf --source-object $source_object "
           "--output-object $veneer_object --output-response $response --scratch $scratch "
           f"--runner $wine --linker {veneer_linker} --link-flags {veneer_flags} "
           "--section $section --expected-count $count",
           "Generating DS Protect interworking veneers for $veneer_object")
    # Scripts go through the C preprocessor, so that they can include the constant headers
    n.rule("as", f"{shlex.quote(clang)} --target=armv5te-none-eabi -x assembler-with-cpp -c $as_includes $defines "
           "-MD -MF $dep -o $out $in", "Assembling $in", depfile="$dep", deps="gcc")
    n.rule("objcopy_bin", f"{shlex.quote(llvm_objcopy)} -O binary $in $out", "Converting $in")
    n.rule("narc", "$python tools/data/narc.py pack $out $in", "Packing $out")
    n.rule("text_pack", "$python tools/text/text_data.py pack $dir $out", "Packing $out")
    n.rule("data_pack", "$python $script pack $dir $game $out", "Packing $dir")
    n.rule("check_file", "cmp $in $original && mkdir -p $$(dirname $out) && touch $out", "Checking $in")
    n.rule("files_tree", "$python tools/build/files_tree.py $source $output $built --stamp $out",
           "Linking the files of $output")
    # dsd points the ROM at the extracted files, and the build's own file system replaces them. BSD sed, as on macOS,
    # needs a backup suffix after -i, which GNU sed also takes
    n.rule("rom_config", "$dsd rom config --elf $in --config $config && sed -i.bak "
           "'s|^files_dir: .*|files_dir: ../files|' $out && rm $out.bak", "Configuring ROM for $config")
    n.rule("rom_build", "$dsd rom build --config $in --rom $out", "Building $out")
    n.rule("check_modules", "$dsd check modules --config-path $config --fail && touch $out", "Checking modules")
    n.rule("sha1", "sha1sum --quiet -c $in && touch $out", "Checking $rom")
    # The context stays UTF-8, for decomp.me
    n.rule("ctx", f"$wine {shlex.quote(str(mwcc))} -EP -lang=c99 -gccinc $defines $includes $in "
           "| grep -v -e '^#line' -e 'prepdump' > $out", "Preprocessing $in")
    # decomp.me compiles the UTF-8 it is given without sjiswrap, so its scratches take the compiler's default encoding
    decomp_me_flags = " ".join(flag for flag in CC_FLAGS if flag != ENCODING_FLAG)
    n.rule("objdiff_config", f"$python tools/build/objdiff_config.py $version --dsd $dsd "
           f"--compiler {DECOMP_ME_COMPILER} --c-flags '{decomp_me_flags}' -o $out", "Writing $out")
    n.rule("report", f"{tools_dir / 'objdiff-cli'} report generate -p . -o $out", "Generating $out")
    n.rule("complete_report", "$python tools/build/complete_report.py $in $out", "Counting complete files in $out")
    n.rule("progress", "$python tools/build/progress.py $in", "Progress")
    # A regeneration keeps the options that chose the tools, so that a build with another dsd keeps it
    configure_args = [*args.versions, *(["--bugfix"] if args.bugfix else []),
                      *([f"--shift {args.shift:#x}"] if args.shift else []),
                      *([f"--dsd {shlex.quote(str(args.dsd.resolve()))}"] if args.dsd else []),
                      *(["--dsd-from-source"] if args.dsd_from_source else []),
                      *([f"--wine {shlex.quote(args.wine)}"] if args.wine else []),
                      *(["--no-download"] if args.no_download else [])]
    n.rule("configure", f"$python configure.py {' '.join(configure_args)}", "Reconfiguring", generator="1")
    # Formats the sources and headers in place with clang-format and .clang-format
    n.rule("format", "clang-format -i $in", "Formatting")

    # The constant headers are generated from the committed lists in generated/, so that the C code, the data
    # files and the scripts share one source of truth, and a mod only edits the list. Compiles depend on them
    # order-only; the depfiles rebuild what includes a changed header. A header that comes out the same is not
    # rewritten, and restat keeps its users from recompiling.
    n.rule("gen_constants", "$python tools/data/gen_constants.py $in $out", "Generating $out", restat="1")
    constant_headers = []
    for source in sorted((ROOT / "generated").glob("*.txt")):
        header = Path(GENERATED_INCLUDE_DIR) / "constants" / source.with_suffix(".h").name
        n.build([header], "gen_constants", [source.relative_to(ROOT)], implicit=["tools/data/gen_constants.py"])
        constant_headers.append(header)
    # Each message file's header of its message IDs (tools/text/text_ids.py). A file with \from lines counts the
    # messages of the data it takes, so it depends on that data
    n.rule("text_ids", "$python tools/text/text_ids.py $in $out", "Generating $out", restat="1")
    text_tools = ["tools/text/text_ids.py", *DATA_PACK_TOOLS]
    from_data = sorted(str(p.relative_to(ROOT)) for d in TEXT_DATA_DIRS for p in (ROOT / d).rglob("*.json"))
    from_data += sorted(str(p.relative_to(ROOT)) for p in (ROOT / "generated").glob("*.txt"))
    for source_dir in TEXT_ARCHIVES.values():
        for source in sorted((ROOT / source_dir).glob("*.txt")):
            header = Path(GENERATED_INCLUDE_DIR) / "text" / source.parent.name / source.with_suffix(".h").name
            uses_data = "\\from{" in source.read_text(encoding="utf-8")
            n.build([header], "text_ids", [source.relative_to(ROOT)],
                    implicit=[*text_tools, *(from_data if uses_data else [])])
            constant_headers.append(header)
    # The easy chat words, numbered across their categories' message files (tools/text/pms_words.py)
    n.rule("pms_words", "$python tools/text/pms_words.py $out", "Generating $out", restat="1")
    pms_words_header = Path(GENERATED_INCLUDE_DIR) / "constants" / "pms_words.h"
    n.build([pms_words_header], "pms_words", [f"res/text/system/{file}.txt" for _, file, _ in PMS_WORD_CATEGORIES],
            implicit=["tools/text/pms_words.py", *text_tools, *from_data])
    constant_headers.append(pms_words_header)
    # The embedded binaries, from the first version's extracted ROM
    n.rule("rom_blob", "$python tools/build/rom_blob.py $module $in $name $out", "Generating $out", restat="1")
    for include, (module, config_dir, name) in ROM_BLOBS.items():
        output = Path(GENERATED_INCLUDE_DIR) / include
        symbols = Path("config") / versions[0] / "arm9" / config_dir / "symbols.txt"
        n.build([output], "rom_blob", [symbols],
                implicit=["tools/build/rom_blob.py", Path("extract") / versions[0] / "config.yaml"],
                variables={"module": str(Path("extract") / versions[0] / module), "name": name})
        constant_headers.append(output)
    n.build(["constants_headers"], "phony", constant_headers)

    checks, configs = [], []
    for version in versions:
        version_checks, version_configs = add_version(n, version, dsd, args.bugfix, args.shift)
        checks += version_checks
        configs += version_configs

    # objdiff.json covers one version, the primary one if it is being built
    objdiff_version = versions[0]
    n.build(["objdiff.json"], "objdiff_config", configs, implicit=["tools/build/objdiff_config.py", "configure.py", "config/compiler_flags.toml"],
            variables={"version": objdiff_version})
    n.build(["report"], "phony", [Path("build") / objdiff_version / "report.json"])
    n.build(["progress"], "phony", [f"{objdiff_version}_progress"])

    # The data directories too: adding, removing or renaming a file changes its directory's time, so the build lists
    # the data files again
    source_dirs = [d for _, d, *_ in DATA_PACKS] + list(ARCHIVES.values())
    source_dirs += list(TEXT_ARCHIVES.values()) + ["generated"]
    data_dirs = sorted({str(p.relative_to(ROOT)) for d in source_dirs for p in [ROOT / d, *(ROOT / d).rglob("*")]
                        if p.is_dir()})
    n.build(["build.ninja"], "configure", ["configure.py"],
            implicit=[*configs, "config/compiler_flags.toml", "config/deadstrip.toml",
                      *(str(p.relative_to(ROOT)) for p in sorted(LIB_DIR.glob("*/library.toml"))), *data_dirs])
    sources = sorted(str(p.relative_to(ROOT)) for pattern in ("src/**/*.c", "lib/*/src/**/*.[ch]", "include/**/*.h",
                                                               "lib/*/include/**/*.h") for p in ROOT.glob(pattern))
    n.build(["format"], "format", sources)
    n.build(["check"], "phony", checks)
    n.default(["check", "objdiff.json"])

    (ROOT / "build.ninja").write_text(n.out.getvalue())
    write_compile_flags()
    fixes = ", with the bugs fixed" if args.bugfix else ""
    shifted = f", with the code shifted by {args.shift:#x} bytes" if args.shift else ""
    print(f"Wrote build.ninja for {', '.join(versions)}{fixes}{shifted}, now run ninja")


if __name__ == "__main__":
    main()

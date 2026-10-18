"""Compiles host C (port/hal files, decomp files, a test's wrapper) into a shared library for the oracle tests, with
the compiler, flags, include paths and defines of the host build, and loads it with ctypes.

    lib = build_library("mi_memory", ["port/hal/mi/mi_memory.c"])
    copy = lib.function("MI_CpuCopy8", None, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint32)

The flags come from a configured host build made with the Ninja generator: the one ctest runs the tests from
(ASM_ORACLE_BUILD_DIR), else build/port (cmake -S port -B build/port -G Ninja), so they follow port/CMakeLists.txt
without a copy here: a decomp file gets the flags its own build statement has, any other
file gets the game's (pokebw2_game's most common set: the language model, include paths, -D PLATFORM_PC and the
version's define), and a file under port/ or the wrapper also gets port_hal's include paths and options after them.
The library goes to asm-oracle/<name>_<version>/ in that build directory.

A symbol that the files reference and neither define nor find in the host's C library gets a stub, so that a file can
be tested without everything it calls: a function stub reports its name and aborts the process, and a data symbol is
a zeroed placeholder. Give the files that define what the tested functions really call.

On macOS the library has the build's slices (the architectures and macOS of its -arch and -mmacosx-version-min
flags, which the stubs and the link take too), and the tests check the one their Python runs: run_all.py skips a
build with no slice for it, such as an x86_64 build on Apple Silicon, whose Python is arm64 (ADR 0015).
"""

from __future__ import annotations

import collections
import concurrent.futures
import ctypes
import functools
import hashlib
import os
import re
import shlex
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

from harness import REPO, SYMBOL_LINE, default_version

HOST_BUILD = Path(os.environ.get("ASM_ORACLE_BUILD_DIR", REPO / "build" / "port")).resolve()
OUTPUT = HOST_BUILD / "asm-oracle"
VERSION_DEFINES = {"b2_us": "BLACK2", "w2_us": "WHITE2"}
GAME_TARGET = "pokebw2_game"
HAL_TARGET = "port_hal"
MACHO = sys.platform == "darwin"
WINDOWS = sys.platform == "win32"
LIBRARY_SUFFIX = ".dylib" if MACHO else ".dll" if WINDOWS else ".so"
# What port/include/port_host.h, which every file includes, declares: the host memory behind the DS's addresses
BASE_SOURCES = ("port/hal/hw/hw.c",)
# Placeholders for data symbols get room beyond the DS size, as port/tools/gen_stubs.py gives them
PLACEHOLDER_SCALE = 2
PLACEHOLDER_MAX = 0x10000


class BuildError(Exception):
    """The host build isn't configured, or a file didn't compile or link. The message has the compiler's output."""


@dataclass(frozen=True)
class CompileSettings:
    defines: tuple[str, ...]
    includes: tuple[str, ...]
    flags: tuple[str, ...]

    def arguments(self) -> list[str]:
        return [*self.defines, *self.includes, *self.flags]


def _unescape(text: str) -> str:
    return re.sub(r"\$([ :$\n])", r"\1", text)


def _target_flags(flags: tuple[str, ...]) -> list[str]:
    """What a macOS build is built for, from its flags: each -arch (CMAKE_OSX_ARCHITECTURES), -isysroot and
    -mmacosx-version-min (CMAKE_OSX_DEPLOYMENT_TARGET), so that the stubs and the library get the objects' slices."""
    kept: list[str] = []
    for index, flag in enumerate(flags):
        if flag in ("-arch", "-isysroot") and index + 1 < len(flags):
            kept += [flag, flags[index + 1]]
        elif flag.startswith("-mmacosx-version-min="):
            kept.append(flag)
    return kept


class HostBuild:
    """The compile settings of the configured host build, read from build/port/build.ninja."""

    def __init__(self, build_dir: Path = HOST_BUILD):
        ninja = build_dir / "build.ninja"
        if not ninja.exists():
            raise BuildError(f"{ninja} is missing: the host build must be configured, with the Ninja generator "
                             f"(cmake -S port -B build/port -G Ninja)")
        self.compiler = self._read_compiler(build_dir)
        self.by_source: dict[str, CompileSettings] = {}
        self.target_default: dict[str, CompileSettings] = {}
        # CMake names the rule C_COMPILER__<target>_unscanned_<build type>, and the build type is empty in a directory
        # configured without CMAKE_BUILD_TYPE (as port/tools/sync_upstream.sh configures build/port)
        statement = re.compile(r"^build (.+?): C_COMPILER__(\w+?)_unscanned_\w* (\S+)")
        per_target: dict[str, list[CompileSettings]] = collections.defaultdict(list)
        current: tuple[str, str] | None = None
        variables: dict[str, str] = {}

        def finish():
            if current is None:
                return
            target, source = current
            settings = CompileSettings(tuple(shlex.split(variables.get("DEFINES", ""))),
                                       tuple(shlex.split(variables.get("INCLUDES", ""))),
                                       tuple(shlex.split(variables.get("FLAGS", ""))))
            per_target[target].append(settings)
            if target == GAME_TARGET:
                self.by_source[str(Path(source).resolve())] = settings

        for line in ninja.read_text().splitlines():
            if line.startswith("build "):
                finish()
                current, variables = None, {}
                match = statement.match(line)
                if match:
                    current = (match.group(2), _unescape(match.group(3)))
            elif current and line.startswith("  ") and " = " in line:
                key, _, value = line.strip().partition(" = ")
                variables[key] = _unescape(value)
        finish()
        if GAME_TARGET not in per_target:
            raise BuildError(f"no {GAME_TARGET} compile statements in {ninja}")
        for target, settings in per_target.items():
            self.target_default[target] = collections.Counter(settings).most_common(1)[0][0]
        self.target_flags = _target_flags(self.target_default[GAME_TARGET].flags)

    @staticmethod
    def _read_compiler(build_dir: Path) -> str:
        override = os.environ.get("ASM_ORACLE_CC")
        if override:
            return override
        cache = build_dir / "CMakeCache.txt"
        match = re.search(r"^CMAKE_C_COMPILER:\w+=(.+)$", cache.read_text(), re.MULTILINE) if cache.exists() else None
        return match.group(1).strip() if match else "clang"

    def settings(self, source: Path, version: str) -> list[str]:
        """The compiler arguments for a file, for a version."""
        source = source.resolve()
        own = self.by_source.get(str(source))
        settings = own or self.target_default[GAME_TARGET]
        arguments = settings.arguments()
        in_port = REPO / "port" in source.parents or OUTPUT in source.parents
        if in_port and HAL_TARGET in self.target_default:
            hal = self.target_default[HAL_TARGET]
            arguments += [*hal.includes, *hal.defines, *hal.flags]
        version_define = VERSION_DEFINES[version]
        others = {f"-D{d}" for d in VERSION_DEFINES.values() if d != version_define}
        arguments = [a for a in arguments if a not in others]
        if f"-D{version_define}" not in arguments:
            arguments.append(f"-D{version_define}")
        return arguments


def _ds_symbols(version: str) -> dict[str, tuple[str, int]]:
    """The kind (function or data) and DS size of every symbol of the version's ARM9 modules."""
    symbols: dict[str, tuple[str, int]] = {}
    for path in sorted((REPO / "config" / version / "arm9").rglob("symbols.txt")):
        for line in path.read_text().splitlines():
            match = SYMBOL_LINE.match(line)
            if match:
                name, kind, params, _ = match.groups()
                size = next((int(p[5:], 0) for p in (params or "").split(",") if p.startswith("size=")), 4)
                symbols.setdefault(name, ("function" if kind == "function" else "data", size))
    return symbols


def _nm(objects: list[Path]) -> tuple[set[str], set[str]]:
    """The global symbols the objects define and the ones they reference without defining."""
    tool = "llvm-nm" if WINDOWS else "nm"
    output = subprocess.run([tool, "-g", "-P", "-A", *map(str, objects)], capture_output=True, text=True,
                            check=True).stdout
    defined, undefined = set(), set()
    for line in output.splitlines():
        _, _, rest = line.rpartition(": ")
        fields = rest.split()
        if len(fields) < 2:
            continue
        name, kind = fields[0], fields[1]
        if MACHO and name.startswith("_"):
            name = name[1:]
        (undefined if kind == "U" else defined).add(name)
    return defined, undefined - defined


@functools.cache
def _host_libraries() -> list[ctypes.CDLL]:
    return [ctypes.CDLL(None)] if not WINDOWS else [ctypes.CDLL("ucrtbase"), ctypes.CDLL("kernel32")]


def _host_provides(name: str) -> bool:
    for library in _host_libraries():
        try:
            getattr(library, name)
            return True
        except AttributeError:
            pass
    return False


def _stub_source(unresolved: list[str], version: str) -> str:
    ds = _ds_symbols(version)
    lines = [
        "// Generated by port/tests/asm_oracle/build.py: stubs for what the tested files reference and don't define.",
        "#include <stdio.h>",
        "#include <stdlib.h>",
        "",
        "static void asm_oracle_unresolved(const char *name) {",
        '    fprintf(stderr, "asm_oracle: %s was called, but no file given to build_library() defines it\\n", name);',
        "    abort();",
        "}",
        "",
    ]
    for name in unresolved:
        kind, size = ds.get(name, ("function", 4))
        if kind == "function":
            lines.append(f'void {name}(void) {{ asm_oracle_unresolved("{name}"); }}')
        else:
            lines.append(f"unsigned char {name}[{min(max(size, 4) * PLACEHOLDER_SCALE, PLACEHOLDER_MAX):#x}];")
    return "\n".join(lines) + "\n"


class HostLibrary:
    """A built shared library, loaded with ctypes."""

    def __init__(self, path: Path, stubbed: list[str]):
        self.path = path
        self.stubbed = stubbed
        self.cdll = ctypes.CDLL(str(path))

    def function(self, name: str, restype, *argtypes):
        """A function of the library with its ctypes result and argument types set (restype None for void)."""
        function = getattr(self.cdll, name)
        function.restype = restype
        function.argtypes = list(argtypes)
        return function

    def address(self, name: str) -> int:
        """The address of a function or data symbol of the library."""
        return ctypes.cast(getattr(self.cdll, name), ctypes.c_void_p).value


def build_library(name: str, sources: list[str | Path], *, wrapper: str | None = None, version: str | None = None,
                  extra_flags: list[str] | tuple[str, ...] = (), base: bool = True,
                  host_build: HostBuild | None = None) -> HostLibrary:
    """Compiles the sources (paths relative to the repository, or absolute) and the wrapper's C text, if any, into
    asm-oracle/<name>_<version>/lib<name> in the host build and loads it. With base, the files of BASE_SOURCES that
    exist are added: the host memory that every file reaches through port_host.h. Warnings are printed; errors raise
    BuildError."""
    if not re.fullmatch(r"[A-Za-z0-9_]+", name):
        raise ValueError(f"library name {name!r} must be an identifier")
    version = version or default_version()
    host = host_build or HostBuild()
    out = OUTPUT / f"{name}_{version}"
    (out / "obj").mkdir(parents=True, exist_ok=True)
    paths = [Path(s) if Path(s).is_absolute() else REPO / s for s in sources]
    if base:
        paths += [REPO / s for s in BASE_SOURCES if (REPO / s).exists() and REPO / s not in paths]
    if wrapper is not None:
        wrapper_path = out / "wrapper.c"
        wrapper_path.write_text(wrapper)
        paths.append(wrapper_path)
    for path in paths:
        if not path.exists():
            raise BuildError(f"{path} doesn't exist")

    def compile_one(path: Path) -> Path:
        digest = hashlib.sha1(str(path).encode()).hexdigest()[:8]
        obj = out / "obj" / f"{path.stem}_{digest}.o"
        command = [host.compiler, *host.settings(path, version), *extra_flags, *([] if WINDOWS else ["-fPIC"]),
                   "-c", str(path), "-o", str(obj)]
        result = subprocess.run(command, capture_output=True, text=True, cwd=REPO)
        if result.returncode != 0:
            raise BuildError(f"compiling {path} failed:\n{shlex.join(command)}\n{result.stderr}")
        if result.stderr.strip():
            print(result.stderr, end="", file=sys.stderr)
        return obj

    with concurrent.futures.ThreadPoolExecutor() as pool:
        objects = list(pool.map(compile_one, paths))

    defined, undefined = _nm(objects)
    unresolved = sorted(n for n in undefined if not _host_provides(n))
    stub_path = out / "stubs.c"
    stub_path.write_text(_stub_source(unresolved, version))
    stub_obj = out / "obj" / "stubs.o"
    command = [host.compiler, "-std=c11", "-w", *host.target_flags, *([] if WINDOWS else ["-fPIC"]), "-c",
               str(stub_path), "-o", str(stub_obj)]
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode != 0:
        raise BuildError(f"compiling the stubs failed:\n{result.stderr}")

    library = out / f"lib{name}{LIBRARY_SUFFIX}"
    link = [host.compiler, "-shared", *host.target_flags, "-o", str(library), *map(str, objects), str(stub_obj)]
    if WINDOWS:
        # A DLL exports nothing by default: export every symbol the files define
        exports = out / "exports.def"
        exports.write_text("EXPORTS\n" + "".join(f"    {n}\n" for n in sorted(defined)))
        link.append(f"-Wl,/DEF:{exports}")
    result = subprocess.run(link, capture_output=True, text=True)
    if result.returncode != 0:
        raise BuildError(f"linking {library.name} failed:\n{shlex.join(link)}\n{result.stderr}")
    return HostLibrary(library, unresolved)

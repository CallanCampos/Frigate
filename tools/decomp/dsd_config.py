"""Helpers for reading dsd configs and the extracted modules they describe."""
import bisect
import contextlib
import fcntl
import re
from dataclasses import dataclass, field
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[2]


@contextlib.contextmanager
def config_lock():
    """Holds the checkout's config lock: the scripts that edit config/ read and rewrite whole files, so two of them
    running at once, from parallel agents, would lose one's changes."""
    (ROOT / "build").mkdir(exist_ok=True)
    with open(ROOT / "build" / ".config.lock", "w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        yield

# Symbol arguments can contain one level of nested parentheses, e.g. `function(arm,size=0x40,dsprot=(0x38,0x1))`
SYMBOL_RE = re.compile(r"^(\S+) kind:(\w+)(?:\(((?:[^()]|\([^()]*\))*)\))? addr:(0x[0-9a-f]+)(.*)$")
RELOC_RE = re.compile(r"^from:(0x[0-9a-f]+) kind:(\S+) to:(0x[0-9a-f]+)(?: add:(\S+))? module:(\S+)")


@dataclass
class Symbol:
    name: str
    kind: str
    addr: int
    args: str
    rest: str
    line: str

    @property
    def size(self) -> int | None:
        match = re.search(r"size=(0x[0-9a-f]+)", self.args or "")
        return int(match.group(1), 16) if match else None

    @property
    def thumb(self) -> bool:
        return (self.args or "").startswith("thumb")


@dataclass
class Module:
    name: str
    """Module path relative to the version's arm9 config directory, e.g. `.` or `overlays/ov004`."""
    config_dir: Path
    binary: Path
    base: int
    symbols: list[Symbol] = field(default_factory=list)
    reloc_sources: list[int] = field(default_factory=list)
    """Sorted addresses of relocated words."""
    relocs: dict[int, tuple[str, int, str]] = field(default_factory=dict)
    """Relocations by source address: kind, target address and target module."""

    def data(self) -> bytes:
        return self.binary.read_bytes()

    def functions(self) -> list[Symbol]:
        return sorted((s for s in self.symbols if s.kind == "function"), key=lambda s: s.addr)


def parse_symbols(path: Path) -> list[Symbol]:
    symbols = []
    for line in path.read_text().splitlines():
        match = SYMBOL_RE.match(line)
        if match:
            name, kind, args, addr, rest = match.groups()
            symbols.append(Symbol(name, kind, int(addr, 16), args, rest, line))
    return symbols


def parse_relocs(path: Path) -> dict[int, tuple[str, int, str]]:
    relocs = {}
    for line in path.read_text().splitlines():
        match = RELOC_RE.match(line)
        if match:
            source, kind, target, _addend, module = match.groups()
            relocs[int(source, 16)] = (kind, int(target, 16), module)
    return relocs


def reloc_module_names(module: str) -> list[str]:
    """Returns the module paths a relocation's `module:` value refers to, as used by `load_modules`."""
    if module == "main":
        return ["."]
    if module in ("itcm", "dtcm"):
        return [module]
    match = re.match(r"^overlays?\(([\d,\s]+)\)$", module)
    if match:
        return [f"overlays/ov{int(i):03d}" for i in match.group(1).split(",")]
    match = re.match(r"^(ltd_)?autoload\((\d+)\)$", module)
    if match:
        return [f"{match.group(1) or ''}autoload_{match.group(2)}"]
    return []


def parse_sections(path: Path) -> dict[str, tuple[int, int]]:
    """Returns the module sections from a delinks.txt file, as (start, end) by name."""
    sections = {}
    for line in path.read_text().splitlines():
        if not line.strip():
            break
        match = re.match(r"^\s+(\S+)\s+start:(0x[0-9a-f]+) end:(0x[0-9a-f]+)", line)
        if match:
            sections[match.group(1)] = (int(match.group(2), 16), int(match.group(3), 16))
    return sections


def load_modules(version: str, with_symbols: bool = True) -> dict[str, Module]:
    """Returns every module of a version, keyed by its path relative to the arm9 config directory."""
    extract = ROOT / "extract" / version
    config = ROOT / "config" / version / "arm9"
    arm9 = yaml.safe_load((extract / "arm9" / "arm9.yaml").read_text())
    modules = {".": Module(".", config, extract / "arm9" / "arm9.bin", arm9["base_address"])}
    for name in ("itcm", "dtcm"):
        info = yaml.safe_load((extract / "arm9" / f"{name}.yaml").read_text())
        modules[name] = Module(name, config / name, extract / "arm9" / f"{name}.bin", info["base_address"])
    for index in (2, 3):
        yaml_path = extract / "arm9" / f"unk_autoload_{index}.yaml"
        if yaml_path.exists():
            info = yaml.safe_load(yaml_path.read_text())
            name = f"autoload_{index}"
            modules[name] = Module(name, config / name, extract / "arm9" / f"unk_autoload_{index}.bin",
                                   info["base_address"])
    for ltd_yaml in sorted((extract / "dsi").glob("ltd_autoload_*.yaml")):
        info = yaml.safe_load(ltd_yaml.read_text())
        name = ltd_yaml.stem
        modules[name] = Module(name, config / name, ltd_yaml.with_suffix(".bin"), info["base_address"])
    overlays = yaml.safe_load((extract / "arm9_overlays" / "overlays.yaml").read_text())["overlays"]
    for overlay in overlays:
        name = f"overlays/ov{overlay['id']:03d}"
        modules[name] = Module(name, config / name, extract / "arm9_overlays" / overlay["file_name"],
                               overlay["base_address"])
    if with_symbols:
        for module in modules.values():
            if (module.config_dir / "symbols.txt").exists():
                module.symbols = parse_symbols(module.config_dir / "symbols.txt")
                module.relocs = parse_relocs(module.config_dir / "relocs.txt")
                module.reloc_sources = sorted(module.relocs)
    return modules


def masked_function_bytes(module: Module, data: bytes, function: Symbol) -> bytes:
    """Returns a function's bytes with every relocated word zeroed."""
    start = function.addr - module.base
    code = bytearray(data[start : start + (function.size or 0)])
    first = bisect.bisect_left(module.reloc_sources, function.addr)
    last = bisect.bisect_left(module.reloc_sources, function.addr + len(code))
    for source in module.reloc_sources[first:last]:
        offset = source - function.addr
        code[offset : offset + 4] = bytes(min(4, len(code) - offset))
    return bytes(code)

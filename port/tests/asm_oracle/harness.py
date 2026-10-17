"""Runs the ROM's original ARM9 functions in the Unicorn CPU emulator, as the oracle that host C is compared against.

Unicorn is a test tool only (PLAN.md §1): it runs the original machine code to produce the expected results, and
nothing in the port's build or program uses it. See README.md.

    emu = Emulator()  # the version in ASM_ORACLE_VERSION, else b2_us
    table = emu.alloc(1024)
    emu.call("MATHi_CRC32InitTableRev", table, 0xEDB88320)
    data = emu.alloc(5, data=b"hello")
    crc = emu.call("MATH_CalcCRC32", table, data, 5).r0
    emu.read(table, 1024)

The ARM9's main module is loaded at its load address, with the ITCM, the DTCM, the two small unknown autoloads and the
DSi's LTD autoload at theirs, so a ROM function may call any other function of main. Overlays are loaded on request
(Emulator.load_overlay). Nothing of the boot runs: .data has the ROM's initial values, .bss is zero, and no static
initializer or SDK init has run. The I/O registers, palette, VRAM and OAM are plain memory: the hardware (divider,
square root, DMA, the geometry engine) isn't modelled, but writes to them can be recorded (Emulator.record_writes).

Arguments are passed as MWCC passes them, the ARM Thumb Procedure Call Standard's way: 32-bit words in r0 to r3 and
then on the stack, a 64-bit argument as the next two words (low word first) with no alignment to an even register,
split between r3 and the stack when it falls there. FX_Mul32x64cFunc(fx32, fx64) takes its fx64 in r1 and r2.
"""

from __future__ import annotations

import bisect
import contextlib
import os
import re
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterator

REPO = Path(__file__).resolve().parents[3]
VERSIONS = ("b2_us", "w2_us")

# The emulator's memory map. The DS regions are where the ROM's code expects them; the harness's own regions
# (scratch, stack, callbacks, code stubs, the return sentinel) are in space the DS leaves unused.
ITCM = (0x01FF8000, 0x8000)
MAIN_RAM = (0x02000000, 0x01000000)  # 16 MB: the DSi's main memory, which holds the DTCM, LTD and autoload 2 too
IO = (0x04000000, 0x01000000)
PALETTE = (0x05000000, 0x1000)
VRAM = (0x06000000, 0x00900000)  # BG, OBJ and LCDC views; autoload 3 is at 0x06898000
OAM = (0x07000000, 0x1000)
SCRATCH = (0x08000000, 0x01000000)
STACK = (0x0B000000, 0x00100000)
CALLBACKS = (0x0C000000, 0x00010000)
CODE_STUBS = (0x0C010000, 0x00010000)
SENTINEL = (0x0D000000, 0x1000)
RETURN_ADDRESS = SENTINEL[0]
HARNESS_REGIONS = {"scratch memory": SCRATCH, "stack": STACK, "callbacks": CALLBACKS, "code stubs": CODE_STUBS,
                   "return sentinel": SENTINEL}

STACK_TOP = STACK[0] + STACK[1] - 0x100
CALLBACK_SLOT = 8
DEFAULT_MAX_INSTRUCTIONS = 20_000_000

ARM_BX_LR = 0xE12FFF1E
CPSR_SYSTEM_IRQS_OFF = 0xDF  # System mode, IRQ and FIQ masked, ARM state

SYMBOL_LINE = re.compile(r"^(\S+) kind:(\w+)(?:\(([^)]*)\))? addr:0x([0-9a-fA-F]+)")


def default_version() -> str:
    """The version the tests run for: ASM_ORACLE_VERSION (run_all.py --version sets it), else b2_us."""
    return os.environ.get("ASM_ORACLE_VERSION", "b2_us")


class EmulatorError(Exception):
    """The ROM's code did something the harness can't run or judge: a fault, an exception or a runaway loop."""


class Fault(EmulatorError):
    """An access to unmapped memory, an undefined instruction, an SVC (BIOS call) or a breakpoint."""


class UnalignedAccess(EmulatorError):
    """A halfword or word access at an address that isn't a multiple of its size. On the ARM946E-S an unaligned
    word load reads the aligned word rotated, a word store or LDM/STM ignores the address's low bits, and a halfword
    access is unpredictable; Unicorn performs them as a later ARM would, so the harness stops instead of giving a
    wrong answer. Either the original doesn't accept such an input, or it relies on what ARMv5 does, which a test
    must then check by other means."""


class InstructionLimit(EmulatorError):
    """The function didn't return within the instruction limit."""


@dataclass(frozen=True)
class Symbol:
    name: str
    address: int
    kind: str  # function, data, bss or label
    thumb: bool
    size: int | None
    module: str

    @property
    def entry(self) -> int:
        """The address to call: with bit 0 set for a Thumb function."""
        return self.address | 1 if self.thumb else self.address


@dataclass(frozen=True)
class I64:
    """A 64-bit argument (s64, u64, fx64): passed as two words, low word first."""

    value: int


@dataclass(frozen=True)
class CallResult:
    r0: int
    r1: int

    @property
    def s32(self) -> int:
        return self.r0 - (1 << 32) if self.r0 & 0x80000000 else self.r0

    @property
    def u64(self) -> int:
        return self.r0 | self.r1 << 32

    @property
    def s64(self) -> int:
        value = self.u64
        return value - (1 << 64) if value >> 63 else value


@dataclass(frozen=True)
class Access:
    """One recorded memory access: the address, its size in bytes, the value, and the instruction's address."""

    address: int
    size: int
    value: int
    pc: int


@dataclass(frozen=True)
class Module:
    name: str
    base: int
    data: bytes
    bss_size: int


def _read_flat_yaml(path: Path) -> dict[str, object]:
    """The top-level scalar keys of one of dsd's YAML files; nested values are left out."""
    values: dict[str, object] = {}
    for line in path.read_text().splitlines():
        if not line or line[0] in " #-" or ":" not in line:
            continue
        key, _, value = line.partition(":")
        value = value.strip()
        if value:
            values[key.strip()] = int(value, 0) if re.fullmatch(r"-?(0x[0-9a-fA-F]+|\d+)", value) else value
    return values


def _read_overlay_table(path: Path) -> dict[int, dict[str, object]]:
    """The overlays of dsd's overlays.yaml, by ID, each with its scalar keys."""
    overlays: dict[int, dict[str, object]] = {}
    current: dict[str, object] | None = None
    for line in path.read_text().splitlines():
        stripped = line.strip()
        if stripped.startswith("- "):
            current = {}
            stripped = stripped[2:]
        if current is None or ":" not in stripped:
            continue
        key, _, value = stripped.partition(":")
        value = value.strip()
        if value:
            current[key.strip()] = int(value, 0) if re.fullmatch(r"-?(0x[0-9a-fA-F]+|\d+)", value) else value
        if key.strip() == "id":
            overlays[int(value, 0)] = current
    return overlays


def missing_reason(version: str | None = None) -> str | None:
    """Why the oracle can't run for a version (the ROM isn't extracted, or the config is missing), or None."""
    version = version or default_version()
    extract = REPO / "extract" / version
    if not (extract / "config.yaml").exists():
        return (f"extract/{version} is missing: the ROM build extracts it (python3 configure.py && ninja, with the "
                f"ROM in orig/, as README.md says)")
    if not (REPO / "config" / version / "arm9" / "symbols.txt").exists():
        return f"config/{version}/arm9/symbols.txt is missing"
    return None


class Rom:
    """A version's ARM9 binaries (extract/<version>) and symbols (config/<version>/arm9/**/symbols.txt)."""

    def __init__(self, version: str | None = None):
        version = version or default_version()
        if version not in VERSIONS:
            raise ValueError(f"unknown version {version!r}; expected one of {VERSIONS}")
        reason = missing_reason(version)
        if reason:
            raise FileNotFoundError(reason)
        self.version = version
        self.extract = REPO / "extract" / version
        self.config = REPO / "config" / version / "arm9"
        self.modules = self._read_modules()
        self._symbols: dict[str, list[Symbol]] = {}
        self._functions: dict[str, list[tuple[int, int, str]]] = {}
        for path in sorted(self.config.rglob("symbols.txt")):
            module = "main" if path.parent == self.config else path.parent.name
            self._read_symbols(path, module)
        for functions in self._functions.values():
            functions.sort()
        self._overlay_table = _read_overlay_table(self.extract / "arm9_overlays" / "overlays.yaml")

    def _read_modules(self) -> list[Module]:
        rom_config = self.extract / "config.yaml"
        arm9 = _read_flat_yaml(self.extract / "arm9" / "arm9.yaml")
        modules = [self._module("main", self.extract / "arm9" / "arm9.bin", arm9)]
        # The autoloads' binaries and configs, as config.yaml lists them
        pairs = re.findall(r"^\s+bin: (\S+)\n\s+config: (\S+)", rom_config.read_text(), re.MULTILINE)
        for binary, config in pairs:
            name = Path(binary).stem.replace("unk_autoload", "autoload")
            modules.append(self._module(name, self.extract / binary, _read_flat_yaml(self.extract / config)))
        return modules

    @staticmethod
    def _module(name: str, binary: Path, config: dict[str, object]) -> Module:
        data = binary.read_bytes()
        base = int(config["base_address"])
        bss = int(config.get("bss_size", 0))
        if name == "main":
            bss = int(config["bss_end"]) - int(config["bss_start"])
        return Module(name, base, data, bss)

    def _read_symbols(self, path: Path, module: str):
        for line in path.read_text().splitlines():
            match = SYMBOL_LINE.match(line)
            if not match:
                continue
            name, kind, params, address = match.groups()
            params = (params or "").split(",")
            size = next((int(p[5:], 0) for p in params if p.startswith("size=")), None)
            symbol = Symbol(name, int(address, 16), kind, "thumb" in params, size, module)
            self._symbols.setdefault(name, []).append(symbol)
            if kind == "function":
                self._functions.setdefault(module, []).append((symbol.address, size or 4, name))

    def symbol(self, name: str, module: str | None = None) -> Symbol:
        """A symbol by name. Main's is preferred when overlays define the same name; module picks another."""
        candidates = self._symbols.get(name, [])
        if module is not None:
            candidates = [s for s in candidates if s.module == module]
        if not candidates:
            where = f" in {module}" if module else ""
            raise KeyError(f"no symbol {name}{where} in config/{self.version}/arm9")
        return next((s for s in candidates if s.module == "main"), candidates[0])

    def describe(self, address: int, overlays: tuple[str, ...] = ()) -> str:
        """An address as the function that contains it plus an offset, for error messages. Overlays' functions are
        named only for the overlays given (the loaded ones), since overlays share addresses."""
        for name, (begin, size) in HARNESS_REGIONS.items():
            if begin <= address < begin + size:
                return f"{address:#010x} (the harness's {name})"
        for module in [m.name for m in self.modules] + list(overlays):
            functions = self._functions.get(module, [])
            index = bisect.bisect_right(functions, (address, 1 << 32, "")) - 1
            if index >= 0:
                start, size, name = functions[index]
                if address < start + size:
                    prefix = f"{module}:" if module.startswith("ov") else ""
                    return f"{address:#010x} ({prefix}{name}+{address - start:#x})"
        return f"{address:#010x}"

    def overlay(self, overlay_id: int) -> Module:
        info = self._overlay_table[overlay_id]
        data = (self.extract / "arm9_overlays" / str(info["file_name"])).read_bytes()
        if len(data) != int(info["code_size"]):
            raise ValueError(f"overlay {overlay_id}: the extracted file isn't decompressed")
        return Module(f"ov{overlay_id:03d}", int(info["base_address"]), data, int(info["bss_size"]))


class Emulator:
    """An ARM946E-S (ARMv5TE) running the ROM's ARM9 code, with scratch memory, a stack, Python callbacks and code
    stubs. One Emulator serves any number of calls; memory persists between them."""

    def __init__(self, version: str | None = None, *, rom: Rom | None = None, strict_alignment: bool = True,
                 max_instructions: int = DEFAULT_MAX_INSTRUCTIONS):
        import unicorn
        import unicorn.arm_const as arm

        self._uc_module = unicorn
        self._arm = arm
        self.rom = rom or Rom(version)
        self.version = self.rom.version
        self.max_instructions = max_instructions
        self.uc = unicorn.Uc(unicorn.UC_ARCH_ARM, unicorn.UC_MODE_ARM)
        self.uc.ctl_set_cpu_model(arm.UC_CPU_ARM_946)
        for begin, size in (ITCM, MAIN_RAM, IO, PALETTE, VRAM, OAM, SCRATCH, STACK, CALLBACKS, CODE_STUBS, SENTINEL):
            self.uc.mem_map(begin, size)
        for module in self.rom.modules:
            self.uc.mem_write(module.base, module.data)
        self.uc.reg_write(arm.UC_ARM_REG_CPSR, CPSR_SYSTEM_IRQS_OFF)
        self.uc.mem_write(CALLBACKS[0], struct.pack("<I", ARM_BX_LR) * (CALLBACKS[1] // 4))

        self._overlays: tuple[str, ...] = ()
        self._scratch_next = SCRATCH[0]
        self._callbacks: dict[int, Callable] = {}
        self._callback_slots: dict[int, int] = {}
        self._stub_next = CODE_STUBS[0]
        self._replaced: dict[int, Callable] = {}
        self._error: EmulatorError | None = None
        self._unmapped: tuple[str, int, int] | None = None

        self.uc.hook_add(unicorn.UC_HOOK_CODE, self._on_callback, begin=CALLBACKS[0],
                         end=CALLBACKS[0] + CALLBACKS[1] - 1)
        self.uc.hook_add(unicorn.UC_HOOK_MEM_INVALID, self._on_invalid)
        self.uc.hook_add(unicorn.UC_HOOK_INTR, self._on_interrupt)
        self._alignment_hook = None
        self.strict_alignment = strict_alignment

    # Memory

    @property
    def strict_alignment(self) -> bool:
        return self._alignment_hook is not None

    @strict_alignment.setter
    def strict_alignment(self, enabled: bool):
        """Whether an unaligned halfword or word access stops the call with UnalignedAccess (the default)."""
        unicorn = self._uc_module
        if enabled and self._alignment_hook is None:
            self._alignment_hook = self.uc.hook_add(unicorn.UC_HOOK_MEM_READ | unicorn.UC_HOOK_MEM_WRITE,
                                                    self._on_access)
        elif not enabled and self._alignment_hook is not None:
            self.uc.hook_del(self._alignment_hook)
            self._alignment_hook = None

    def alloc(self, size: int, *, align: int = 16, data: bytes | None = None, fill: int = 0) -> int:
        """Reserves size bytes of scratch memory, aligned to align, filled with data (then fill) and returns its
        address. Scratch memory lasts until reset_scratch()."""
        address = (self._scratch_next + align - 1) & -align
        if address + size > SCRATCH[0] + SCRATCH[1]:
            raise MemoryError("scratch memory is exhausted; call reset_scratch() between cases")
        self._scratch_next = address + size
        contents = (data or b"")[:size]
        self.uc.mem_write(address, contents + bytes([fill]) * (size - len(contents)))
        return address

    def reset_scratch(self):
        """Releases all scratch memory. Its contents stay until overwritten."""
        self._scratch_next = SCRATCH[0]

    def read(self, address: int, size: int) -> bytes:
        return bytes(self.uc.mem_read(address, size))

    def write(self, address: int, data: bytes):
        """Writes data. Code written here isn't seen by already translated blocks; use stub() for code."""
        self.uc.mem_write(address, bytes(data))

    def read_u8(self, address: int) -> int:
        return self.read(address, 1)[0]

    def read_u16(self, address: int) -> int:
        return struct.unpack("<H", self.read(address, 2))[0]

    def read_u32(self, address: int) -> int:
        return struct.unpack("<I", self.read(address, 4))[0]

    def read_s32(self, address: int) -> int:
        return struct.unpack("<i", self.read(address, 4))[0]

    def write_u8(self, address: int, value: int):
        self.write(address, struct.pack("<B", value & 0xFF))

    def write_u16(self, address: int, value: int):
        self.write(address, struct.pack("<H", value & 0xFFFF))

    def write_u32(self, address: int, value: int):
        self.write(address, struct.pack("<I", value & 0xFFFFFFFF))

    def load_overlay(self, overlay_id: int):
        """Loads an overlay's code and .data at its address and zeroes its .bss. Overlays that share an address
        replace each other, as on the DS; its static initializers don't run."""
        module = self.rom.overlay(overlay_id)
        self.uc.mem_write(module.base, module.data + bytes(module.bss_size))
        self.uc.ctl_remove_cache(module.base, module.base + len(module.data))
        # An overlay that this one overlaps is gone
        end = module.base + len(module.data) + module.bss_size
        kept = []
        for name in self._overlays:
            other = self.rom.overlay(int(name[2:]))
            if other.base + len(other.data) + other.bss_size <= module.base or other.base >= end:
                kept.append(name)
        self._overlays = tuple(kept) + (module.name,)

    def describe(self, address: int) -> str:
        """An address as the loaded function that contains it plus an offset."""
        return self.rom.describe(address, self._overlays)

    # Code

    def callback(self, function: Callable) -> int:
        """Returns an ARM address that, when the ROM's code calls it, runs function(emu, r0, r1, r2, r3) in Python
        and returns to the caller. The function returns the result: an int for r0, an I64 or a (r0, r1) pair, or
        None to leave r0 as it is. Further arguments are read with stack_arg() or args(). The same function object
        always gets the same address. A callback must not call Emulator.call()."""
        key = id(function)
        if key in self._callback_slots:
            return self._callback_slots[key]
        address = CALLBACKS[0] + len(self._callbacks) * CALLBACK_SLOT
        if address >= CALLBACKS[0] + CALLBACKS[1]:
            raise MemoryError("too many callbacks")
        self._callbacks[address] = function
        self._callback_slots[key] = address
        return address

    def stub(self, code: bytes, *, thumb: bool = False) -> int:
        """Writes machine code to the code stub area and returns its address to call or pass as a function
        pointer, with bit 0 set for Thumb code. The code ends with its own return (bx lr)."""
        address = (self._stub_next + 7) & ~7
        if address + len(code) > CODE_STUBS[0] + CODE_STUBS[1]:
            raise MemoryError("the code stub area is full")
        self.uc.mem_write(address, bytes(code))
        self.uc.ctl_remove_cache(address, address + len(code))
        self._stub_next = address + len(code)
        return address | 1 if thumb else address

    def replace(self, name: str, function: Callable, module: str | None = None):
        """Makes calls to a ROM function run function(emu, r0, r1, r2, r3) instead, which returns as callback()'s
        functions do: for example DC_FlushRange as a no-op, since the CP15 cache operations aren't modelled."""
        symbol = self.rom.symbol(name, module)
        self._replaced[symbol.address] = function
        self.uc.hook_add(self._uc_module.UC_HOOK_CODE, self._on_replaced, begin=symbol.address,
                         end=symbol.address + 1)

    # Calls

    def call(self, function: str | int | Symbol, *args: int | I64, max_instructions: int | None = None
             ) -> CallResult:
        """Calls a ROM function (a name, a Symbol, or an address with bit 0 set for Thumb) with up to any number
        of arguments, as MWCC passes them, and returns r0 and r1 once it returns to the sentinel return address.
        Raises Fault, UnalignedAccess or InstructionLimit when it can't."""
        arm = self._arm
        if isinstance(function, str):
            function = self.rom.symbol(function)
        if isinstance(function, Symbol) and function.module.startswith("ov") and function.module not in self._overlays:
            raise EmulatorError(f"{function.name} is in {function.module}: load_overlay({int(function.module[2:])}) "
                                f"first")
        entry = function.entry if isinstance(function, Symbol) else function

        words: list[int] = []
        for arg in args:
            if isinstance(arg, I64):
                words += [arg.value & 0xFFFFFFFF, arg.value >> 32 & 0xFFFFFFFF]
            else:
                words.append(arg & 0xFFFFFFFF)
        stacked = words[4:]
        sp = (STACK_TOP - 4 * len(stacked)) & ~7
        for index, word in enumerate(stacked):
            self.write_u32(sp + 4 * index, word)
        registers = [arm.UC_ARM_REG_R0, arm.UC_ARM_REG_R1, arm.UC_ARM_REG_R2, arm.UC_ARM_REG_R3]
        for index, register in enumerate(registers):
            self.uc.reg_write(register, words[index] if index < len(words) else 0)
        # The callee-saved registers get values that show whether they come back intact
        saved = {getattr(arm, f"UC_ARM_REG_R{n}"): 0xC0DE0000 + n for n in range(4, 12)}
        for register, value in saved.items():
            self.uc.reg_write(register, value)
        self.uc.reg_write(arm.UC_ARM_REG_R12, 0)
        self.uc.reg_write(arm.UC_ARM_REG_SP, sp)
        self.uc.reg_write(arm.UC_ARM_REG_LR, RETURN_ADDRESS)

        self._error = None
        self._unmapped = None
        limit = self.max_instructions if max_instructions is None else max_instructions
        try:
            self.uc.emu_start(entry, RETURN_ADDRESS, count=limit)
        except self._uc_module.UcError as error:
            raise self._error or self._fault(str(error)) from None
        if self._error:
            raise self._error
        pc = self.uc.reg_read(arm.UC_ARM_REG_PC)
        if pc != RETURN_ADDRESS:
            raise InstructionLimit(f"{self._name(entry)} didn't return within {limit} instructions; "
                                   f"stopped at {self.describe(pc)}")
        for register, value in saved.items():
            if self.uc.reg_read(register) != value:
                raise EmulatorError(f"{self._name(entry)} didn't preserve a callee-saved register")
        if self.uc.reg_read(arm.UC_ARM_REG_SP) != sp:
            raise EmulatorError(f"{self._name(entry)} returned with a different stack pointer")
        return CallResult(self.uc.reg_read(arm.UC_ARM_REG_R0), self.uc.reg_read(arm.UC_ARM_REG_R1))

    def stack_arg(self, index: int) -> int:
        """In a callback: the word at index of the stack arguments (the fifth argument word is 0)."""
        return self.read_u32(self.uc.reg_read(self._arm.UC_ARM_REG_SP) + 4 * index)

    def args(self, sizes: list[int]) -> list[int]:
        """In a callback: the arguments as words (size 4) and 64-bit values (size 8), passed as call() passes
        them, unsigned."""
        arm = self._arm
        registers = [arm.UC_ARM_REG_R0, arm.UC_ARM_REG_R1, arm.UC_ARM_REG_R2, arm.UC_ARM_REG_R3]
        words_needed = sum(1 if size <= 4 else 2 for size in sizes)
        words = [self.uc.reg_read(r) for r in registers]
        words += [self.stack_arg(i) for i in range(max(0, words_needed - 4))]
        values, position = [], 0
        for size in sizes:
            if size <= 4:
                values.append(words[position])
                position += 1
            else:
                values.append(words[position] | words[position + 1] << 32)
                position += 2
        return values

    @contextlib.contextmanager
    def record_writes(self, begin: int, end: int, *, reads: bool = False) -> Iterator[list[Access]]:
        """Records the writes (and with reads=True the reads too, with a negative size) to [begin, end) while the
        block runs, in order, for comparing what a function sent to an I/O register or a FIFO."""
        unicorn = self._uc_module
        accesses: list[Access] = []

        def on_access(uc, access, address, size, value, _):
            pc = uc.reg_read(self._arm.UC_ARM_REG_PC)
            if access == unicorn.UC_MEM_WRITE:
                accesses.append(Access(address, size, value & ((1 << 8 * size) - 1), pc))
            else:
                data = bytes(uc.mem_read(address, size))
                accesses.append(Access(address, -size, int.from_bytes(data, "little"), pc))

        kinds = unicorn.UC_HOOK_MEM_WRITE | (unicorn.UC_HOOK_MEM_READ if reads else 0)
        handle = self.uc.hook_add(kinds, on_access, begin=begin, end=end - 1)
        try:
            yield accesses
        finally:
            self.uc.hook_del(handle)

    # Hooks

    def _name(self, entry: int) -> str:
        return self.describe(entry & ~1)

    def _fault(self, reason: str) -> Fault:
        arm = self._arm
        pc = self.uc.reg_read(arm.UC_ARM_REG_PC)
        lr = self.uc.reg_read(arm.UC_ARM_REG_LR)
        detail = ""
        if self._unmapped:
            kind, address, size = self._unmapped
            detail = f": {kind} of {size} bytes at {address:#010x}"
        return Fault(f"{reason}{detail}, at {self.describe(pc)}, lr {self.describe(lr)}")

    def _on_invalid(self, uc, access, address, size, value, _):
        unicorn = self._uc_module
        kinds = {unicorn.UC_MEM_READ_UNMAPPED: "read", unicorn.UC_MEM_WRITE_UNMAPPED: "write",
                 unicorn.UC_MEM_FETCH_UNMAPPED: "fetch"}
        self._unmapped = (kinds.get(access, "access"), address, size)
        return False

    def _on_interrupt(self, uc, number, _):
        arm = self._arm
        pc = uc.reg_read(arm.UC_ARM_REG_PC)
        thumb = uc.reg_read(arm.UC_ARM_REG_CPSR) & 0x20
        if number == 2:  # SVC: the comment field is in the instruction before the return address
            if thumb:
                what = f"SVC {self.read(pc - 2, 1)[0]:#x} (a BIOS call)"
            else:
                what = f"SVC {self.read_u32(pc - 4) & 0xFFFFFF:#x} (a BIOS call)"
        else:
            what = {1: "undefined instruction", 3: "prefetch abort", 4: "data abort", 7: "breakpoint"}.get(
                number, f"exception {number}")
        self._error = Fault(f"{what} near {self.describe(pc)}")
        uc.emu_stop()

    def _on_access(self, uc, access, address, size, value, _):
        if size in (2, 4) and address & (size - 1):
            unicorn = self._uc_module
            kind = "write" if access == unicorn.UC_MEM_WRITE else "read"
            pc = uc.reg_read(self._arm.UC_ARM_REG_PC)
            self._error = UnalignedAccess(f"unaligned {size}-byte {kind} at {address:#010x} by "
                                          f"{self.describe(pc)}")
            uc.emu_stop()

    def _return_from_hook(self, result):
        arm = self._arm
        if isinstance(result, I64):
            result = (result.value & 0xFFFFFFFF, result.value >> 32 & 0xFFFFFFFF)
        if isinstance(result, tuple):
            self.uc.reg_write(arm.UC_ARM_REG_R0, result[0] & 0xFFFFFFFF)
            self.uc.reg_write(arm.UC_ARM_REG_R1, result[1] & 0xFFFFFFFF)
        elif result is not None:
            self.uc.reg_write(arm.UC_ARM_REG_R0, result & 0xFFFFFFFF)

    def _registers(self) -> list[int]:
        arm = self._arm
        return [self.uc.reg_read(r) for r in (arm.UC_ARM_REG_R0, arm.UC_ARM_REG_R1, arm.UC_ARM_REG_R2,
                                              arm.UC_ARM_REG_R3)]

    def _on_callback(self, uc, address, size, _):
        function = self._callbacks.get(address)
        if function is None:
            self._error = Fault(f"call to an unassigned callback slot {address:#010x}")
            uc.emu_stop()
            return
        # The slot's own bx lr then returns to the caller
        self._return_from_hook(function(self, *self._registers()))

    def _on_replaced(self, uc, address, size, _):
        function = self._replaced.get(address)
        if function is None:
            return
        self._return_from_hook(function(self, *self._registers()))
        uc.reg_write(self._arm.UC_ARM_REG_PC, uc.reg_read(self._arm.UC_ARM_REG_LR))

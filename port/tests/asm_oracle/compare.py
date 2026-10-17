"""Calls the ROM's original function (in the emulator) and its host C version (in a built library) on the same
seeded random inputs, and compares the return values, every byte of every memory region the call could see
(including a margin around each), and the calls each made to the function pointers it was given.

    def generate(rng):
        size = rng.randrange(0, 64)
        return Case({"src": rng.randbytes(size), "dst": rng.randbytes(size)},
                    [Ptr("src", rng.randrange(4)), Ptr("dst", rng.randrange(4)), size])

    compare(emu, lib, "MI_CpuCopy8", "void(ptr, ptr, u32)", generate)

A region is placed at the same alignment on both sides (its start is a multiple of ALIGN), so a pointer's offset
decides its low address bits for the original and the host version alike. The margin (pad bytes before and after
each region) is filled with the same seeded bytes on both sides and compared too, so a write past either end, or a
result that depends on bytes read there, is caught. Pointers that a function returns or passes to a callback are
compared as a region and an offset; any pointer outside the regions (the stack, a static table) compares equal to
any other.
"""

from __future__ import annotations

import ctypes
import random
import re
import sys
import traceback
from dataclasses import dataclass, field
from typing import Callable

from harness import I64, EmulatorError, missing_reason

ALIGN = 0x1000
DEFAULT_PAD = 64

# Each type: its size in bytes, whether it's signed, and its ctypes type
TYPES = {
    "void": (0, False, None),
    "u8": (1, False, ctypes.c_uint8),
    "s8": (1, True, ctypes.c_int8),
    "u16": (2, False, ctypes.c_uint16),
    "s16": (2, True, ctypes.c_int16),
    "u32": (4, False, ctypes.c_uint32),
    "s32": (4, True, ctypes.c_int32),
    "u64": (8, False, ctypes.c_uint64),
    "s64": (8, True, ctypes.c_int64),
    "ptr": (4, False, ctypes.c_void_p),
    "fn": (4, False, ctypes.c_void_p),
}


@dataclass(frozen=True)
class Signature:
    """A C function type in the TYPES names: Signature("s32", ("ptr", "u32")), or parsed: "s32(ptr, u32)"."""

    ret: str
    args: tuple[str, ...]

    @staticmethod
    def parse(text: str | Signature) -> Signature:
        if isinstance(text, Signature):
            return text
        match = re.fullmatch(r"\s*(\w+)\s*\(([^)]*)\)\s*", text)
        if not match:
            raise ValueError(f"bad signature {text!r}; expected e.g. 's32(ptr, u32)'")
        args = tuple(a.strip() for a in match.group(2).split(",") if a.strip() and a.strip() != "void")
        signature = Signature(match.group(1), args)
        for name in (signature.ret, *signature.args):
            if name not in TYPES:
                raise ValueError(f"unknown type {name!r} in {text!r}; types are {', '.join(TYPES)}")
        return signature


def normalize(type_name: str, value: int) -> int:
    """A value as the C type holds it: truncated to its width, sign-extended when it's signed."""
    size, signed, _ = TYPES[type_name]
    bits = 8 * size
    value &= (1 << bits) - 1
    return value - (1 << bits) if signed and value >> (bits - 1) else value


@dataclass(frozen=True)
class Ptr:
    """A pointer argument: an offset into one of the case's regions (negative, or past the end, reaches the pad)."""

    region: str
    offset: int = 0


class Memory:
    """What a callback reads and writes memory through, on either side: addresses are the side's own."""

    def read(self, address: int, size: int) -> bytes:
        raise NotImplementedError

    def write(self, address: int, data: bytes):
        raise NotImplementedError

    def _int(self, address: int, size: int, signed: bool) -> int:
        return int.from_bytes(self.read(address, size), "little", signed=signed)

    def u8(self, address: int) -> int:
        return self._int(address, 1, False)

    def s8(self, address: int) -> int:
        return self._int(address, 1, True)

    def u16(self, address: int) -> int:
        return self._int(address, 2, False)

    def s16(self, address: int) -> int:
        return self._int(address, 2, True)

    def u32(self, address: int) -> int:
        return self._int(address, 4, False)

    def s32(self, address: int) -> int:
        return self._int(address, 4, True)


class EmulatorMemory(Memory):
    def __init__(self, emu):
        self.emu = emu

    def read(self, address: int, size: int) -> bytes:
        return self.emu.read(address, size)

    def write(self, address: int, data: bytes):
        self.emu.write(address, data)


class HostMemory(Memory):
    def read(self, address: int, size: int) -> bytes:
        return ctypes.string_at(address, size)

    def write(self, address: int, data: bytes):
        ctypes.memmove(address, bytes(data), len(data))


class Callback:
    """A function pointer argument, written in Python once for both sides: function(memory, *args) returns the
    result (an int, or None for void). memory is a Memory for the side that calls it, and pointer arguments are
    that side's addresses. Each call is logged, and the logs of the two sides must be the same. An exception it
    raises on the host side can't cross the C frames: ctypes prints it and the call returns 0, which then shows as
    a difference."""

    def __init__(self, function: Callable, signature: str | Signature = "s32(ptr, ptr)"):
        self.function = function
        self.signature = Signature.parse(signature)
        self.log: list = []
        self._translate: Callable[[int], object] = lambda value: value
        self._emulator_entries: dict[int, int] = {}
        self._host_pointer = None

    def _invoke(self, memory: Memory, values: list[int]):
        values = [normalize(t, v) if t not in ("ptr", "fn") else v for t, v in zip(self.signature.args, values)]
        result = self.function(memory, *values)
        logged = tuple(self._translate(v) if t == "ptr" else v for t, v in zip(self.signature.args, values))
        if self.signature.ret == "void":
            result = None
        elif result is not None:
            result = normalize(self.signature.ret, result)
        self.log.append((logged, result))
        return result

    def emulator_entry(self, emu) -> int:
        if id(emu) not in self._emulator_entries:
            sizes = [TYPES[t][0] for t in self.signature.args]

            def on_call(emu, *_):
                result = self._invoke(EmulatorMemory(emu), emu.args(sizes))
                if result is not None and TYPES[self.signature.ret][0] == 8:
                    return I64(result & 0xFFFFFFFFFFFFFFFF)
                return result

            self._emulator_entries[id(emu)] = emu.callback(on_call)
        return self._emulator_entries[id(emu)]

    def host_pointer(self):
        if self._host_pointer is None:
            ret = TYPES[self.signature.ret][2]
            prototype = ctypes.CFUNCTYPE(ret, *(TYPES[t][2] for t in self.signature.args))

            def on_call(*values):
                result = self._invoke(HostMemory(), [v or 0 for v in values])
                return 0 if result is None and ret is not None else result

            self._host_pointer = prototype(on_call)
        return self._host_pointer


@dataclass
class Case:
    """One call: the memory regions it gets (name to initial bytes) and its arguments (ints, Ptr, Callback)."""

    regions: dict[str, bytes]
    args: list
    note: str = ""


@dataclass
class Outcome:
    ret: object = None
    memory: dict[str, bytes] = field(default_factory=dict)
    calls: list = field(default_factory=list)
    error: str | None = None


class Mismatch(AssertionError):
    """The original and the host version disagree on a case; the message has the inputs and the first difference."""


def _pad_bytes(seed: object, index: int, name: str, pad: int) -> tuple[bytes, bytes]:
    rng = random.Random(f"pad:{seed}:{index}:{name}")
    return rng.randbytes(pad), rng.randbytes(pad)


class _Layout:
    """Where a case's regions are on one side, and how to translate a pointer back to a region and an offset."""

    def __init__(self, case: Case, pad: int):
        self.case = case
        self.pad = pad
        self.starts: dict[str, int] = {}

    def translate(self, value: int | None) -> object:
        """A pointer as NULL, or a region and an offset. A pointer outside every region (the stack, a static
        table) has no counterpart on the other side, so all of them compare equal: a test checks those itself."""
        if not value:
            return "NULL"
        for name, start in self.starts.items():
            if start - self.pad <= value <= start + len(self.case.regions[name]) + self.pad:
                return (name, value - start)
        return "outside every region"

    def resolve(self, arg) -> int:
        return self.starts[arg.region] + arg.offset


def _check_signature(signature: Signature, case: Case):
    if len(case.args) != len(signature.args):
        raise ValueError(f"the case has {len(case.args)} arguments, the signature {len(signature.args)}")
    for type_name, arg in zip(signature.args, case.args):
        if isinstance(arg, Ptr) and type_name != "ptr":
            raise ValueError(f"a Ptr is passed as {type_name}")
        if isinstance(arg, Callback) and type_name not in ("fn", "ptr"):
            raise ValueError(f"a Callback is passed as {type_name}")


def run_original(emu, function, signature: str | Signature, case: Case, *, pad: int = DEFAULT_PAD,
                 pads: dict[str, tuple[bytes, bytes]] | None = None) -> Outcome:
    """Runs one case on the ROM's function in the emulator."""
    signature = Signature.parse(signature)
    _check_signature(signature, case)
    emu.reset_scratch()
    layout = _Layout(case, pad)
    pads = pads or {name: (bytes(pad), bytes(pad)) for name in case.regions}
    for name, data in case.regions.items():
        span = emu.alloc(ALIGN + len(data) + pad, align=ALIGN)
        start = span + ALIGN
        before, after = pads[name]
        emu.write(start - pad, before + bytes(data) + after)
        layout.starts[name] = start

    callbacks = [arg for arg in case.args if isinstance(arg, Callback)]
    for callback in callbacks:
        callback.log = []
        callback._translate = layout.translate
    words = []
    for type_name, arg in zip(signature.args, case.args):
        if isinstance(arg, Ptr):
            words.append(layout.resolve(arg))
        elif isinstance(arg, Callback):
            words.append(arg.emulator_entry(emu))
        elif TYPES[type_name][0] == 8:
            words.append(I64(normalize(type_name, arg) & 0xFFFFFFFFFFFFFFFF))
        else:
            words.append(normalize(type_name, arg) & 0xFFFFFFFF)

    outcome = Outcome()
    try:
        result = emu.call(function, *words)
    except EmulatorError as error:
        outcome.error = f"{type(error).__name__}: {error}"
        return outcome
    if signature.ret == "ptr":
        outcome.ret = layout.translate(result.r0)
    elif signature.ret != "void":
        outcome.ret = normalize(signature.ret, result.u64 if TYPES[signature.ret][0] == 8 else result.r0)
    for name, data in case.regions.items():
        outcome.memory[name] = emu.read(layout.starts[name] - pad, len(data) + 2 * pad)
    outcome.calls = [(index, call) for index, callback in enumerate(callbacks) for call in callback.log]
    return outcome


def run_host(function, signature: str | Signature, case: Case, *, pad: int = DEFAULT_PAD,
             pads: dict[str, tuple[bytes, bytes]] | None = None) -> Outcome:
    """Runs one case on a host function (a ctypes function; its argument and result types are set here)."""
    signature = Signature.parse(signature)
    _check_signature(signature, case)
    if pad > ALIGN:
        raise ValueError(f"pad must be at most {ALIGN:#x}")
    function.restype = TYPES[signature.ret][2]
    function.argtypes = [TYPES[t][2] for t in signature.args]
    layout = _Layout(case, pad)
    pads = pads or {name: (bytes(pad), bytes(pad)) for name in case.regions}
    buffers = []
    for name, data in case.regions.items():
        buffer = ctypes.create_string_buffer(len(data) + pad + 2 * ALIGN)
        base = ctypes.addressof(buffer)
        start = (base + pad + ALIGN - 1) & -ALIGN
        before, after = pads[name]
        contents = before + bytes(data) + after
        ctypes.memmove(start - pad, contents, len(contents))
        layout.starts[name] = start
        buffers.append(buffer)

    callbacks = [arg for arg in case.args if isinstance(arg, Callback)]
    for callback in callbacks:
        callback.log = []
        callback._translate = layout.translate
    values = []
    for type_name, arg in zip(signature.args, case.args):
        if isinstance(arg, Ptr):
            values.append(layout.resolve(arg))
        elif isinstance(arg, Callback):
            values.append(ctypes.cast(arg.host_pointer(), ctypes.c_void_p).value)
        elif type_name in ("ptr", "fn"):
            values.append(arg or None)
        else:
            values.append(normalize(type_name, arg))

    outcome = Outcome()
    result = function(*values)
    if signature.ret == "ptr":
        outcome.ret = layout.translate(result)
    elif signature.ret != "void":
        outcome.ret = normalize(signature.ret, result)
    for name, data in case.regions.items():
        outcome.memory[name] = ctypes.string_at(layout.starts[name] - pad, len(data) + 2 * pad)
    outcome.calls = [(index, call) for index, callback in enumerate(callbacks) for call in callback.log]
    del buffers
    return outcome


def _hex(data: bytes, limit: int = 64) -> str:
    text = data[:limit].hex(" ")
    return text + (f" ... ({len(data)} bytes)" if len(data) > limit else "")


def _describe_case(case: Case, signature: Signature) -> list[str]:
    lines = []
    if case.note:
        lines.append(f"  note: {case.note}")
    shown = []
    for type_name, arg in zip(signature.args, case.args):
        if isinstance(arg, Ptr):
            shown.append(f"&{arg.region}[{arg.offset}]")
        elif isinstance(arg, Callback):
            shown.append(f"callback {getattr(arg.function, '__name__', '?')}")
        else:
            shown.append(f"{normalize(type_name, arg):#x}" if type_name in ("ptr", "fn") else
                         str(normalize(type_name, arg)))
    lines.append(f"  arguments: ({', '.join(shown)})")
    for name, data in case.regions.items():
        lines.append(f"  {name} ({len(data)} bytes, starts at a multiple of {ALIGN:#x}): {_hex(data)}")
    return lines


def _first_difference(original: Outcome, host: Outcome, pad: int) -> list[str]:
    if original.ret != host.ret:
        return [f"  return value: original {original.ret!r}, host {host.ret!r}"]
    for name, expected in original.memory.items():
        actual = host.memory[name]
        if expected != actual:
            index = next(i for i in range(len(expected)) if expected[i] != actual[i])
            offset = index - pad
            where = ("in the pad before it" if offset < 0 else
                     "in the pad after it" if offset >= len(expected) - 2 * pad else "")
            low = max(0, index - 8)
            return [f"  memory: {name}[{offset}] {where} differs (original {expected[index]:#04x}, host "
                    f"{actual[index]:#04x}); from {name}[{low - pad}]:",
                    f"    original {expected[low:low + 32].hex(' ')}",
                    f"    host     {actual[low:low + 32].hex(' ')}"]
    if original.calls != host.calls:
        index = next((i for i, (a, b) in enumerate(zip(original.calls, host.calls)) if a != b),
                     min(len(original.calls), len(host.calls)))
        return [f"  callback calls: {len(original.calls)} by the original, {len(host.calls)} by the host; "
                f"call {index} differs:",
                f"    original {original.calls[index] if index < len(original.calls) else 'none'}",
                f"    host     {host.calls[index] if index < len(host.calls) else 'none'}"]
    return []


def compare(emu, lib, function: str, signature: str | Signature, generate: Callable[[random.Random], Case], *,
            count: int = 1000, seed: object = 0, host_function=None, pad: int = DEFAULT_PAD,
            only: int | None = None) -> int:
    """Runs count cases from generate(rng) on the ROM's function and on the host's (by default lib's function of
    the same name) and raises Mismatch at the first case where they differ, or where the original faults (the
    generator then made an input the original doesn't accept). Case i draws from random.Random(f"{seed}:{i}"), so
    only=i runs that case alone. Returns the number of cases run."""
    signature = Signature.parse(signature)
    host = host_function if host_function is not None else getattr(lib.cdll, function)
    if isinstance(host, str):
        host = getattr(lib.cdll, host)
    indices = [only] if only is not None else range(count)
    for index in indices:
        case = generate(random.Random(f"{seed}:{index}"))
        pads = {name: _pad_bytes(seed, index, name, pad) for name in case.regions}
        original = run_original(emu, function, signature, case, pad=pad, pads=pads)
        name = function if isinstance(function, str) else getattr(function, "name", None) or f"{function:#x}"
        header = [f"{name}: case {index} of seed {seed!r} (rerun it with only={index})"]
        if original.error:
            raise Mismatch("\n".join(header + _describe_case(case, signature) +
                                     [f"  the original can't run it: {original.error}"]))
        outcome = run_host(host, signature, case, pad=pad, pads=pads)
        difference = _first_difference(original, outcome, pad)
        if difference:
            raise Mismatch("\n".join(header + _describe_case(case, signature) + difference))
    return len(indices)


def main(tests: list[Callable[[], object]], version: str | None = None):
    """Runs a test file's test functions and exits as run_all.py expects: 0 when all pass, 1 when one fails, 77 when
    the ROM isn't extracted. A test passes when it returns without raising; what it returns is printed."""
    from build import BuildError

    reason = missing_reason(version)
    if reason:
        print(f"skipped: {reason}")
        sys.exit(77)
    failed = 0
    for test in tests:
        try:
            result = test()
        except Exception as error:  # noqa: BLE001 - every failure is reported, then the next test runs
            failed += 1
            print(f"FAIL {test.__name__}")
            if isinstance(error, (Mismatch, EmulatorError, BuildError)):
                print(str(error))
            else:
                traceback.print_exc(file=sys.stdout)
        else:
            print(f"ok   {test.__name__}" + (f": {result}" if result is not None else ""))
        sys.stdout.flush()
    sys.exit(1 if failed else 0)

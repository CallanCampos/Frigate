#!/usr/bin/env python3
"""Print an object's Thumb functions, or ARM ones with --arm, in the format of `arm-none-eabi-objdump -drz`, for
decomp-permuter.

decomp-permuter parses GNU objdump output, which needs an ARM build of binutils. This prints the same format with
capstone. Relocated bytes are zeroed, so calls and pointers compare equal between our objects and the target object
that permuter_setup.py makes, whose relocated bytes are zeroed too.

    permuter_objdump.py [-drz] [--arm] FILE.o
"""
import sys

import capstone
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection


def main():
    path = sys.argv[-1]
    with open(path, "rb") as f:
        elf = ELFFile(f)
        relocated: dict[int, set[int]] = {}
        for section in elf.iter_sections():
            if isinstance(section, RelocationSection):
                offsets = relocated.setdefault(section["sh_info"], set())
                for reloc in section.iter_relocations():
                    offsets.update(range(reloc["r_offset"], reloc["r_offset"] + 4))
        mode = capstone.CS_MODE_ARM if "--arm" in sys.argv else capstone.CS_MODE_THUMB
        disassembler = capstone.Cs(capstone.CS_ARCH_ARM, mode)
        print(f"\n{path}:     file format elf32-littlearm\n\n\nDisassembly of section .text:")
        for symbol in elf.get_section_by_name(".symtab").iter_symbols():
            if symbol["st_info"]["type"] != "STT_FUNC" or symbol["st_shndx"] in ("SHN_UNDEF", "SHN_ABS"):
                continue
            if symbol.name.startswith("$"):
                continue
            data = bytearray(elf.get_section(symbol["st_shndx"]).data())
            for offset in relocated.get(symbol["st_shndx"], ()):
                if offset < len(data):
                    data[offset] = 0
            start = symbol["st_value"] & ~1
            size = symbol["st_size"] or len(data) - start
            code = bytes(data[start : start + size])
            print(f"\n{0:08x} <{symbol.name}>:")
            for insn in disassembler.disasm(code, 0):
                raw = code[insn.address : insn.address + insn.size].hex()
                print(f"  {insn.address:4x}:\t{raw}\t{insn.mnemonic}\t{insn.op_str}")


if __name__ == "__main__":
    main()

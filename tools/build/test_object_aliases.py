"""object_aliases.py on a hand-built ELF object and temporary configs, without ROMs or a compiler.

    python3 -m unittest tools.build.test_object_aliases
"""
import struct
import tempfile
import unittest
from pathlib import Path

from tools.build.object_aliases import add_aliases, label_aliases

STB_GLOBAL_FUNC = (1 << 4) | 2


def elf_object(symbols: list[tuple[str, int, int, int]]) -> bytes:
    """Returns a relocatable ELF with a null section, .text, .symtab and .strtab, and the (name, value, size, section)
    symbols after the null one."""
    strings = b"\0" + b"".join(name.encode() + b"\0" for name, *_ in symbols)
    offsets, at = [], 1
    for name, *_ in symbols:
        offsets.append(at)
        at += len(name) + 1
    table = bytes(16) + b"".join(struct.pack("<IIIBBH", o, v, s, STB_GLOBAL_FUNC, 0, x)
                                 for o, (_, v, s, x) in zip(offsets, symbols))
    text = bytes(range(0x40))
    body_start = 0x34
    text_at, table_at = body_start, body_start + len(text)
    strings_at = table_at + len(table)
    headers_at = strings_at + len(strings) + (-(strings_at + len(strings)) % 4)
    headers = [
        (0,) * 10,
        (0, 1, 6, 0, text_at, len(text), 0, 0, 4, 0),
        (0, 2, 0, 0, table_at, len(table), 3, 1, 4, 16),
        (0, 3, 0, 0, strings_at, len(strings), 0, 0, 1, 0),
    ]
    header = b"\x7fELF\x01\x01\x01" + bytes(9) + struct.pack(
        "<HHIIIIIHHHHHH", 1, 40, 1, 0, 0, headers_at, 0, 0x34, 0, 0, 40, len(headers), 0)
    body = text + table + strings
    return header + body + bytes(headers_at - body_start - len(body)) + b"".join(struct.pack("<10I", *h)
                                                                                 for h in headers)


def read_symbols(elf: bytes) -> dict[str, tuple[int, int, int, int]]:
    """Returns (value, size, info, section) of every named symbol."""
    shoff, = struct.unpack_from("<I", elf, 0x20)
    shnum, = struct.unpack_from("<H", elf, 0x30)
    headers = [struct.unpack_from("<10I", elf, shoff + i * 40) for i in range(shnum)]
    table = next(h for h in headers if h[1] == 2)
    strings = headers[table[6]]
    names = elf[strings[4]:strings[4] + strings[5]]
    out = {}
    for at in range(table[4], table[4] + table[5], 16):
        name, value, size, info, _, section = struct.unpack_from("<IIIBBH", elf, at)
        if name:
            out[names[name:names.index(b"\0", name)].decode()] = (value, size, info, section)
    return out


class AddAliasesTests(unittest.TestCase):
    def test_alias_copies_the_function_with_size_0(self):
        elf = elf_object([("_ull_mul", 0x20, 0x20, 1), ("__aeabi_dadd", 0, 0x10, 1)])
        symbols = read_symbols(add_aliases(elf, {"_ll_mul": "_ull_mul", "_dadd": "__aeabi_dadd"}))
        self.assertEqual(symbols["_ll_mul"], (0x20, 0, STB_GLOBAL_FUNC, 1))
        self.assertEqual(symbols["_dadd"], (0, 0, STB_GLOBAL_FUNC, 1))
        self.assertEqual(symbols["_ull_mul"], (0x20, 0x20, STB_GLOBAL_FUNC, 1))

    def test_the_code_is_unchanged(self):
        elf = elf_object([("_ull_mul", 0, 0x20, 1)])
        self.assertEqual(add_aliases(elf, {"_ll_mul": "_ull_mul"})[0x34:0x74], bytes(range(0x40)))

    def test_missing_function_or_defined_label_stops(self):
        elf = elf_object([("_ull_mul", 0, 0x20, 1)])
        with self.assertRaises(SystemExit):
            add_aliases(elf, {"_dadd": "__aeabi_dadd"})
        with self.assertRaises(SystemExit):
            add_aliases(elf, {"_ull_mul": "_ull_mul"})


class LabelAliasesTests(unittest.TestCase):
    def test_labels_at_function_starts_by_file(self):
        with tempfile.TemporaryDirectory() as temporary:
            module = Path(temporary) / "arm9"
            module.mkdir()
            (module / "symbols.txt").write_text(
                "_ull_mul kind:function(arm,size=0x20) addr:0x02000020\n"
                "_ll_mul kind:label(arm) addr:0x02000020\n"
                "_ll_sdiv kind:function(arm,size=0x10) addr:0x02000040\n"
                "_ll_sdiv kind:label(arm) addr:0x02000040\n"
                ".L_02000024 kind:label(arm) addr:0x02000024\n"
            )
            (module / "delinks.txt").write_text(
                "    .text       start:0x02000000 end:0x02000100 kind:code align:32\n\n"
                "lib/msl/src/fp/fp_fastI.c:\n    .text       start:0x02000000 end:0x02000100\n"
            )
            # A label inside a function, or one with the function's own name, is no alias
            self.assertEqual(label_aliases(Path(temporary)), {"lib/msl/src/fp/fp_fastI.c": {"_ll_mul": "_ull_mul"}})


if __name__ == "__main__":
    unittest.main()

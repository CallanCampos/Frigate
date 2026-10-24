#!/usr/bin/env python3
"""Check the dead-strip of tools/build/deadstrip.py on small objects laid out as MWCC writes them.

    python3 -m unittest tools.build.test_deadstrip

The objects are built here, so no compiler or ROM is needed, and read back with a reader of the tests' own.
"""
import io
import struct
import sys
import tempfile
import unittest
from contextlib import redirect_stderr
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.build import check_deadstrip, deadstrip  # noqa: E402

SHT_PROGBITS, SHT_SYMTAB, SHT_STRTAB, SHT_RELA, SHT_NOBITS = 1, 2, 3, 4, 8
SHF_WRITE, SHF_ALLOC, SHF_EXECINSTR = 0x1, 0x2, 0x4
STB_LOCAL, STB_GLOBAL = 0, 1
STT_NOTYPE, STT_OBJECT, STT_FUNC = 0, 1, 2
R_ARM_ABS32, R_ARM_THM_CALL = 2, 10
UNDEFINED = None


class ObjectBuilder:
    """Writes a relocatable ELF object as MWCC does: the symbol and string tables first, each section's relocations
    right after it, every section's contents at a multiple of 16 bytes, and the section headers last."""

    def __init__(self):
        self.sections = []  # (name, type, flags, body or NOBITS size, align)
        self.symbols = []  # (name, section handle or None, value, size, bind, type)
        self.relocations = {}  # section handle: [(offset, symbol handle, type, addend)]

    def section(self, name, body=b"", flags=0, kind=SHT_PROGBITS, align=4):
        self.sections.append((name, kind, flags, body, align))
        return len(self.sections) - 1

    def symbol(self, name, section, value=0, size=0, bind=STB_LOCAL, kind=STT_OBJECT):
        self.symbols.append((name, section, value, size, bind, kind))
        return len(self.symbols) - 1

    def relocate(self, section, offset, symbol, kind=R_ARM_ABS32, addend=0):
        self.relocations.setdefault(section, []).append((offset, symbol, kind, addend))

    def build(self):
        names = bytearray(b"\0")

        def name_offset(name):
            offset = len(names)
            names.extend(name.encode() + b"\0")
            return offset

        # Final order: the null section, the tables, then each section with its relocations
        layout = [None, "symtab", "strtab", "shstrtab"]
        index_of = {}
        for handle in range(len(self.sections)):
            index_of[handle] = len(layout)
            layout.append(("section", handle))
            if handle in self.relocations:
                layout.append(("rela", handle))
        strtab = bytearray(b"\0")
        symtab = bytearray(16)
        first_global = None
        for n, (name, section, value, size, bind, kind) in enumerate(self.symbols):
            if bind != STB_LOCAL and first_global is None:
                first_global = n + 1
            shndx = 0 if section is UNDEFINED else index_of[section]
            symtab += struct.pack("<IIIBBH", len(strtab), value, size, bind << 4 | kind, 0, shndx)
            strtab.extend(name.encode() + b"\0")
        headers = []
        for entry in layout:
            if entry is None:
                headers.append([0, 0, 0, b"", 0, 0, 0, 0, 0])
            elif entry == "symtab":
                headers.append([name_offset(".symtab"), SHT_SYMTAB, 0, bytes(symtab), 2,
                                first_global or len(self.symbols) + 1, 0, 16, None])
            elif entry == "strtab":
                headers.append([name_offset(".strtab"), SHT_STRTAB, 0, bytes(strtab), 0, 0, 0, 0, None])
            elif entry == "shstrtab":
                headers.append([0, SHT_STRTAB, 0, None, 0, 0, 0, 0, None])
            elif entry[0] == "section":
                name, kind, flags, body, align = self.sections[entry[1]]
                headers.append([name_offset(name), kind, flags, body, 0, 0, align, 0, None])
            else:
                body = b"".join(struct.pack("<IIi", offset, (symbol + 1) << 8 | kind, addend)
                                for offset, symbol, kind, addend in self.relocations[entry[1]])
                headers.append([name_offset(".rela" + self.sections[entry[1]][0]), SHT_RELA, 0, body, 1,
                                index_of[entry[1]], 0, 12, None])
        shstrtab_header = headers[3]
        shstrtab_header[0] = name_offset(".shstrtab")
        shstrtab_header[3] = bytes(names)
        out = bytearray(struct.pack("<16sHHIIIIIHHHHHH", b"\x7fELF\x01\x01\x01", 1, 40, 1, 0, 0, 0, 0, 52, 0, 0,
                                    40, len(layout), 3))
        table = bytearray()
        for name, kind, flags, body, link, info, align, entsize, _ in headers:
            if kind == 0:
                table += bytes(40)
                continue
            out += bytes(-len(out) % 16)
            size = body if kind == SHT_NOBITS else len(body)
            table += struct.pack("<10I", name, kind, flags, 0, len(out), size, link, info, align, entsize)
            if kind != SHT_NOBITS:
                out += body
        out += bytes(-len(out) % 16)
        struct.pack_into("<I", out, 0x20, len(out))
        return bytes(out + table)


def c_string(table, offset):
    return table[offset:table.index(b"\0", offset)].decode()


def read(data):
    """Reads an object back: its sections, symbols and relocations, by name."""
    (shoff,) = struct.unpack_from("<I", data, 0x20)
    shnum, shstrndx = struct.unpack_from("<HH", data, 0x30)
    headers = [struct.unpack_from("<10I", data, shoff + 40 * i) for i in range(shnum)]
    shstrtab = data[headers[shstrndx][4]:headers[shstrndx][4] + headers[shstrndx][5]]
    sections = []
    for name, kind, flags, _, offset, size, link, info, _, _ in headers:
        body = b"" if kind == SHT_NOBITS else data[offset:offset + size]
        sections.append(SimpleNamespace(name=c_string(shstrtab, name), type=kind, flags=flags, offset=offset,
                                        size=size, link=link, info=info, body=body))
    symtab = next(i for i, s in enumerate(sections) if s.type == SHT_SYMTAB)
    strtab = sections[sections[symtab].link].body
    symbols = []
    for offset in range(0, len(sections[symtab].body), 16):
        name, value, size, info, _, shndx = struct.unpack_from("<IIIBBH", sections[symtab].body, offset)
        symbols.append(SimpleNamespace(name=c_string(strtab, name), value=value, bind=info >> 4, shndx=shndx))
    relocations = {}
    for i, section in enumerate(sections):
        if section.type == SHT_RELA:
            assert section.link == symtab, "relocations must use the symbol table"
            relocations[section.info] = [
                (offset, symbols[info >> 8].name, info & 0xFF, addend)
                for offset, info, addend in struct.iter_unpack("<IIi", section.body)]
    return SimpleNamespace(sections=sections, symbols=symbols, symtab=symtab, relocations=relocations)


def defining(obj, name):
    """Returns the index of the section that defines a symbol."""
    return next(s.shndx for s in obj.symbols if s.name == name)


def fixture(live_calls_dead=False):
    """A file with a live and a dead function, as MWCC emits it: each function, string and unpooled data object in a
    section of its own, pooled data shared, exception indices, debug information and undefined callees."""
    b = ObjectBuilder()
    b.section(".comment", b"MW\0\0")
    debug = b.section(".debug_info", bytes(8), align=1)
    bss = b.section(".bss", 8, SHF_ALLOC | SHF_WRITE, SHT_NOBITS)
    pool = b.section(".rodata", b"LIVEDEAD", SHF_ALLOC)
    shared = b.section(".data", b"shared\0", SHF_ALLOC | SHF_WRITE, align=1)
    dead_string = b.section(".data", b"dead_one\0", SHF_ALLOC | SHF_WRITE, align=1)
    live_string = b.section(".data", b"live_one\0", SHF_ALLOC | SHF_WRITE, align=1)
    dead_index = b.section(".exceptix", bytes(12), SHF_ALLOC)
    dead_text = b.section(".text", b"D" * 24, SHF_ALLOC | SHF_EXECINSTR)
    live_index = b.section(".exceptix", bytes(12), SHF_ALLOC)
    live_text = b.section(".text", b"L" * 24, SHF_ALLOC | SHF_EXECINSTR)
    helper_text = b.section(".text", b"H" * 8, SHF_ALLOC | SHF_EXECINSTR)
    global_helper_text = b.section(".text", b"G" * 8, SHF_ALLOC | SHF_EXECINSTR)
    # Locals first, as MWCC lists them
    b.symbol("$t", dead_text, kind=STT_FUNC)
    b.symbol("$t", live_text, kind=STT_FUNC)
    counter = b.symbol("sCounter", bss, size=4)
    pool_live = b.symbol("@pool_live", pool, size=4)
    pool_dead = b.symbol("@pool_dead", pool, value=4, size=4)
    shared_symbol = b.symbol("@shared", shared, size=7)
    dead_symbol = b.symbol("@dead_one", dead_string, size=9)
    live_symbol = b.symbol("@live_one", live_string, size=9)
    b.symbol("@EX@DeadFunc", dead_index, size=12)
    b.symbol("@EX@LiveFunc", live_index, size=12)
    helper = b.symbol("Helper", helper_text, size=8, kind=STT_FUNC)
    dead_func = b.symbol("DeadFunc", dead_text, size=24, bind=STB_GLOBAL, kind=STT_FUNC)
    live_func = b.symbol("LiveFunc", live_text, size=24, bind=STB_GLOBAL, kind=STT_FUNC)
    global_helper = b.symbol("GlobalHelper", global_helper_text, size=8, bind=STB_GLOBAL, kind=STT_FUNC)
    use = b.symbol("Use", UNDEFINED, bind=STB_GLOBAL, kind=STT_NOTYPE)
    dead_only = b.symbol("DeadOnlyExtern", UNDEFINED, bind=STB_GLOBAL, kind=STT_NOTYPE)
    b.relocate(debug, 0, dead_func)
    b.relocate(debug, 4, live_func)
    b.relocate(dead_index, 0, dead_func)
    b.relocate(live_index, 0, live_func)
    for offset, symbol in ((0, use), (4, dead_only), (8, helper), (12, global_helper)):
        b.relocate(dead_text, offset, symbol, R_ARM_THM_CALL, -4)
    for offset, symbol in ((16, shared_symbol), (20, dead_symbol)):
        b.relocate(dead_text, offset, symbol)
    b.relocate(dead_text, 0, pool_dead)
    b.relocate(live_text, 0, use, R_ARM_THM_CALL, -4)
    if live_calls_dead:
        b.relocate(live_text, 4, dead_func, R_ARM_THM_CALL, -4)
    for offset, symbol in ((12, counter), (16, shared_symbol), (20, live_symbol), (8, pool_live)):
        b.relocate(live_text, offset, symbol)
    return b.build()


def kept_contents(obj):
    """The allocated sections, each as its type and the symbols it defines with its contents."""
    return sorted((s.name, tuple(sorted(sym.name for sym in obj.symbols if sym.shndx == i and sym.name != "$t")),
                   s.body, s.size) for i, s in enumerate(obj.sections) if s.flags & SHF_ALLOC)


class StripTests(unittest.TestCase):
    def setUp(self):
        self.original = fixture()
        self.stripped = deadstrip.strip(self.original, ["DeadFunc"])
        self.before = read(self.original)
        self.after = read(self.stripped.data)

    def names(self, obj):
        return [s.name for s in obj.symbols]

    def test_fixture_round_trips(self):
        # The writer lays an unchanged object out as MWCC did, so parsing and writing it changes nothing
        self.assertEqual(deadstrip.serialize(deadstrip.parse(self.original)), self.original)

    def test_nothing_listed_copies_the_object(self):
        self.assertEqual(deadstrip.strip(self.original, []).data, self.original)

    def test_dead_function_and_its_own_literal_dropped(self):
        names = self.names(self.after)
        for name in ("DeadFunc", "@dead_one", "@EX@DeadFunc"):
            self.assertNotIn(name, names)
        self.assertFalse(any(s.body == b"D" * 24 or s.body == b"dead_one\0" for s in self.after.sections))
        self.assertEqual(self.stripped.symbols, {"DeadFunc", "@dead_one", "@EX@DeadFunc", "Helper"})

    def test_shared_literal_and_pooled_data_kept(self):
        names = self.names(self.after)
        for name in ("@shared", "@pool_live", "@pool_dead", "@live_one", "sCounter"):
            self.assertIn(name, names)
        self.assertEqual(self.after.sections[defining(self.after, "@shared")].body, b"shared\0")
        self.assertEqual(self.after.sections[defining(self.after, "@pool_dead")].body, b"LIVEDEAD")

    def test_static_helper_goes_and_global_helper_stays(self):
        # A static function only dead code calls is dead too; a global may have callers in other files
        names = self.names(self.after)
        self.assertNotIn("Helper", names)
        self.assertIn("GlobalHelper", names)

    def test_undefined_symbol_only_dead_code_called_is_dropped(self):
        names = self.names(self.after)
        self.assertNotIn("DeadOnlyExtern", names)
        self.assertIn("Use", names)
        self.assertIn("dropped undefined DeadOnlyExtern", self.stripped.report)
        self.assertEqual(self.stripped.undefined, {"DeadOnlyExtern"})

    def test_everything_else_unchanged(self):
        dropped = {"DeadFunc", "@dead_one", "@EX@DeadFunc", "Helper"}
        expected = [entry for entry in kept_contents(self.before) if not set(entry[1]) & dropped]
        self.assertEqual(kept_contents(self.after), expected)
        bss = self.after.sections[defining(self.after, "sCounter")]
        self.assertEqual((bss.type, bss.size), (SHT_NOBITS, 8))
        for name in (".comment", ".debug_info", ".strtab", ".shstrtab"):
            self.assertEqual(next(s.body for s in self.after.sections if s.name == name),
                             next(s.body for s in self.before.sections if s.name == name))

    def test_exception_index_dropped_with_its_function(self):
        indices = [i for i, s in enumerate(self.after.sections) if s.name == ".exceptix"]
        self.assertEqual(len(indices), 1)
        self.assertEqual(self.after.relocations[indices[0]], [(0, "LiveFunc", R_ARM_ABS32, 0)])

    def test_relocations_remapped(self):
        live = defining(self.after, "LiveFunc")
        self.assertEqual(self.after.sections[live].body, b"L" * 24)
        self.assertEqual(self.after.relocations[live], self.before.relocations[defining(self.before, "LiveFunc")])
        for _, name, _, _ in self.after.relocations[live]:
            if name != "Use":
                self.assertEqual(self.after.sections[defining(self.after, name)].body,
                                 self.before.sections[defining(self.before, name)].body)
        debug = next(i for i, s in enumerate(self.after.sections) if s.name == ".debug_info")
        self.assertEqual(self.after.relocations[debug], [(4, "LiveFunc", R_ARM_ABS32, 0)])
        # Only the relocation sections of what stays are left, each still after its section
        rela = [s for s in self.after.sections if s.type == SHT_RELA]
        self.assertEqual(len(rela), 3)
        for section in rela:
            self.assertEqual(self.after.sections.index(section), section.info + 1)
        symtab = self.after.sections[self.after.symtab]
        self.assertEqual(self.after.sections[symtab.link].name, ".strtab")

    def test_locals_first_symbol_order_kept(self):
        # The dropped sections' symbols go, DeadFunc's mapping symbol with them, and the callee only it had
        dropped = {defining(self.before, name) for name in ("DeadFunc", "@dead_one", "@EX@DeadFunc", "Helper")}
        expected = [s.name for s in self.before.symbols if s.shndx not in dropped and s.name != "DeadOnlyExtern"]
        self.assertEqual(self.names(self.after), expected)
        self.assertEqual(self.names(self.after)[:3], ["", "$t", "sCounter"])
        first_global = self.after.sections[self.after.symtab].info
        self.assertTrue(all(s.bind == STB_LOCAL for s in self.after.symbols[:first_global]))
        self.assertTrue(all(s.bind != STB_LOCAL for s in self.after.symbols[first_global:]))
        self.assertEqual(self.after.symbols[first_global].name, "LiveFunc")
        self.assertEqual(self.names(self.after)[0], "")

    def test_output_round_trips(self):
        self.assertEqual(deadstrip.serialize(deadstrip.parse(self.stripped.data)), self.stripped.data)


class StripErrorTests(unittest.TestCase):
    def test_kept_section_referring_to_listed_symbol_is_an_error(self):
        with self.assertRaisesRegex(deadstrip.DeadStripError,
                                    r"stays refers to a listed symbol: \.text \(LiveFunc\) -> \.text \(DeadFunc\)"):
            deadstrip.strip(fixture(live_calls_dead=True), ["DeadFunc"])

    def test_symbol_not_defined_here_is_an_error(self):
        with self.assertRaisesRegex(deadstrip.DeadStripError, "Use is not defined"):
            deadstrip.strip(fixture(), ["Use"])
        with self.assertRaisesRegex(deadstrip.DeadStripError, "Missing is not defined"):
            deadstrip.strip(fixture(), ["Missing"])

    def test_section_shared_with_an_unlisted_global_is_an_error(self):
        b = ObjectBuilder()
        text = b.section(".text", bytes(8), SHF_ALLOC | SHF_EXECINSTR)
        b.symbol("First", text, size=4, bind=STB_GLOBAL, kind=STT_FUNC)
        b.symbol("Second", text, value=4, size=4, bind=STB_GLOBAL, kind=STT_FUNC)
        with self.assertRaisesRegex(deadstrip.DeadStripError, "First shares its section with Second"):
            deadstrip.strip(b.build(), ["First"])

    def test_not_an_object_is_an_error(self):
        with self.assertRaisesRegex(deadstrip.DeadStripError, "not an ELF file"):
            deadstrip.strip(b"not an object", ["DeadFunc"])


def caller(*callees, defines=()):
    """Another file's object, which calls each of callees and defines the functions named in defines."""
    b = ObjectBuilder()
    text = b.section(".text", bytes(4 * max(len(callees), 1)), SHF_ALLOC | SHF_EXECINSTR)
    for name in defines:
        b.symbol(name, text, size=4, bind=STB_GLOBAL, kind=STT_FUNC)
    for offset, name in enumerate(callees):
        b.relocate(text, 4 * offset, b.symbol(name, UNDEFINED, bind=STB_GLOBAL, kind=STT_NOTYPE), R_ARM_THM_CALL, -4)
    return b.build()


class ReferenceTests(unittest.TestCase):
    """The check that no other object of the link refers to what a strip removed."""

    def setUp(self):
        self.stripped = deadstrip.strip(fixture(), ["DeadFunc"])
        self.strips = [("src/file.c", ["DeadFunc"], self.stripped)]

    def check(self, *objects, known=frozenset()):
        return deadstrip.check_references(self.strips, [("src/file.c", self.stripped.data), *objects], set(known))

    def test_symbol_names(self):
        self.assertEqual(deadstrip.symbol_names(fixture()),
                         ({"DeadFunc", "LiveFunc", "GlobalHelper"}, {"Use", "DeadOnlyExtern"}))
        self.assertEqual(deadstrip.symbol_names(self.stripped.data), ({"LiveFunc", "GlobalHelper"}, {"Use"}))

    def test_call_to_a_listed_symbol_is_an_error(self):
        errors = self.check(("src/other.c", caller("DeadFunc", "LiveFunc")))
        self.assertEqual(errors, ["src/other.c: refers to DeadFunc, which config/deadstrip.toml lists as dead code of "
                                  "src/file.c"])

    def test_call_to_what_only_dropped_code_called_and_nothing_defines_is_an_error(self):
        errors = self.check(("src/other.c", caller("DeadOnlyExtern")))
        self.assertEqual(errors, ["src/other.c: refers to DeadOnlyExtern, which nothing defines: only the code "
                                  "dead-stripped from src/file.c called it"])

    def test_what_the_rom_or_another_object_defines_is_not_an_error(self):
        self.assertEqual(self.check(("src/other.c", caller("DeadOnlyExtern")), known={"DeadOnlyExtern"}), [])
        self.assertEqual(self.check(("src/other.c", caller("DeadOnlyExtern")),
                                    ("delinks/extern.o", caller(defines=["DeadOnlyExtern"]))), [])

    def test_other_calls_are_not_errors(self):
        self.assertEqual(self.check(("src/other.c", caller("Use", "LiveFunc", "GlobalHelper"))), [])
        self.assertEqual(deadstrip.check_references([], [("src/other.c", caller("DeadFunc"))], set()), [])

    def test_rom_symbol_names(self):
        with tempfile.TemporaryDirectory() as temp:
            config = Path(temp)
            (config / "arm9/overlays/ov001").mkdir(parents=True)
            (config / "arm9/symbols.txt").write_text("Main kind:function(thumb,size=0x4) addr:0x02000000\n\n")
            (config / "arm9/overlays/ov001/symbols.txt").write_text("Other kind:data(any) addr:0x02100000\n")
            self.assertEqual(deadstrip.rom_symbol_names(config), {"Main", "Other"})

    def test_command_line(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp).resolve()
            (root / "src").mkdir()
            (root / "src/file.c").write_text("")
            manifest = root / "deadstrip.toml"
            manifest.write_text('[deadstrip]\n"src/file.c" = ["DeadFunc"]\n')
            (root / "unstripped/src").mkdir(parents=True)
            (root / "unstripped/src/file.o").write_bytes(fixture())
            (root / "file.o").write_bytes(self.stripped.data)
            (root / "other@1.o").write_bytes(caller("Use"))
            # One object to a line, quoted as ninja quotes a path with characters the shell might take
            (root / "objects.txt").write_text(f"{root / 'file.o'}\n'{root / 'other@1.o'}'\n")
            (root / "config").mkdir()
            stamp = root / "stamps/deadstrip.ok"
            arguments = ["check_deadstrip.py", "--config", str(root / "config"), "--unstripped",
                         str(root / "unstripped"), "--manifest", str(manifest), str(stamp), f"@{root / 'objects.txt'}"]
            with patch.object(check_deadstrip, "ROOT", root), patch.object(sys, "argv", arguments):
                self.assertEqual(check_deadstrip.main(), 0)
            self.assertTrue(stamp.exists())
            (root / "other@1.o").write_bytes(caller("DeadFunc"))
            with (patch.object(check_deadstrip, "ROOT", root), patch.object(sys, "argv", arguments),
                  redirect_stderr(io.StringIO()) as errors):
                self.assertEqual(check_deadstrip.main(), 1)
            self.assertIn("other@1.o: refers to DeadFunc", errors.getvalue())
            # A failed check leaves no stamp, so the next build runs it again
            self.assertFalse(stamp.exists())


class ManifestTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name).resolve()
        (self.root / "src").mkdir()
        (self.root / "src/file.c").write_text("")
        self.manifest = self.root / "deadstrip.toml"

    def load(self, text):
        self.manifest.write_text(text)
        return deadstrip.load_manifest(self.manifest, self.root)

    def test_entries(self):
        self.assertEqual(self.load('[deadstrip]\n"src/file.c" = ["DeadFunc", "sDeadTable"]\n'),
                         {"src/file.c": ["DeadFunc", "sDeadTable"]})
        self.assertEqual(deadstrip.dead_symbols(Path("src/file.c"), self.manifest, self.root),
                         ["DeadFunc", "sDeadTable"])
        self.assertEqual(deadstrip.dead_symbols(self.root / "src/file.c", self.manifest, self.root),
                         ["DeadFunc", "sDeadTable"])
        self.assertEqual(deadstrip.dead_symbols(Path("src/other.c"), self.manifest, self.root), [])
        self.assertEqual(self.load('[deadstrip]\n"src/file.c" = []\n'), {"src/file.c": []})

    def test_invalid_entries(self):
        for text in ('[deadstrip]\n"src/missing.c" = []\n', '[deadstrip]\n"src/../src/file.c" = []\n',
                     '[deadstrip]\n"./src/file.c" = []\n', f'[deadstrip]\n"{self.root}/src/file.c" = []\n',
                     '[deadstrip]\n"src/file.c" = "DeadFunc"\n', '[deadstrip]\n"src/file.c" = ["Dead Func"]\n',
                     '[deadstrip]\n"src/file.c" = ["DeadFunc", "DeadFunc"]\n',
                     '[deadstrip]\n"src/file.c" = ["DeadFunc\\n"]\n', '[other]\n', 'deadstrip = 1\n'):
            with self.subTest(text=text), self.assertRaises(deadstrip.DeadStripError):
                self.load(text)

    def test_project_manifest_is_valid(self):
        deadstrip.load_manifest()

    def test_command_line(self):
        self.manifest.write_text('[deadstrip]\n"src/file.c" = ["DeadFunc"]\n')
        source, output = self.root / "file.o", self.root / "out/file.o"
        source.write_bytes(fixture())
        arguments = ["deadstrip.py", "--manifest", str(self.manifest), "--source", "src/file.c", str(source),
                     str(output)]
        with patch.object(deadstrip, "ROOT", self.root), patch.object(sys, "argv", arguments):
            self.assertEqual(deadstrip.main(), 0)
        self.assertEqual(output.read_bytes(), deadstrip.strip(fixture(), ["DeadFunc"]).data)
        arguments[4] = "src/other.c"
        with (patch.object(deadstrip, "ROOT", self.root), patch.object(sys, "argv", arguments),
              redirect_stderr(io.StringIO()) as errors):
            self.assertEqual(deadstrip.main(), 1)
        self.assertIn("is not listed", errors.getvalue())
        # A failed strip leaves no old object for the link or objdiff to take
        self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()

#!/usr/bin/env python3
"""Writes the trap stubs that let the host build link before every DS function and data object exists as C.

    gen_stubs.py --version b2_us --out build/port/gen/stubs.c --objects-file OBJECTS.txt [--nm NM] [--compiler CC]

It reads the symbols that the given objects define and the ones they reference (the objects are listed one a line in
OBJECTS.txt, or given with --objects OBJ...: a Windows command line can't hold their some 1,600 paths), and for each
referenced symbol that no object defines and the host's C library doesn't provide, but that the ROM has
(config/<version>/arm9/**/symbols.txt):
  - a function gets a stub that calls port_trap_function(), which names it, its caller and the backtrace, then stops;
  - a data object gets a placeholder in a section that port_trap_protect_data() makes inaccessible at startup, so the
    first access is reported by name rather than reading zeros.
A referenced symbol that is in neither the objects, the host's C library nor the ROM is left for the linker to report.
The list shrinks as upstream writes C and as port/hal implements the platform layer. A summary goes to stderr. The
generated file takes its page alignment and section from the host's headers (port/hal/host/host.h), so that the
target, not this script's interpreter, decides them.

What the host's C library provides is what this process can find in it through ctypes (macOS and Linux), and, with
--compiler (Linux and Windows), what the libraries that the compiler driver links into every program define, read
with --nm: glibc keeps atexit in libc_nonshared.a, linked into each program and hidden from ctypes, and Windows'
Python has no ctypes view of its process's C library. A function of standard C is never stubbed: if no lookup finds
one the objects reference, the script fails rather than let a trap stand in for the C library (the platform layer
calls it too: backup.c's atexit). And since the decomp's wide strings are 16-bit (-fshort-wchar) while only Windows'
C library's wchar_t is, a wide-character function the objects call is a warning: the build keeps the compiler from
turning the decomp's loops into such calls (-fno-builtin-wcslen, port/CMakeLists.txt).
"""
import argparse
import ctypes
import keyword
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SYMBOL_LINE = re.compile(r"^(\S+) kind:(\w+)(?:\(([^)]*)\))? addr:0x([0-9a-fA-F]+)")
IDENTIFIER = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
# Placeholders get room beyond the DS size, since host layouts of structs with pointers are larger
PLACEHOLDER_SCALE = 2
PLACEHOLDER_MAX = 0x10000
# nm prints Mach-O's symbols with a leading underscore; the objects are this machine's, which builds for itself
MACHO = sys.platform == "darwin"
# A quoted argument of the compiler driver's -### output, with its backslashes and quotes escaped
DRIVER_ARGUMENT = re.compile(r'"((?:[^"\\]|\\.)*)"')

# The functions of standard C (C11's library, but for its complex numbers, threads and Annex K), which every hosted C
# library has, so that none is ever stubbed, though the ROM has MSL's of the same names. The wide-character ones take
# the host's wchar_t, which is 16-bit as the decomp's is on Windows only
MATH_C = """acos asin atan atan2 cos sin tan acosh asinh atanh cosh sinh tanh exp exp2 expm1 frexp ilogb ldexp log
    log10 log1p log2 logb modf scalbn scalbln cbrt fabs hypot pow sqrt erf erfc lgamma tgamma ceil floor nearbyint rint
    lrint llrint round lround llround trunc fmod remainder remquo copysign nan nextafter nexttoward fdim fmax fmin
    fma""".split()
WIDE_C = set("""fwprintf fwscanf swprintf swscanf vfwprintf vfwscanf vswprintf vswscanf vwprintf vwscanf wprintf wscanf
    fgetwc fgetws fputwc fputws fwide getwc getwchar putwc putwchar ungetwc wcstod wcstof wcstold wcstol wcstoll
    wcstoul wcstoull wcscpy wcsncpy wmemcpy wmemmove wcscat wcsncat wcscmp wcscoll wcsncmp wcsxfrm wmemcmp wcschr
    wcscspn wcspbrk wcsrchr wcsspn wcsstr wcstok wmemchr wcslen wmemset wcsftime btowc wctob mbsinit mbrlen mbrtowc
    wcrtomb mbsrtowcs wcsrtombs mbtowc wctomb mbstowcs wcstombs iswalnum iswalpha iswblank iswcntrl iswdigit iswgraph
    iswlower iswprint iswpunct iswspace iswupper iswxdigit iswctype wctype towlower towupper towctrans wctrans
    wcstoimax wcstoumax""".split())
STANDARD_C = WIDE_C | {name + suffix for name in MATH_C for suffix in ("", "f", "l")} | set("""isalnum isalpha
    isblank iscntrl isdigit isgraph islower isprint ispunct isspace isupper isxdigit tolower toupper feclearexcept
    fegetexceptflag feraiseexcept fesetexceptflag fetestexcept fegetround fesetround fegetenv feholdexcept fesetenv
    feupdateenv imaxabs imaxdiv strtoimax strtoumax setlocale localeconv longjmp signal raise remove rename tmpfile
    tmpnam fclose fflush fopen freopen setbuf setvbuf fprintf fscanf printf scanf snprintf sprintf sscanf vfprintf
    vfscanf vprintf vscanf vsnprintf vsprintf vsscanf fgetc fgets fputc fputs getc getchar gets putc putchar puts
    ungetc fread fwrite fgetpos fseek fsetpos ftell rewind clearerr feof ferror perror atof atoi atol atoll strtod
    strtof strtold strtol strtoll strtoul strtoull rand srand aligned_alloc calloc free malloc realloc abort atexit
    at_quick_exit exit _Exit getenv quick_exit system bsearch qsort abs labs llabs div ldiv lldiv mblen memcpy memmove
    strcpy strncpy strcat strncat memcmp strcmp strcoll strncmp strxfrm memchr strchr strcspn strpbrk strrchr strspn
    strstr strtok memset strerror strlen clock difftime mktime time asctime ctime gmtime localtime strftime
    timespec_get""".split())


def load_ds_symbols(version: str) -> dict[str, dict]:
    """Every function and data symbol of the version's ARM9 modules, with its module, address and size. A size the
    config doesn't record is the distance to the module's next symbol."""
    base = ROOT / "config" / version / "arm9"
    symbols = {}
    for path in sorted(base.rglob("symbols.txt")):
        module = "main" if path.parent == base else path.parent.name
        entries = []
        for line in path.read_text().splitlines():
            match = SYMBOL_LINE.match(line)
            if not match:
                continue
            name, kind, params, address = match.groups()
            size = None
            for param in (params or "").split(","):
                if param.startswith("size="):
                    size = int(param[5:], 0)
            entries.append((int(address, 16), name, kind, size))
        entries.sort()
        for index, (address, name, kind, size) in enumerate(entries):
            if kind == "label":
                continue
            if size is None:
                following = next((a for a, *_ in entries[index + 1:] if a > address), None)
                size = following - address if following is not None else 4
            symbols.setdefault(name, {"module": module, "address": address, "size": size,
                                      "function": kind == "function"})
    return symbols


def read_object_symbols(objects: list[str], nm: str) -> tuple[set[str], set[str]]:
    """The global symbols the objects define and the ones they reference without defining."""
    defined, undefined = set(), set()
    for start in range(0, len(objects), 200):
        output = subprocess.run([nm, "-g", "-P", "-A", *objects[start:start + 200]], capture_output=True,
                                text=True, check=True).stdout
        for line in output.splitlines():
            # "<object>: <name> <type> [<value> <size>]"
            _, _, rest = line.rpartition(": ")
            fields = rest.split()
            if len(fields) < 2:
                continue
            name, kind = fields[0], fields[1]
            if MACHO and name.startswith("_"):
                name = name[1:]
            (undefined if kind == "U" else defined).add(name)
    return defined, undefined - defined


def process_library():
    """This process's C library through ctypes, where ctypes has a view of it (macOS and Linux), else None."""
    try:
        return ctypes.CDLL(None)
    except (OSError, TypeError):
        return None


def host_provides(name: str, process, libraries: set[str]) -> bool:
    """Whether the host's C library has name: in the driver's libraries, or in this process through ctypes."""
    if name in libraries:
        return True
    if process is None:
        return False
    try:
        getattr(process, name)
        return True
    except AttributeError:
        return False


def driver_libraries(compiler: str) -> list[str]:
    """The libraries the compiler driver links into every program (the C runtime, compiler-rt, the system's import
    libraries), from the link command it prints for a program of one empty object, without running it."""
    with tempfile.TemporaryDirectory() as tmp:
        probe = Path(tmp) / "probe.o"
        probe.write_bytes(b"")
        result = subprocess.run([compiler, "-###", str(probe), "-o", str(Path(tmp) / "probe.exe")],
                                capture_output=True, text=True)
    link = next((line for line in reversed(result.stderr.splitlines()) if "probe.o" in line), "")
    arguments = [re.sub(r"\\(.)", r"\1", match) for match in DRIVER_ARGUMENT.findall(link)]
    directories = [Path(argument[2:]) for argument in arguments if argument.startswith("-L")]
    libraries = []
    for argument in arguments:
        if argument.startswith("-l"):
            libraries += find_library(argument[2:], directories)
        elif argument.endswith((".a", ".lib")) and Path(argument).is_file():
            libraries.append(argument)
    if not libraries:
        sys.exit(f"gen_stubs: {compiler} -### named no library of the C runtime: {link or result.stderr}")
    return list(dict.fromkeys(libraries))


def find_library(name: str, directories: list[Path]) -> list[str]:
    """The archives the linker takes for -lname, searching the directories in order as it does: an import library or a
    static one, or the archives that a shared library's linker script adds (glibc's libc.so is one: libc.so.6, whose
    exports ctypes sees, and libc_nonshared.a, whose atexit it doesn't). A shared library itself adds nothing here."""
    for directory in directories:
        for candidate in (f"lib{name}.dll.a", f"lib{name}.so", f"lib{name}.a", f"{name}.lib"):
            path = directory / candidate
            if not path.is_file():
                continue
            if candidate.endswith(".so"):
                return script_archives(path, directories)
            return [str(path)]
    return []


def script_archives(path: Path, directories: list[Path]) -> list[str]:
    """The archives a linker script names (GROUP ( ... libc_nonshared.a ... )), or none for a shared library."""
    with open(path, "rb") as file:
        if file.read(4) == b"\x7fELF":
            return []
    archives = []
    for token in re.findall(r"[^\s()]+\.a\b", path.read_text(errors="replace")):
        archive = Path(token.lstrip("="))
        if not archive.is_absolute():
            archive = next((directory / token for directory in directories if (directory / token).is_file()), archive)
        if archive.is_file():
            archives.append(str(archive))
    return archives


def library_symbols(libraries: list[str], nm: str) -> set[str]:
    """Every global symbol the libraries define, an import library's functions by their own names too."""
    names = set()
    output = subprocess.run([nm, "-g", "-P", "-A", "--defined-only", *libraries], capture_output=True, text=True,
                            check=True).stdout
    for line in output.splitlines():
        _, _, rest = line.rpartition(": ")
        fields = rest.split()
        if fields:
            names.add(fields[0].removeprefix("__imp_"))
    return names


def c_string(text: str) -> str:
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def write_stubs(out: Path, functions: list[tuple[str, dict]], data: list[tuple[str, dict]]):
    lines = [
        "// Generated by port/tools/gen_stubs.py from the objects of the host build; don't edit.",
        "#include \"hal/host/host.h\"",
        "#include \"hal/trap.h\"",
        "",
        "// The placeholders' own section, whose paddings start and end a page of the host's (port/hal/host/host.h)",
        "#ifdef __APPLE__",
        "#define PORT_TRAP_SECTION \"__DATA,__porttrap\"",
        "#else",
        "#define PORT_TRAP_SECTION \".porttrap\"",
        "#endif",
        "#define PORT_TRAP_DATA __attribute__((section(PORT_TRAP_SECTION), aligned(16), used))",
        "#define PORT_TRAP_PAGE __attribute__((section(PORT_TRAP_SECTION), aligned(PORT_HOST_PAGE_ALIGNMENT), used))",
        "",
    ]
    for name, info in functions:
        lines.append(f"static const PortTrapSymbol s_{name} = {{{c_string(name)}, {info['address']:#010x}, "
                     f"{info['size']:#x}, {c_string(info['module'])}}};")
        lines.append(f"void {name}(void) {{ port_trap_function(&s_{name}, __builtin_return_address(0)); }}")
    lines += ["", "PORT_TRAP_PAGE unsigned char port_trap_padding_begin[PORT_HOST_PAGE_ALIGNMENT];"]
    for name, info in data:
        size = min(max(info["size"], 4) * PLACEHOLDER_SCALE, PLACEHOLDER_MAX)
        lines.append(f"PORT_TRAP_DATA unsigned char {name}[{size:#x}];")
    lines += ["PORT_TRAP_PAGE unsigned char port_trap_padding_end[PORT_HOST_PAGE_ALIGNMENT];", ""]
    lines.append("const PortTrapSymbol port_trap_data_symbols[] = {")
    for name, info in data:
        size = min(max(info["size"], 4) * PLACEHOLDER_SCALE, PLACEHOLDER_MAX)
        lines.append(f"    {{{c_string(name)}, {info['address']:#010x}, {size:#x}, {c_string(info['module'])}}},")
    if not data:
        lines.append("    {0},")
    lines.append("};")
    lines.append("void *const port_trap_data_addresses[] = {")
    lines += [f"    {name}," for name, _ in data] or ["    0,"]
    lines.append("};")
    lines.append(f"const size_t port_trap_data_count = {len(data)};")
    lines.append("unsigned char *const port_trap_data_begin = port_trap_padding_begin;")
    lines.append("unsigned char *const port_trap_data_end = port_trap_padding_end + PORT_HOST_PAGE_ALIGNMENT;")
    text = "\n".join(lines) + "\n"
    if not out.exists() or out.read_text() != text:
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text(text)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", required=True, choices=["b2_us", "w2_us"])
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--objects", nargs="+", default=[])
    parser.add_argument("--objects-file", type=Path, help="a file that lists the objects, one a line")
    parser.add_argument("--nm", default="nm", help="the nm to read the objects with")
    parser.add_argument("--compiler",
                        help="the compiler driver whose libraries provide the C library too (Linux and Windows)")
    args = parser.parse_args()

    objects = list(args.objects)
    if args.objects_file is not None:
        objects += [line for line in args.objects_file.read_text().splitlines() if line]
    if not objects:
        sys.exit("gen_stubs: no objects")
    ds = load_ds_symbols(args.version)
    defined, undefined = read_object_symbols(objects, args.nm)
    process = process_library()
    libraries = library_symbols(driver_libraries(args.compiler), args.nm) if args.compiler is not None else set()
    if process is None and not libraries:
        sys.exit("gen_stubs: no view of the host's C library: give --compiler and --nm (Windows)")
    functions, data, unknown, provided = [], [], [], []
    for name in sorted(undefined):
        info = ds.get(name)
        if info is None or not IDENTIFIER.match(name) or keyword.iskeyword(name):
            # A Windows object names a library's function by its import, __imp_NAME
            library_name = name.removeprefix("__imp_")
            if (not host_provides(library_name, process, libraries) and
                    not library_name.startswith(("SDL_", "port_"))):
                unknown.append(name)
            continue
        if host_provides(name, process, libraries):
            provided.append(name)
            continue
        (functions if info["function"] else data).append((name, info))
    standard = sorted(name for name, _ in functions + data if name in STANDARD_C)
    if standard:
        sys.exit(f"gen_stubs: the objects reference {', '.join(standard)}, of standard C, which no lookup found in the "
                 "host's C library; a trap stub would stand in for it (give --compiler and --nm, or see why the "
                 "lookup misses it)")
    wide = sorted({name.removeprefix("__imp_") for name in undefined} & WIDE_C)
    if wide:
        print(f"gen_stubs: warning: the objects call the host C library's {', '.join(wide)}, whose wchar_t only "
              "Windows' shares with the decomp's 16-bit wide strings (-fshort-wchar); -fno-builtin-NAME in "
              "port/CMakeLists.txt keeps the compiler from turning a loop into such a call", file=sys.stderr)
    data.sort(key=lambda item: (item[1]["module"], item[1]["address"]))
    write_stubs(args.out, functions, data)
    print(f"gen_stubs: {len(functions)} function stubs, {len(data)} data placeholders, "
          f"{len(defined)} symbols defined", file=sys.stderr)
    # The ROM's functions the host's C library stands in for (memcpy, sprintf and the like): a name a host library
    # happens to share with a game function would show here
    print(f"gen_stubs: the host's C library provides {len(provided)} of the ROM's symbols: {', '.join(provided)}",
          file=sys.stderr)
    if unknown:
        print(f"gen_stubs: {len(unknown)} referenced symbols are in neither the objects, the host's C library nor "
              f"the ROM: {', '.join(unknown[:40])}{' ...' if len(unknown) > 40 else ''}", file=sys.stderr)


if __name__ == "__main__":
    main()

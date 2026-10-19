#!/usr/bin/env python3
"""Find functions the DS build compiled into more than one place, so one matched copy can unlock its duplicates.

The build links the same helpers into several overlays and libraries (libraries twice, helpers copied into overlays),
and MWCC emits the same bytes for the same source. A group of byte-identical functions is therefore either a free
match (one copy is already matched, so its C proves the others) or a group worth solving once and matching many times.

Method. Every primary-version function of size >= `--min-size` is fingerprinted with its masked bytes (relocated
words zeroed, `masked_function_bytes()` of `tools/decomp/dsd_config.py`, as `version_map.py` uses), and functions
are grouped by those bytes. The other version's functions are added when `version_map.py` marks their partner
`different`, since no C currently emits those bytes. Every index (functions by module, relocation references, the
delinked source files and the gaps between them) is built once, and a function's relocations are found by bisecting
the module's sorted relocation addresses, so the run costs one pass over the functions, not a scan per function.

Labels. A group's relocation targets are compared with the Thumb bit cleared, because a Thumb function pointer
relocates to its function's address with bit 0 set:

  identical            the copies relocate the same words to the same addresses;
  same via duplicates  the copies differ only in relocating to functions that are themselves one duplicate group,
                       so one written copy of that inner group proves them the same code;
  same shape           anything else: the relocated words point elsewhere, so the same instruction shape only.

Emission. MWCC drops a `static` function nothing in its file references (docs/decompiling.md), so an unmatched copy
whose only references are inside its own delinked source file needs those in-file callers written as well, up to a
function that is global or that nothing references. Each unmatched copy's row says whether it is `global` (referenced
from outside its file, so it is emitted as soon as it is written), `global?` (every outside reference comes through a
relocation naming more than one candidate module, so which one holds the target is not recorded; check by hand),
`static` (needs the callers listed), `static?` (every reference is inside the unassigned range it sits in, which holds
several original files, so its file boundary has to be found first and no callers are guessed), or `unreferenced`
(nothing calls it, so it has to be written as global). References come from `relocs.txt`; a pointer dsd did not
record is not seen.

Output. TSV on stdout, or `-o FILE`. One row per group, free matches first (by how many functions they unlock), then
unmatched groups by copies x size, then the fully matched groups. A row is the group columns `group status label
copies size matched unmatched` followed by one seven-column block per copy: `version module address name
copy_matched copy_static copy_callers`. `copy_callers` is a `;`-separated list of `0xADDR:name`, the in-file callers
a `static` copy needs; a trailing `*` marks a function the walk stopped at (global, or unreferenced).

This reports only: the owner prefers shared functions (#315), and the convention is being settled there.
Near-duplicates are out of scope.
"""
import argparse
import bisect
import collections
import json
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.decomp.dsd_config import (  # noqa: E402
    load_modules,
    masked_function_bytes,
    parse_sections,
    reloc_module_names,
)


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MIN_SIZE = 0x10
LABELS = ("identical", "same via duplicates", "same shape")
FREE = "free"
UNMATCHED = "unmatched"
ALL_MATCHED = "all-matched"
GLOBAL = "global"
GLOBAL_AMBIGUOUS = "global?"
STATIC = "static"
STATIC_UNASSIGNED = "static?"
UNREFERENCED = "unreferenced"
BUCKETS = ("0x10", "0x11-0x1f", "0x20-0x3f", "0x40-0x7f", "0x80+")
HEADER = "\t".join(("group", "status", "label", "copies", "size", "matched", "unmatched",
                    "version", "module", "address", "name", "copy_matched", "copy_static", "copy_callers"))
COPY_COLUMNS = 7


@dataclass(frozen=True)
class Copy:
    """One compiled instance of a function: a version, a module, an address, and what the ROM has there."""
    version: str
    module: str
    addr: int
    name: str
    size: int
    matched: bool
    masked: bytes
    layout: tuple
    targets: tuple


@dataclass(frozen=True)
class Emission:
    """What has to be written for an unemitted copy to reach the object.

    `kind` is `global`, `global?`, `static`, `static?` or `unreferenced`; `callers` are the in-file functions that
    must be written, in walk order; `stops` are the functions the walk stopped at, which are global or referenced by
    nothing.
    """
    kind: str
    callers: tuple
    stops: tuple


@dataclass
class Group:
    """Copies of one function, its label, and the emission work of its unmatched copies."""
    copies: list
    label: str
    status: str
    emissions: dict = field(default_factory=dict)


def matched_count(copies) -> int:
    return sum(1 for copy in copies if copy.matched)


def group_status(copies) -> str:
    """A group with some matched copies is a free match; one with none is work; a fully matched one is neither."""
    matched = matched_count(copies)
    if matched == 0:
        return UNMATCHED
    return ALL_MATCHED if matched == len(copies) else FREE


def reloc_slice(module, function) -> list:
    """A function's relocation source addresses, by bisecting the module's sorted list."""
    first = bisect.bisect_left(module.reloc_sources, function.addr)
    last = bisect.bisect_left(module.reloc_sources, function.addr + (function.size or 0))
    return module.reloc_sources[first:last]


def reloc_layout(module, function) -> tuple:
    """Where a function relocates, as (offset, kind): copies of one C share it."""
    return tuple((source - function.addr, module.relocs[source][0]) for source in reloc_slice(module, function))


def reloc_targets(module, function) -> tuple:
    """Every relocated word of a function, as (offset, kind, target module string, target address)."""
    return tuple((source - function.addr, kind, target_module, target)
                 for source in reloc_slice(module, function)
                 for kind, target, target_module in (module.relocs[source],))


def module_copies(version, modules, matched, min_size) -> list:
    """Every function of size >= min_size in one version, as a Copy."""
    copies = []
    for name, module in modules.items():
        data = module.data()
        for function in module.functions():
            size = function.size or 0
            if size < min_size:
                continue
            copies.append(Copy(version, name, function.addr, function.name, size,
                               function.addr in matched.get(name, frozenset()),
                               masked_function_bytes(module, data, function),
                               reloc_layout(module, function), reloc_targets(module, function)))
    return copies


def version_map_copies(version, modules, text, min_size) -> list:
    """The other version's functions whose primary partner differs, so no C currently emits them.

    Only functions `version_map.py` marks `different` are added: a function byte-identical to its partner is already
    covered by that partner's copy.
    """
    by_addr = {name: {f.addr: f for f in module.functions()} for name, module in modules.items()}
    data = {}
    copies = []
    for line in text.splitlines()[1:]:
        fields = line.split("\t")
        if len(fields) != 6 or fields[5] != "different":
            continue
        module_name, other_addr = fields[0], int(fields[2], 16)
        function = by_addr.get(module_name, {}).get(other_addr)
        if function is None or (function.size or 0) < min_size:
            continue
        module = modules[module_name]
        data.setdefault(module_name, module.data())
        copies.append(Copy(version, module_name, function.addr, function.name, function.size or 0, False,
                           masked_function_bytes(module, data[module_name], function),
                           reloc_layout(module, function), reloc_targets(module, function)))
    return copies


def group_copies(copies) -> list:
    """Groups copies of two or more by their masked bytes, in a stable order."""
    by_bytes = collections.defaultdict(list)
    for copy in copies:
        by_bytes[copy.masked].append(copy)
    groups = [sorted(group, key=lambda copy: (copy.version, copy.module, copy.addr))
              for group in by_bytes.values() if len(group) >= 2]
    return sorted(groups, key=lambda group: (group[0].size, group[0].masked))


def group_ids(groups) -> dict:
    """(version, module, address) -> the index of the duplicate group holding it."""
    ids = {}
    for index, group in enumerate(groups):
        for copy in group:
            ids[(copy.version, copy.module, copy.addr)] = index
    return ids


def function_index(modules_by_version) -> dict:
    """(version, module) -> (sorted addresses, size by address, name by address)."""
    index = {}
    for version, modules in modules_by_version.items():
        for name, module in modules.items():
            functions = module.functions()
            index[(version, name)] = ([f.addr for f in functions],
                                      {f.addr: (f.size or 0) for f in functions},
                                      {f.addr: f.name for f in functions})
    return index


def make_resolver(functions):
    """Returns resolve(version, relocation module string, target address) -> (module, function address) or None.

    A relocation's `module:` is a group such as `main` or `overlays(11,12)`, so the owning module is whichever of
    those holds a function at, or containing, the target. The Thumb bit is cleared when the exact address resolves
    to nothing, since a Thumb function pointer targets its function's address with bit 0 set.
    """
    def resolve(version, module_string, addr):
        for attempt in (addr, addr & ~1):
            for name in reloc_module_names(module_string):
                entry = functions.get((version, name))
                if entry is None:
                    continue
                addrs, sizes = entry[0], entry[1]
                if attempt in sizes:
                    return name, attempt
                position = bisect.bisect_right(addrs, attempt) - 1
                if position >= 0 and addrs[position] <= attempt < addrs[position] + sizes[addrs[position]]:
                    return name, addrs[position]
        return None
    return resolve


def classify(group, ids, resolve) -> str:
    """Labels a group by its relocation targets: identical, same via duplicates, or same shape."""
    if len({copy.layout for copy in group}) != 1:
        return "same shape"
    targets = [copy.targets for copy in group]
    if all(same == targets[0] for same in targets):
        return "identical"
    for index in range(len(targets[0])):
        words = [words[index] for words in targets]
        if len({(word[2], word[3]) for word in words}) == 1:
            continue
        owners = set()
        for copy, word in zip(group, words):
            resolved = resolve(copy.version, word[2], word[3])
            owner = ids.get((copy.version, *resolved)) if resolved else None
            if owner is None:
                return "same shape"
            owners.add(owner)
        if len(owners) != 1:
            return "same shape"
    return "same via duplicates"


def delink_entries(module) -> list:
    """(section, start, end, source) for every delinks.txt entry of a module."""
    path = module.config_dir / "delinks.txt"
    if not path.exists():
        return []
    entries = []
    source = None
    for line in path.read_text().splitlines():
        stripped = line.strip()
        if not stripped:
            continue
        if not line[0].isspace() and stripped.endswith(":"):
            source = stripped[:-1]
            continue
        match = re.match(r"(\.\w+)\s+start:(0x[0-9a-f]+)\s+end:(0x[0-9a-f]+)", stripped)
        if match and source:
            entries.append((match.group(1), int(match.group(2), 16), int(match.group(3), 16), source))
    return entries


def module_scopes(module) -> tuple:
    """A module's .text as sorted (start, end, source or None): its delinked files and the gaps between them.

    A function the delinks have not covered yet is scoped to the unassigned range it sits in, which is the file it
    will be split into, or a part of it.
    """
    text = parse_sections(module.config_dir / "delinks.txt").get(".text")
    if text is None:
        return [], []
    files = sorted((start, end, source) for section, start, end, source in delink_entries(module) if section == ".text")
    entries = []
    cursor = text[0]
    for start, end, source in files:
        if start > cursor:
            entries.append((cursor, start, None))
        entries.append((max(start, cursor), end, source))
        cursor = max(cursor, end)
    if cursor < text[1]:
        entries.append((cursor, text[1], None))
    return [entry[0] for entry in entries], entries


def in_file(module, scope, site) -> bool:
    """Whether a reference site (source module, source address, kind, ambiguous) is inside a function's own file."""
    source_module, source_addr = site[:2]
    return source_module == module and scope is not None and scope[0] <= source_addr < scope[1]


class EmissionIndex:
    """Everything the emission walk needs, indexed once: functions, relocation references and file scopes."""

    def __init__(self, modules_by_version):
        self.functions = function_index(modules_by_version)
        self.references = collections.defaultdict(list)
        self.scopes = {}
        for version, modules in modules_by_version.items():
            for name, module in modules.items():
                for source, (kind, target, module_string) in module.relocs.items():
                    names = reloc_module_names(module_string)
                    ambiguous = len(names) > 1
                    for target_module in names:
                        self.references[(version, target_module, target)].append((name, source, kind, ambiguous))
                        # A Thumb function pointer targets the function's address with bit 0 set.
                        self.references[(version, target_module, target - 1)].append((name, source, kind, ambiguous))
            for name, module in modules.items():
                self.scopes[(version, name)] = module_scopes(module)

    def function_at(self, version, module, addr):
        """The function containing addr in a module, as (address, name), or None."""
        entry = self.functions.get((version, module))
        if entry is None:
            return None
        starts, sizes, names = entry
        position = bisect.bisect_right(starts, addr) - 1
        if position < 0:
            return None
        start = starts[position]
        return (start, names[start]) if addr < start + sizes[start] else None

    def scope(self, version, module, addr):
        """The file or unassigned range addr belongs to, as (start, end, source or None), or None."""
        starts, entries = self.scopes.get((version, module), ((), ()))
        if not entries:
            return None
        position = bisect.bisect_right(starts, addr) - 1
        if position < 0:
            return None
        entry = entries[position]
        return entry if entry[0] <= addr < entry[1] else None

    def emission(self, copy) -> Emission:
        """What has to be written for a copy of an unemitted function to be emitted.

        A copy in an unassigned range can only be called `static?`: every reference to it is inside the range, but the
        range holds several original files, so only the file boundary (the `decomp-file` skill's boundary step) tells
        which references are in-file. Its callers are left empty rather than guessed from the whole range, which would
        walk hundreds of functions that belong to neighbouring files.
        """
        scope = self.scope(copy.version, copy.module, copy.addr)
        sites = self.references.get((copy.version, copy.module, copy.addr), ())
        outside = [site for site in sites if not in_file(copy.module, scope, site)]
        if outside:
            # A relocation that names several modules records no winner, so none of them owns the reference.
            ambiguous = all(site[3] for site in outside)
            return Emission(GLOBAL_AMBIGUOUS if ambiguous else GLOBAL, (), ())
        if not sites:
            return Emission(UNREFERENCED, (), ())
        if scope is not None and scope[2] is None:
            return Emission(STATIC_UNASSIGNED, (), ())
        callers, stops, seen, frontier = [], [], {copy.addr}, [copy.addr]
        while frontier:
            current = frontier.pop(0)
            current_sites = self.references.get((copy.version, copy.module, current), ())
            outside = [site for site in current_sites if not in_file(copy.module, scope, site)]
            if current != copy.addr:
                if outside or not current_sites:
                    # Global already, or referenced by nothing: writing it emits it, and the walk stops here.
                    stops.append(self.function_at(copy.version, copy.module, current) or (current, ""))
                    continue
                callers.append(self.function_at(copy.version, copy.module, current) or (current, ""))
            for site in current_sites:
                if not in_file(copy.module, scope, site):
                    continue
                caller = self.function_at(copy.version, copy.module, site[1])
                if caller is None or caller[0] in seen:
                    continue
                seen.add(caller[0])
                frontier.append(caller[0])
        return Emission(STATIC, tuple(callers), tuple(stops))


def build_groups(copies, resolve, index=None) -> list:
    """Groups the copies, labels each group, and (when index is given) walks the emission of its unmatched copies."""
    raw = group_copies(copies)
    ids = group_ids(raw)
    groups = []
    for group in raw:
        emissions = {}
        if index is not None:
            emissions = {(copy.version, copy.module, copy.addr): index.emission(copy)
                         for copy in group if not copy.matched}
        groups.append(Group(copies=group, label=classify(group, ids, resolve), status=group_status(group),
                            emissions=emissions))
    return groups


def rank(groups) -> list:
    """Free matches first by how many functions they unlock, then unmatched groups by copies x size."""
    free = sorted((g for g in groups if g.status == FREE),
                  key=lambda g: (-(len(g.copies) - matched_count(g.copies)), -g.copies[0].size, -len(g.copies)))
    unmatched = sorted((g for g in groups if g.status == UNMATCHED),
                       key=lambda g: (-(len(g.copies) * g.copies[0].size), -len(g.copies), -g.copies[0].size))
    complete = sorted((g for g in groups if g.status == ALL_MATCHED),
                      key=lambda g: (-(len(g.copies) * g.copies[0].size), -len(g.copies)))
    return free + unmatched + complete


def render_callers(emission) -> str:
    """The emission walk as `0xADDR:name` entries, the functions it stopped at marked with a trailing `*`."""
    return ";".join([f"0x{addr:08x}:{name}" for addr, name in emission.callers]
                    + [f"0x{addr:08x}:{name}*" for addr, name in emission.stops])


def tsv_lines(groups):
    """Yields the TSV header and one row per group."""
    yield HEADER
    for number, group in enumerate(groups):
        fields = [str(number), group.status, group.label, str(len(group.copies)), f"0x{group.copies[0].size:x}",
                  str(matched_count(group.copies)), str(len(group.copies) - matched_count(group.copies))]
        for copy in group.copies:
            emission = group.emissions.get((copy.version, copy.module, copy.addr))
            fields += [copy.version, copy.module, f"0x{copy.addr:08x}", copy.name, "1" if copy.matched else "0",
                       emission.kind if emission else "", render_callers(emission) if emission else ""]
        yield "\t".join(fields)


def matched_functions(modules, report) -> dict:
    """The addresses objdiff matches at 100% in a version, by module, from its report.json.

    A delinked object's function addresses are relative to its unit's section, so the offset between them and the ROM
    address is constant per unit. It is voted on by the names a unit's functions and the module's symbols share; a
    unit dsd could not pair with a source file is a gap named `_dsd_gap@MODULE_n`.
    """
    source_modules = {}
    for name, module in modules.items():
        for _section, _start, _end, source in delink_entries(module):
            source_modules[source.removesuffix(".c")] = name
    symbols = {}
    functions = {}
    for name, module in modules.items():
        table = collections.defaultdict(list)
        for symbol in module.symbols:
            if symbol.kind == "function":
                table[symbol.name].append(symbol)
        symbols[name] = table
        functions[name] = {f.addr for f in module.functions()}

    matched = collections.defaultdict(set)
    for unit in report["units"]:
        name = unit["name"]
        module_name = source_modules.get(name)
        if module_name is None and name.startswith("_dsd_gap@"):
            tag = name.split("@", 1)[1].rsplit("_", 1)[0]
            module_name = "." if tag == "main" else tag if tag in modules else f"overlays/{tag}"
        if module_name not in modules:
            continue
        votes = collections.Counter()
        for function in unit.get("functions", []):
            entries = symbols[module_name].get(function["name"], ())
            if len(entries) == 1:
                votes[entries[0].addr - int(function["address"])] += 1
        if not votes:
            continue
        delta = votes.most_common(1)[0][0]
        for function in unit.get("functions", []):
            addr = int(function["address"]) + delta
            if addr in functions[module_name] and function.get("fuzzy_match_percent") == 100:
                matched[module_name].add(addr)
    return matched


def bucket(group) -> str:
    """Size bucket of a group, for the distribution summaries."""
    size = group.copies[0].size
    if size <= 0x10:
        return "0x10"
    return "0x11-0x1f" if size <= 0x1f else "0x20-0x3f" if size <= 0x3f else "0x40-0x7f" if size <= 0x7f else "0x80+"


def bucket_counts(groups) -> str:
    counts = collections.Counter(bucket(group) for group in groups)
    return ", ".join(f"{name} {counts.get(name, 0)}" for name in BUCKETS)


def unlocked(group) -> int:
    return len(group.copies) - matched_count(group.copies)


def summarise(groups) -> None:
    """Prints the counts the ticket and its issue quote, to stderr, so stdout stays the TSV."""
    def count(selected):
        return len(selected), sum(len(group.copies) for group in selected)

    free = [g for g in groups if g.status == FREE]
    unmatched = [g for g in groups if g.status == UNMATCHED]
    complete = [g for g in groups if g.status == ALL_MATCHED]
    total = sum(len(group.copies) for group in groups)
    print(f"groups with 2+ copies: {len(groups)} ({total} functions, {total - len(groups)} copies beyond the first)",
          file=sys.stderr)
    print(f"free-match groups: {len(free)} ({sum(unlocked(g) for g in free)} unmatched functions in them)",
          file=sys.stderr)
    print(f"unmatched groups: {count(unmatched)[0]} ({count(unmatched)[1]} functions)", file=sys.stderr)
    print(f"all-copies-matched groups: {count(complete)[0]} ({count(complete)[1]} functions)", file=sys.stderr)

    print("by relocation targets:", file=sys.stderr)
    for label in LABELS:
        selected = [g for g in groups if g.label == label]
        label_free = [g for g in selected if g.status == FREE]
        label_unmatched = [g for g in selected if g.status == UNMATCHED]
        print(f"  {label}: {count(selected)[0]} groups, {count(selected)[1]} functions", file=sys.stderr)
        print(f"    free-match: {len(label_free)} groups, unlocking {sum(unlocked(g) for g in label_free)} functions",
              file=sys.stderr)
        print(f"      by size: {bucket_counts(label_free)}", file=sys.stderr)
        print(f"    unmatched: {len(label_unmatched)} groups, {count(label_unmatched)[1]} functions", file=sys.stderr)
        print(f"      by size: {bucket_counts(label_unmatched)}", file=sys.stderr)

    combined = [g for g in groups if g.label in ("identical", "same via duplicates")]
    combined_free = [g for g in combined if g.status == FREE]
    combined_unmatched = [g for g in combined if g.status == UNMATCHED]
    print(f"identical + same via duplicates: free-match {len(combined_free)} groups "
          f"(unlocking {sum(unlocked(g) for g in combined_free)} functions), "
          f"unmatched {len(combined_unmatched)} groups ({count(combined_unmatched)[1]} functions)", file=sys.stderr)

    kinds = collections.Counter(emission.kind for group in groups for emission in group.emissions.values())
    print("emission of unmatched copies: " + ", ".join(f"{kind} {kinds.get(kind, 0)}"
                                                       for kind in (GLOBAL, GLOBAL_AMBIGUOUS, STATIC, STATIC_UNASSIGNED,
                                                                    UNREFERENCED)),
          file=sys.stderr)
    labels = collections.Counter(group.label for group in groups)
    print("labels: " + ", ".join(f"{label} {labels.get(label, 0)}" for label in LABELS), file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--version", default="b2_us", help="primary version, whose objdiff report gives match status")
    parser.add_argument("--other", default="w2_us", help="version whose functions are added when they differ")
    parser.add_argument("--report", type=Path, help="primary version's report.json (default build/<version>/report.json)")
    parser.add_argument("--version-map", type=Path, default=ROOT / "build" / "version_map.tsv",
                        help="version_map.py's output; without it only the primary version is scanned")
    parser.add_argument("--min-size", type=lambda value: int(value, 0), default=DEFAULT_MIN_SIZE,
                        help="smallest function to consider, in bytes (default 0x10)")
    parser.add_argument("-o", "--output", type=Path, help="output TSV (default stdout)")
    args = parser.parse_args()

    report_path = args.report or ROOT / "build" / args.version / "report.json"
    if not report_path.exists():
        parser.error(f"{report_path} does not exist: run ninja first, or pass --report")
    report = json.loads(report_path.read_text())

    primary = load_modules(args.version)
    matched = matched_functions(primary, report)
    mapped = sum(len(addresses) for addresses in matched.values())
    print(f"matched {mapped} functions in {report_path} "
          f"(report says {report['measures']['matched_functions']})", file=sys.stderr)

    copies = module_copies(args.version, primary, matched, args.min_size)
    other_modules = {}
    if args.version_map.exists():
        other_modules = load_modules(args.other)
        added = version_map_copies(args.other, other_modules, args.version_map.read_text(), args.min_size)
        copies += added
        print(f"{len(added)} {args.other} functions added ({args.version_map})", file=sys.stderr)
    else:
        print(f"{args.version_map} is missing: only {args.version} is scanned "
              f"(tools/decomp/version_map.py writes it)", file=sys.stderr)

    index = EmissionIndex({args.version: primary, args.other: other_modules})
    groups = build_groups(copies, make_resolver(index.functions), index)
    lines = list(tsv_lines(rank(groups)))
    if args.output:
        args.output.write_text("\n".join(lines) + "\n")
    else:
        print("\n".join(lines))
    summarise(groups)
    return 0


if __name__ == "__main__":
    sys.exit(main())

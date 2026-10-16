"""Focused duplicate-finder tests; fixtures never read the repository's configs or modules."""
import tempfile
import unittest
from pathlib import Path

from tools.decomp import find_duplicates as fd
from tools.decomp.dsd_config import Module, Symbol

VERSION = "b2_us"


def function(name, addr, size, thumb=True):
    return Symbol(name, "function", addr, f"{'thumb' if thumb else 'arm'},size=0x{size:x}", "", "")


def delinks_text(text, files=()):
    """A module's delinks.txt: the module sections, a blank line, then one entry per source file."""
    lines = [f"    .text       start:{text[0]:#010x} end:{text[1]:#010x} kind:code align:32", ""]
    for source, start, end in files:
        lines.append(f"{source}:")
        lines.append(f"    .text       start:{start:#010x} end:{end:#010x}")
    return "\n".join(lines) + "\n"


class Fixture(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.modules = {}

    def module(self, name, base, code, symbols, relocs=(), text=None, files=()):
        config = self.root / "config" / VERSION / name.replace("/", "_")
        config.mkdir(parents=True, exist_ok=True)
        binary = self.root / f"{name.replace('/', '_')}.bin"
        binary.write_bytes(code)
        (config / "delinks.txt").write_text(delinks_text(text or (base, base + len(code)), files))
        module = Module(name, config, binary, base, symbols=list(symbols),
                        reloc_sources=sorted(relocs), relocs=dict(relocs))
        self.modules[name] = module
        return module

    def index(self):
        return fd.EmissionIndex({VERSION: self.modules})

    def analyse(self, min_size=0x10, matched=None, with_emission=True):
        index = self.index()
        copies = fd.module_copies(VERSION, self.modules, matched or {}, min_size)
        groups = fd.build_groups(copies, fd.make_resolver(index.functions), index if with_emission else None)
        return groups, copies

    def copy(self, copies, name):
        return next(copy for copy in copies if copy.name == name)


class GroupingTests(Fixture):
    def test_identical_bytes_group_across_modules(self):
        code = bytes(range(0x10))
        self.module("overlays/ov001", 0x1000, code, [function("funcA", 0x1000, 0x10)])
        self.module("overlays/ov002", 0x2000, code, [function("funcB", 0x2000, 0x10)])
        groups, _ = self.analyse()
        self.assertEqual(len(groups), 1)
        self.assertEqual([copy.name for copy in groups[0].copies], ["funcA", "funcB"])
        self.assertEqual(groups[0].label, "identical")
        self.assertEqual(groups[0].status, "unmatched")

    def test_group_with_a_matched_copy_is_free(self):
        code = bytes(range(0x10))
        self.module("overlays/ov001", 0x1000, code, [function("funcA", 0x1000, 0x10)])
        self.module("overlays/ov002", 0x2000, code, [function("funcB", 0x2000, 0x10)])
        groups, _ = self.analyse(matched={"overlays/ov001": {0x1000}})
        self.assertEqual(groups[0].status, "free")
        self.assertEqual(fd.matched_count(groups[0].copies), 1)

    def test_different_bytes_do_not_group(self):
        self.module("overlays/ov001", 0x1000, bytes(range(0x10)), [function("funcA", 0x1000, 0x10)])
        self.module("overlays/ov002", 0x2000, bytes(range(1, 0x11)), [function("funcB", 0x2000, 0x10)])
        groups, _ = self.analyse()
        self.assertEqual(groups, [])

    def test_min_size_excludes_small_functions(self):
        code = bytes(range(8))
        self.module("overlays/ov001", 0x1000, code, [function("funcA", 0x1000, 8)])
        self.module("overlays/ov002", 0x2000, code, [function("funcB", 0x2000, 8)])
        self.assertEqual(self.analyse(min_size=0x10)[0], [])
        self.assertEqual(len(self.analyse(min_size=8)[0]), 1)

    def test_relocated_words_are_masked_before_grouping(self):
        code = bytearray(0x10)
        self.module("overlays/ov001", 0x1000, bytes(code), [function("funcA", 0x1000, 0x10)],
                    {0x1004: ("load", 0x9000, "main")})
        other = bytearray(0x10)
        other[4:8] = (0x12345678).to_bytes(4, "little")
        self.module("overlays/ov002", 0x2000, bytes(other), [function("funcB", 0x2000, 0x10)],
                    {0x2004: ("load", 0x9000, "main")})
        groups, _ = self.analyse()
        self.assertEqual(len(groups), 1)
        self.assertEqual(groups[0].label, "identical")


class LabelTests(Fixture):
    def test_same_targets_are_identical(self):
        code = bytearray(0x10)
        self.module("overlays/ov001", 0x1000, bytes(code), [function("funcA", 0x1000, 0x10)],
                    {0x1004: ("load", 0x9000, "main")})
        self.module("overlays/ov002", 0x2000, bytes(code), [function("funcB", 0x2000, 0x10)],
                    {0x2004: ("load", 0x9000, "main")})
        groups, _ = self.analyse()
        self.assertEqual(groups[0].label, "identical")

    def test_targets_that_are_one_duplicate_group_are_same_via_duplicates(self):
        outer, inner = bytes(range(0x10, 0x20)), bytes(range(0x20, 0x30))
        self.module("overlays/ov001", 0x1000, outer + inner,
                    [function("funcP", 0x1000, 0x10), function("funcInnerA", 0x1010, 0x10)],
                    {0x1004: ("load", 0x1010, "overlays(1)")})
        self.module("overlays/ov002", 0x2000, outer + inner,
                    [function("funcQ", 0x2000, 0x10), function("funcInnerB", 0x2010, 0x10)],
                    {0x2004: ("load", 0x2010, "overlays(2)")})
        groups, _ = self.analyse()
        by_names = {frozenset(copy.name for copy in group.copies): group for group in groups}
        self.assertEqual(by_names[frozenset({"funcP", "funcQ"})].label, "same via duplicates")
        self.assertEqual(by_names[frozenset({"funcInnerA", "funcInnerB"})].label, "identical")

    def test_a_thumb_target_still_resolves_to_its_function(self):
        outer, inner = bytes(range(0x10, 0x20)), bytes(range(0x20, 0x30))
        self.module("overlays/ov001", 0x1000, outer + inner,
                    [function("funcP", 0x1000, 0x10), function("funcInnerA", 0x1010, 0x10)],
                    {0x1004: ("load", 0x1010, "overlays(1)")})
        # The other copy points at its inner function with the Thumb bit set: one function, odd pointer.
        self.module("overlays/ov002", 0x2000, outer + inner,
                    [function("funcQ", 0x2000, 0x10), function("funcInnerB", 0x2010, 0x10)],
                    {0x2004: ("load", 0x2011, "overlays(2)")})
        groups, _ = self.analyse()
        by_names = {frozenset(copy.name for copy in group.copies): group for group in groups}
        self.assertEqual(by_names[frozenset({"funcP", "funcQ"})].label, "same via duplicates")

    def test_different_targets_are_same_shape(self):
        outer = bytes(range(0x10, 0x20))
        self.module("overlays/ov001", 0x1000, outer, [function("funcA", 0x1000, 0x10)],
                    {0x1004: ("load", 0x9000, "main")})
        self.module("overlays/ov002", 0x2000, outer, [function("funcB", 0x2000, 0x10)],
                    {0x2004: ("load", 0x9100, "main")})
        groups, _ = self.analyse()
        self.assertEqual(groups[0].label, "same shape")

    def test_a_different_relocation_layout_is_same_shape(self):
        self.module("overlays/ov001", 0x1000, bytes(0x10), [function("funcA", 0x1000, 0x10)],
                    {0x1004: ("load", 0x9000, "main")})
        self.module("overlays/ov002", 0x2000, bytes(0x10), [function("funcB", 0x2000, 0x10)],
                    {0x2008: ("load", 0x9000, "main")})
        groups, _ = self.analyse()
        self.assertEqual(groups[0].label, "same shape")


class EmissionTests(Fixture):
    """funcS is the copy. Its file is src/a.c (0x1000-0x1080) unless a test says otherwise."""

    def build(self, relocs=(), files=(("src/a.c", 0x1000, 0x1080), ("src/b.c", 0x1080, 0x1100))):
        self.module("overlays/ov001", 0x1000, bytes(0x100),
                    [function("funcS", 0x1000, 0x10), function("funcC", 0x1040, 0x10),
                     function("funcCaller2", 0x1060, 0x10), function("funcOther", 0x1080, 0x10)],
                    (), text=(0x1000, 0x1100), files=list(files))
        module = self.modules["overlays/ov001"]
        for source, target in relocs:
            module.relocs[source] = ("load", target, "overlays(1)")
        module.reloc_sources = sorted(module.relocs)
        return fd.EmissionIndex({VERSION: self.modules})

    def emission(self, index, name="funcS"):
        copies = fd.module_copies(VERSION, self.modules, {}, 0x10)
        return index.emission(self.copy(copies, name))

    def test_a_function_referenced_from_another_file_is_global(self):
        # funcOther, in src/b.c, references funcS
        emission = self.emission(self.build([(0x1084, 0x1000)]))
        self.assertEqual(emission.kind, "global")
        self.assertEqual(emission.callers, ())
        self.assertEqual(emission.stops, ())

    def test_an_unreferenced_function_is_unreferenced(self):
        emission = self.emission(self.build())
        self.assertEqual(emission.kind, "unreferenced")

    def test_a_reference_with_the_thumb_bit_still_counts(self):
        # A Thumb pointer to funcS at 0x1000 must be seen as a reference to it.
        emission = self.emission(self.build([(0x1084, 0x1001)]))
        self.assertEqual(emission.kind, "global")

    def test_a_function_called_only_in_its_file_stops_at_its_global_caller(self):
        emission = self.emission(self.build([(0x1044, 0x1000), (0x1084, 0x1040)]))
        self.assertEqual(emission.kind, "static")
        self.assertEqual(emission.callers, ())
        self.assertEqual(emission.stops, ((0x1040, "funcC"),))

    def test_the_closure_walks_up_through_several_in_file_callers(self):
        emission = self.emission(self.build([(0x1064, 0x1040), (0x1044, 0x1000), (0x1084, 0x1060)]))
        self.assertEqual(emission.kind, "static")
        self.assertEqual(emission.callers, ((0x1040, "funcC"),))
        self.assertEqual(emission.stops, ((0x1060, "funcCaller2"),))

    def test_an_in_file_caller_nothing_references_is_itself_a_stop(self):
        emission = self.emission(self.build([(0x1044, 0x1000)]))  # funcC calls funcS; nothing calls funcC
        self.assertEqual(emission.kind, "static")
        self.assertEqual(emission.stops, ((0x1040, "funcC"),))

    def test_an_unassigned_function_is_static_but_its_callers_are_not_guessed(self):
        # funcS and funcC are both in the gap before src/b.c, which holds several original files, so the walk over
        # the whole range would claim neighbours of the file that does not exist yet.
        index = self.build([(0x1044, 0x1000), (0x1084, 0x1040)], files=(("src/b.c", 0x1080, 0x1100),))
        emission = self.emission(index)
        self.assertEqual(emission.kind, "static?")
        self.assertEqual(emission.callers, ())
        self.assertEqual(emission.stops, ())


class MatchedFunctionsTests(Fixture):
    def test_report_addresses_map_through_a_name_vote(self):
        self.module(".", 0x1000, bytes(0x20), [function("alpha", 0x1000, 0x10), function("beta", 0x1010, 0x10)],
                    text=(0x1000, 0x2000), files=[("src/a.c", 0x1000, 0x1020)])
        report = {"units": [{"name": "src/a", "functions": [
            {"name": "alpha", "address": "0", "size": "16", "fuzzy_match_percent": 100.0},
            {"name": "beta", "address": "16", "size": "16"},
        ]}]}
        matched = fd.matched_functions(self.modules, report)
        self.assertEqual(matched["."], {0x1000})

    def test_a_gap_unit_is_mapped_by_its_overlay_tag(self):
        self.module("overlays/ov182", 0x2000, bytes(0x10), [function("funcA", 0x2000, 0x10)],
                    text=(0x2000, 0x2100))
        report = {"units": [{"name": "_dsd_gap@ov182_0", "functions": [
            {"name": "funcA", "address": "0", "size": "16", "fuzzy_match_percent": 100.0},
        ]}]}
        matched = fd.matched_functions(self.modules, report)
        self.assertEqual(matched["overlays/ov182"], {0x2000})


class VersionMapTests(Fixture):
    def test_only_functions_differing_from_their_partner_are_added(self):
        self.module("overlays/ov001", 0x3000, bytes(range(0x10, 0x20)),
                    [function("funcDiff", 0x3000, 0x10), function("funcSame", 0x3010, 0x10)])
        table = ("module\tb2_addr\tw2_addr\tb2_name\tw2_name\tstatus\n"
                 "overlays/ov001\t0x1000\t0x3000\tfuncDiff\tfuncDiff\tdifferent\n"
                 "overlays/ov001\t0x1010\t0x3010\tfuncSame\tfuncSame\tidentical\n")
        copies = fd.version_map_copies("w2_us", self.modules, table, 0x10)
        self.assertEqual([copy.name for copy in copies], ["funcDiff"])
        self.assertFalse(copies[0].matched)


def copy(matched, size, name="func", module="overlays/ov001", addr=0x1000, version=VERSION):
    return fd.Copy(version, module, addr, name, size, matched, bytes(size), (), ())


class RankingTests(unittest.TestCase):
    def group(self, status, label, copies):
        return fd.Group(copies=copies, label=label, status=status, emissions={})

    def test_free_matches_come_first_and_unmatched_groups_by_copies_times_size(self):
        free = self.group("free", "identical", [copy(True, 0x20), copy(False, 0x20)])
        small = self.group("unmatched", "identical", [copy(False, 0x10), copy(False, 0x10)])
        big = self.group("unmatched", "identical", [copy(False, 0x20)] * 3)
        ordered = fd.rank([small, big, free])
        self.assertEqual([group.status for group in ordered], ["free", "unmatched", "unmatched"])
        self.assertIs(ordered[1], big)
        self.assertIs(ordered[2], small)

    def test_a_bigger_free_group_unlocks_more_so_it_ranks_first(self):
        one = self.group("free", "identical", [copy(True, 0x10), copy(False, 0x10)])
        two = self.group("free", "identical", [copy(True, 0x10), copy(False, 0x10), copy(False, 0x10)])
        ordered = fd.rank([one, two])
        self.assertIs(ordered[0], two)


class TsvTests(Fixture):
    def test_header_then_one_row_per_group_with_seven_columns_per_copy(self):
        code = bytes(range(0x10))
        self.module("overlays/ov001", 0x1000, code, [function("funcA", 0x1000, 0x10)],
                    text=(0x1000, 0x1100), files=[("src/a.c", 0x1000, 0x1100)])
        self.module("overlays/ov002", 0x2000, code, [function("funcB", 0x2000, 0x10)],
                    text=(0x2000, 0x2100), files=[("src/b.c", 0x2000, 0x2100)])
        groups, _ = self.analyse(matched={"overlays/ov001": {0x1000}})
        lines = list(fd.tsv_lines(fd.rank(groups)))
        self.assertEqual(lines[0].split("\t")[:7], ["group", "status", "label", "copies", "size", "matched", "unmatched"])
        fields = lines[1].split("\t")
        self.assertEqual(fields[:7], ["0", "free", "identical", "2", "0x10", "1", "1"])
        self.assertEqual(len(fields), 7 + 7 * 2)
        # The matched copy carries no emission work, the unmatched one is unreferenced so it needs none.
        self.assertEqual(fields[7:14], ["b2_us", "overlays/ov001", "0x00001000", "funcA", "1", "", ""])
        self.assertEqual(fields[14:21], ["b2_us", "overlays/ov002", "0x00002000", "funcB", "0", "unreferenced", ""])

    def test_a_static_copy_lists_its_callers_and_marks_where_the_walk_stopped(self):
        emission = fd.Emission("static", ((0x1040, "funcC"),), ((0x1060, "funcCaller2"),))
        self.assertEqual(fd.render_callers(emission),
                         "0x00001040:funcC;0x00001060:funcCaller2*")


if __name__ == "__main__":
    unittest.main()

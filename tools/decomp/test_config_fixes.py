"""Focused config-fix tests; fixtures never read or change the repository's configs."""
import contextlib
import io
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from tools.decomp import config_fixes as fixes


MODULE = "overlays/ov011"
SOURCE = 0x1010
MAPPED = 0x2010


class FakeMapper:
    def __init__(self):
        self.calls = []
        self.addresses = {(".", SOURCE, "w2_us"): MAPPED,
                          (MODULE, 0x5010, "w2_us"): 0x6010}
        self.error = None

    def map(self, module, addr, version, section=None):
        self.calls.append((module, addr, version))
        if self.error:
            raise SystemExit(self.error)
        return self.addresses[(module, addr, version)]


class ConfigFixFixture(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.fix_path = self.root / "config" / "fixes.txt"
        for name, value in [("ROOT", self.root), ("FIXES", self.fix_path)]:
            patcher = patch.object(fixes, name, value)
            patcher.start()
            self.addCleanup(patcher.stop)
        self.mapper = FakeMapper()
        for version, source, recorded in [("b2_us", 0x1000, 0x5000), ("w2_us", 0x2000, 0x6000)]:
            self.sections(version, ".", [(".text", source, source + 0x100)])
            self.sections(version, MODULE, [(".text", recorded, recorded + 0x100)])
            self.relocs(version).write_text(self.forward(version))
        self.relocs("b2_us").write_text(self.orphan(SOURCE) + self.forward("b2_us"))
        fixes.save_fixes([])

    def sections(self, version, module, sections):
        path = fixes.config_dir(version, module) / "delinks.txt"
        path.parent.mkdir(parents=True, exist_ok=True)
        # parse_sections reads the indented module header only, stopping before source-file ranges.
        lines = [f"    {name:<11} start:{start:#010x} end:{end:#010x} kind:code align:4"
                 for name, start, end in sections]
        path.write_text("\n".join(lines) + "\n\nsrc/example.c:\n    .text start:0x00000000 end:0xffffffff\n")

    def relocs(self, version):
        return fixes.config_dir(version, MODULE) / "relocs.txt"

    @staticmethod
    def orphan(addr):
        return f"from:{addr:#010x} kind:load to:0x00005000 module:main\n"

    @staticmethod
    def forward(version):
        source, target = (0x5004, SOURCE) if version == "b2_us" else (0x6004, MAPPED)
        return f"from:{source:#010x} kind:load to:{target:#010x} module:main\n"

    def snapshot(self):
        return {path.relative_to(self.root): path.read_bytes() for path in self.root.rglob("*") if path.is_file()}

    def apply(self, addr=SOURCE, owner="."):
        fixes.apply_everywhere(self.mapper, MODULE, addr, "remove_orphan_reloc", owner)

    def cli(self, *args):
        output = io.StringIO()
        with patch.object(fixes, "AddressMapper", return_value=self.mapper), \
                patch("sys.argv", ["config_fixes.py", *args]), contextlib.redirect_stdout(output):
            fixes.main()
        return output.getvalue()

    def assert_rejected_without_changes(self, operation, message):
        before = self.snapshot()
        with self.assertRaisesRegex(SystemExit, message):
            operation()
        self.assertEqual(self.snapshot(), before)


class OrphanRelocationTests(ConfigFixFixture):
    def test_primary_present_other_absent_preserves_forward_relocations(self):
        absent_before = self.relocs("w2_us").read_bytes()
        self.apply()
        self.assertEqual(self.relocs("b2_us").read_text(), self.forward("b2_us"))
        self.assertEqual(self.relocs("w2_us").read_bytes(), absent_before)
        self.assertEqual(self.mapper.calls, [(".", SOURCE, "w2_us")])
        self.assertEqual(fixes.load_fixes(), [])  # Applying a tuple does not itself record it.

    def test_each_version_uses_its_mapped_source(self):
        self.relocs("w2_us").write_text(self.orphan(MAPPED) + self.forward("w2_us"))
        self.apply()
        for version in ["b2_us", "w2_us"]:
            self.assertEqual(self.relocs(version).read_text(), self.forward(version))
        self.assertEqual(self.mapper.calls, [(".", SOURCE, "w2_us")])

    def test_repeat_and_replay_are_idempotent(self):
        fix = (MODULE, SOURCE, "remove_orphan_reloc", ".")
        fixes.save_fixes([fix, fix])
        self.assertEqual(fixes.load_fixes(), [fix])
        self.assertIn("overlays/ov011 0x00001010 remove_orphan_reloc .\n", self.fix_path.read_text())
        fixes.apply_everywhere(self.mapper, *fixes.load_fixes()[0])
        after = self.snapshot()
        self.apply()
        self.assertEqual(self.snapshot(), after)
        # Regenerated configs can bring the orphan back in either version.
        self.relocs("w2_us").write_text(self.orphan(MAPPED) + self.forward("w2_us"))
        fixes.apply_everywhere(self.mapper, *fixes.load_fixes()[0])
        self.assertEqual(self.snapshot(), after)

    def test_save_preserves_equal_address_action_order_and_stably_deduplicates(self):
        start = (MODULE, SOURCE, "section_start", "src/example.c:.text")
        end = (MODULE, SOURCE, "section_end", "src/previous.c:.text")
        data = (MODULE, SOURCE, "add_data", "boundary")
        earlier = (".", 0x1000, "add_label", "label")
        later = ("overlays/ov012", SOURCE, "remove_symbol", "")
        for ordered in [(start, end, data), (data, end, start)]:
            with self.subTest(actions=[fix[2] for fix in ordered]):
                fixes.save_fixes([later, ordered[0], earlier, ordered[1], ordered[0], ordered[2], ordered[1]])
                self.assertEqual(fixes.load_fixes(), [earlier, *ordered, later])
                before = self.fix_path.read_bytes()
                fixes.save_fixes(fixes.load_fixes())
                self.assertEqual(self.fix_path.read_bytes(), before)
                fixes.save_fixes([*fixes.load_fixes(), *ordered])
                self.assertEqual(self.fix_path.read_bytes(), before)

    def test_recorded_module_cannot_be_source_owner(self):
        self.assert_rejected_without_changes(lambda: self.apply(owner=MODULE), "must differ")
        self.assertEqual(self.mapper.calls, [])

    def test_empty_source_owner_is_rejected(self):
        self.assert_rejected_without_changes(lambda: self.apply(owner=""), "must differ")

    def test_primary_source_must_be_inside_owner_sections(self):
        for addr in [0x0fff, 0x1100, 0x3000]:
            with self.subTest(addr=addr):
                self.assert_rejected_without_changes(lambda: self.apply(addr), "outside source module")
        self.assertEqual(self.mapper.calls, [])

    def test_source_inside_any_recorded_section_is_rejected(self):
        self.sections("b2_us", MODULE, [(".text", 0x5000, 0x5100), (".data", SOURCE, SOURCE + 4)])
        self.assert_rejected_without_changes(self.apply, "inside recorded module")
        self.assertEqual(self.mapper.calls, [])

    def test_mapped_source_must_be_inside_owner(self):
        self.mapper.addresses[(".", SOURCE, "w2_us")] = 0x2100
        self.assert_rejected_without_changes(self.apply, "outside source module")
        self.assertEqual(self.mapper.calls, [(".", SOURCE, "w2_us")])

    def test_mapped_source_cannot_overlap_recorded_module(self):
        self.sections("w2_us", MODULE, [(".text", 0x6000, 0x6100), (".bss", MAPPED, MAPPED + 4)])
        self.assert_rejected_without_changes(self.apply, "inside recorded module")

    def test_second_version_mapping_failure_prevents_primary_mutation(self):
        self.mapper.error = "no paired function"
        self.assert_rejected_without_changes(self.apply, "no paired function")

    def test_missing_second_relocation_file_prevents_primary_mutation(self):
        self.relocs("w2_us").unlink()
        before = self.snapshot()
        with self.assertRaises(FileNotFoundError):
            self.apply()
        self.assertEqual(self.snapshot(), before)

    def test_empty_module_section_header_is_not_evidence(self):
        self.sections("w2_us", MODULE, [])
        self.assert_rejected_without_changes(self.apply, "cannot validate sections")

    def test_malformed_later_relocation_file_prevents_primary_mutation(self):
        self.relocs("w2_us").write_text(self.orphan(MAPPED) + "invalid relocation\n")
        self.assert_rejected_without_changes(self.apply, "malformed relocation file")

    def test_empty_other_relocation_file_is_left_untouched(self):
        self.relocs("w2_us").write_text("")
        self.apply()
        self.assertEqual(self.relocs("w2_us").read_bytes(), b"")
        self.assertEqual(self.relocs("b2_us").read_text(), self.forward("b2_us"))

    def test_duplicates_in_either_version_are_ambiguous(self):
        for version, addr in [("b2_us", SOURCE), ("w2_us", MAPPED)]:
            with self.subTest(version=version):
                original = self.relocs(version).read_text()
                self.relocs(version).write_text(self.orphan(addr) * 2 + self.forward(version))
                self.assert_rejected_without_changes(self.apply, "multiple relocations")
                self.relocs(version).write_text(original)

    def test_ordinary_remove_reloc_still_maps_through_recorded_module(self):
        for version, addr in [("b2_us", 0x5010), ("w2_us", 0x6010)]:
            self.relocs(version).write_text(self.orphan(addr) + self.forward(version))
        fixes.apply_everywhere(self.mapper, MODULE, 0x5010, "remove_reloc", "")
        self.assertEqual(self.mapper.calls, [(MODULE, 0x5010, "w2_us")])
        for version in ["b2_us", "w2_us"]:
            self.assertEqual(self.relocs(version).read_text(), self.forward(version))

    def test_ordinary_source_mapping_failure_prevents_primary_mutation(self):
        self.relocs("b2_us").write_text(self.orphan(0x5010) + self.forward("b2_us"))
        self.mapper.error = "source mapping failed"
        self.assert_rejected_without_changes(
            lambda: fixes.apply_everywhere(self.mapper, MODULE, 0x5010, "remove_reloc", ""),
            "source mapping failed")
        self.assertEqual(self.mapper.calls, [(MODULE, 0x5010, "w2_us")])

    def test_add_reloc_destination_mapping_failure_prevents_primary_mutation(self):
        self.mapper.error = "destination mapping failed"
        self.assert_rejected_without_changes(
            lambda: fixes.apply_everywhere(self.mapper, MODULE, 0x5010, "add_reloc", "load,main,0x1011"),
            "destination mapping failed")
        self.assertEqual(self.mapper.calls, [(".", SOURCE, "w2_us")])

    def test_add_reloc_source_mapping_failure_after_destination_map_prevents_primary_mutation(self):
        def mapping(module, addr, version, section=None):
            if module == MODULE:
                raise SystemExit("source mapping failed")
            return MAPPED

        with patch.object(self.mapper, "map", side_effect=mapping) as mapped:
            self.assert_rejected_without_changes(
                lambda: fixes.apply_everywhere(self.mapper, MODULE, 0x5010, "add_reloc", "load,main,0x1011"),
                "source mapping failed")
        self.assertEqual(mapped.call_args_list[0].args, (".", SOURCE, "w2_us"))
        self.assertEqual(mapped.call_args_list[1].args, (MODULE, 0x5010, "w2_us"))

    def test_add_reloc_maps_source_and_thumb_destination_for_both_versions(self):
        before = {version: self.relocs(version).read_text() for version in ["b2_us", "w2_us"]}
        fixes.apply_everywhere(self.mapper, MODULE, 0x5010, "add_reloc", "load,main,0x1011,0x4")
        for version, source, target in [("b2_us", 0x5010, 0x1011), ("w2_us", 0x6010, 0x2011)]:
            expected = f"from:{source:#010x} kind:load to:{target:#010x} add:0x4 module:main\n"
            self.assertEqual(self.relocs(version).read_text(), before[version] + expected)
        self.assertEqual(self.mapper.calls, [(".", SOURCE, "w2_us"), (MODULE, 0x5010, "w2_us")])

    def test_cli_later_relocation_conflict_does_not_write_or_record(self):
        self.relocs("w2_us").write_text(self.orphan(0x6010) + self.forward("w2_us"))
        self.assert_rejected_without_changes(
            lambda: self.cli("add-reloc", MODULE, "load", "main", "0x1011", "0x5010"),
            "already has another relocation")

    def test_later_missing_label_function_leaves_primary_symbols_unchanged(self):
        for version in ["b2_us", "w2_us"]:
            path = fixes.config_dir(version, MODULE) / "symbols.txt"
            path.write_text("function kind:function(thumb,size=0x10) addr:0x00005010\n"
                            if version == "b2_us" else "")
        self.assert_rejected_without_changes(
            lambda: self.cli("add-label", MODULE, "alias", "0x5010"), "no function or label")

    def test_successful_label_plans_write_both_versions(self):
        for version, addr in [("b2_us", 0x5010), ("w2_us", 0x6010)]:
            path = fixes.config_dir(version, MODULE) / "symbols.txt"
            path.write_text(f"function kind:function(thumb,size=0x10) addr:{addr:#010x}\n")
        fixes.apply_everywhere(self.mapper, MODULE, 0x5010, "add_label", "alias")
        for version, addr in [("b2_us", 0x5010), ("w2_us", 0x6010)]:
            path = fixes.config_dir(version, MODULE) / "symbols.txt"
            self.assertEqual(path.read_text(),
                             f"function kind:function(thumb,size=0x10) addr:{addr:#010x}\n"
                             f"alias kind:label(thumb) addr:{addr:#010x}\n")
        after = self.snapshot()
        fixes.apply_everywhere(self.mapper, MODULE, 0x5010, "add_label", "alias")
        self.assertEqual(self.snapshot(), after)

    def test_remove_label_drops_only_that_label_in_both_versions(self):
        for version, addr in [("b2_us", 0x5010), ("w2_us", 0x6010)]:
            path = fixes.config_dir(version, MODULE) / "symbols.txt"
            path.write_text(f"function kind:function(thumb,size=0x10) addr:{addr:#010x}\n"
                            f"alias kind:label(thumb) addr:{addr:#010x}\n"
                            f"other kind:label(thumb) addr:{addr:#010x}\n")
        fixes.apply_everywhere(self.mapper, MODULE, 0x5010, "remove_label", "alias")
        for version, addr in [("b2_us", 0x5010), ("w2_us", 0x6010)]:
            path = fixes.config_dir(version, MODULE) / "symbols.txt"
            self.assertEqual(path.read_text(),
                             f"function kind:function(thumb,size=0x10) addr:{addr:#010x}\n"
                             f"other kind:label(thumb) addr:{addr:#010x}\n")
        after = self.snapshot()
        fixes.apply_everywhere(self.mapper, MODULE, 0x5010, "remove_label", "alias")
        self.assertEqual(self.snapshot(), after)

    def test_cli_remove_label_drops_the_recorded_add_label(self):
        for version, addr in [("b2_us", 0x5010), ("w2_us", 0x6010)]:
            path = fixes.config_dir(version, MODULE) / "symbols.txt"
            path.write_text(f"function kind:function(thumb,size=0x10) addr:{addr:#010x}\n")
        self.cli("add-label", MODULE, "alias", "0x5010")
        self.assertIn((MODULE, 0x5010, "add_label", "alias"), fixes.load_fixes())
        self.cli("remove-label", MODULE, "alias", "0x5010")
        self.assertEqual(fixes.load_fixes(), [])
        path = fixes.config_dir("b2_us", MODULE) / "symbols.txt"
        self.assertEqual(path.read_text(), "function kind:function(thumb,size=0x10) addr:0x00005010\n")

    def test_planning_text_transforms_never_writes_and_wrapper_preserves_output(self):
        symbol = "item kind:data(any) addr:0x00005010\n"
        function = "function kind:function(thumb,size=0x10) addr:0x00005010\n"
        reloc = "from:0x00005010 kind:load to:0x00001010 module:main\n"
        cases = [
            ("remove_symbol", "", "symbols.txt", symbol, "\n"),
            ("add_data", "data", "symbols.txt", "", "data kind:data(any) addr:0x00005010\n"),
            ("add_function", "function:thumb:0x10", "symbols.txt", "", function),
            ("add_label", "alias", "symbols.txt", function, function + "alias kind:label(thumb) addr:0x00005010\n"),
            ("reloc_module", "overlay(12)", "relocs.txt", reloc, reloc.replace("module:main", "module:overlay(12)")),
            ("overlay_id", "12", "relocs.txt", "", "from:0x00005010 kind:overlay_id to:12\n"),
            ("link_time_const", "SDK_AUTOLOAD_LIST", "relocs.txt", reloc,
             "from:0x00005010 kind:link_time_const(SDK_AUTOLOAD_LIST)\n"),
            ("link_time_const", "SDK_LTDAUTOLOAD_START", "relocs.txt", "",
             "from:0x00005010 kind:link_time_const(SDK_LTDAUTOLOAD_START)\n"),
            ("add_reloc", "load,main,0x1010", "relocs.txt", "", reloc),
            ("remove_reloc", "", "relocs.txt", reloc, "\n"),
            ("reloc_addend", "4", "relocs.txt", reloc,
             "from:0x00005010 kind:load to:0x0000100c add:0x4 module:main\n"),
        ]
        for action, argument, filename, before, expected in cases:
            with self.subTest(action=action):
                path = fixes.config_dir("b2_us", MODULE) / filename
                path.write_text(before)
                snapshot = self.snapshot()
                self.assertEqual(fixes.plan_fix("b2_us", MODULE, 0x5010, action, argument), (path, expected))
                self.assertEqual(self.snapshot(), snapshot)
                fixes.apply_fix("b2_us", MODULE, 0x5010, action, argument)
                self.assertEqual(path.read_text(), expected)

    def test_section_plans_preserve_existing_ranges_and_wrapper_output(self):
        path = fixes.config_dir("b2_us", MODULE) / "delinks.txt"
        original = path.read_text()
        cases = [
            ("section_start", "src/example.c:.text", original.replace("start:0x00000000", "start:0x00005010")),
            ("section_end", "src/example.c:.text", original.replace("end:0xffffffff", "end:0x00005010")),
            ("add_section", "src/example.c:.bss:0x10",
             original + "    .bss        start:0x00005010 end:0x00005020\n"),
        ]
        for action, argument, expected in cases:
            with self.subTest(action=action):
                path.write_text(original)
                snapshot = self.snapshot()
                self.assertEqual(fixes.plan_fix("b2_us", MODULE, 0x5010, action, argument), (path, expected))
                self.assertEqual(self.snapshot(), snapshot)
                fixes.apply_fix("b2_us", MODULE, 0x5010, action, argument)
                self.assertEqual(path.read_text(), expected)

    def test_cli_records_source_owner_and_replays(self):
        output = self.cli("remove-orphan-reloc", MODULE, "--source-module", ".", hex(SOURCE))
        self.assertIn("remove_orphan_reloc", output)
        self.assertEqual(fixes.load_fixes(), [(MODULE, SOURCE, "remove_orphan_reloc", ".")])
        after = self.snapshot()
        self.cli("remove-orphan-reloc", MODULE, "--source-module", ".", hex(SOURCE))
        self.assertEqual(self.snapshot(), after)
        self.relocs("b2_us").write_text(self.orphan(SOURCE) + self.forward("b2_us"))
        self.assertIn("applied 1 fixes", self.cli("apply"))
        self.assertEqual(self.snapshot(), after)

    def test_cli_mapping_failure_does_not_record_fix(self):
        self.mapper.error = "mapping failed"
        self.assert_rejected_without_changes(
            lambda: self.cli("remove-orphan-reloc", MODULE, "--source-module", ".", hex(SOURCE)), "mapping failed")

    def test_cli_preflights_all_addresses_before_writing(self):
        self.assert_rejected_without_changes(
            lambda: self.cli("remove-orphan-reloc", MODULE, "--source-module", ".", hex(SOURCE), "0x1100"),
            "outside source module")



class SectionPrefixTests(ConfigFixFixture):
    def prefix_fixture(self, crossing=False):
        for version, start in (("b2_us", 0x5010), ("w2_us", 0x6010)):
            path = fixes.config_dir(version, MODULE) / "delinks.txt"
            end = start + (0x18 if crossing else 0x10)
            path.write_text(f"    .rodata start:{start:#010x} end:{start + 0x100:#010x} kind:rodata align:4\n"
                            f"\nsrc/example.c:\n    .rodata start:{start:#010x} end:{end:#010x}\n")

    def test_split_prefix_maps_migrates_and_replays(self):
        self.prefix_fixture()
        self.cli("split-section-prefix", MODULE, ".rodata", ".exceptix", "0x10", "0x5010")
        for version, start in (("b2_us", 0x5010), ("w2_us", 0x6010)):
            text = (fixes.config_dir(version, MODULE) / "delinks.txt").read_text()
            self.assertIn(f".exceptix   start:{start:#010x} end:{start + 0x10:#010x} kind:rodata align:4", text)
            self.assertIn(f".rodata     start:{start + 0x10:#010x}", text)
            self.assertIn(f"src/example.c:\n    .exceptix start:{start:#010x}", text)
        after = self.snapshot()
        self.cli("apply")
        self.assertEqual(self.snapshot(), after)

    def test_split_prefix_rejects_crossing_owner_in_either_version(self):
        self.prefix_fixture()
        path = fixes.config_dir("w2_us", MODULE) / "delinks.txt"
        path.write_text(path.read_text().replace("end:0x00006020\n", "end:0x00006028\n"))
        self.assert_rejected_without_changes(
            lambda: self.cli("split-section-prefix", MODULE, ".rodata", ".exceptix", "0x10", "0x5010"),
            "crosses")

    def test_split_prefix_rejects_conflicting_target(self):
        self.prefix_fixture()
        path = fixes.config_dir("w2_us", MODULE) / "delinks.txt"
        path.write_text("    .exceptix start:0x00006000 end:0x00006008 kind:rodata align:4\n" + path.read_text())
        self.assert_rejected_without_changes(
            lambda: self.cli("split-section-prefix", MODULE, ".rodata", ".exceptix", "0x10", "0x5010"),
            "conflicting")

    def test_split_prefix_replay_rejects_conflicting_attributes_atomically(self):
        for attributes in ("kind:code align:4", "kind:rodata align:8", "kind:code align:8"):
            with self.subTest(attributes=attributes):
                self.prefix_fixture()
                self.cli("split-section-prefix", MODULE, ".rodata", ".exceptix", "0x10", "0x5010")
                path = fixes.config_dir("w2_us", MODULE) / "delinks.txt"
                path.write_text(path.read_text().replace(
                    "end:0x00006020 kind:rodata align:4", f"end:0x00006020 {attributes}"))
                self.assert_rejected_without_changes(lambda: self.cli("apply"), "conflicting")

    def test_split_prefix_rejects_invalid_size_name_and_nonprefix(self):
        self.prefix_fixture()
        for target, size in ((".exceptix", "0"), (".exceptix", "0x100"), (".exceptix", "0x3"), (".rodata", "0x10"),
                             ("bad name", "0x10")):
            self.assert_rejected_without_changes(
                lambda: self.cli("split-section-prefix", MODULE, ".rodata", target, size, "0x5010"), "")
        with self.assertRaisesRegex(SystemExit, "proper prefix"):
            fixes.plan_fix("b2_us", MODULE, 0x5014, "split_section_prefix", ".rodata:.exceptix:0x10")


class BoundaryRecordTests(unittest.TestCase):
    """Recording a file's section boundary again replaces its earlier record, so a replay can't undo a move down."""

    # The fixture of the orphan relocation tests, without their tests
    sections = OrphanRelocationTests.sections
    relocs = OrphanRelocationTests.relocs
    orphan = staticmethod(OrphanRelocationTests.orphan)
    forward = staticmethod(OrphanRelocationTests.forward)
    snapshot = OrphanRelocationTests.snapshot
    cli = OrphanRelocationTests.cli

    def setUp(self):
        OrphanRelocationTests.setUp(self)
        for addr in (0x5010, 0x5020, 0x5030):
            self.mapper.addresses[(MODULE, addr, "w2_us")] = addr + 0x1000
        for version in ("b2_us", "w2_us"):
            path = fixes.config_dir(version, MODULE) / "delinks.txt"
            path.write_text(path.read_text() + "\nsrc/next.c:\n    .text start:0xffffffff end:0xffffffff\n")
            (fixes.config_dir(version, MODULE) / "symbols.txt").write_text("")

    def delinks(self, version="b2_us"):
        return (fixes.config_dir(version, MODULE) / "delinks.txt").read_text()

    def test_move_down_drops_older_record(self):
        self.cli("section-end", MODULE, "src/example.c", ".text", "0x5030")
        self.cli("remove-symbol", MODULE, "0x5030")  # another action at the old address stays
        self.cli("section-start", MODULE, "src/next.c", ".text", "0x5030")
        self.cli("section-end", MODULE, "src/example.c", ".text", "0x5010")
        self.assertEqual(fixes.load_fixes(), [(MODULE, 0x5010, "section_end", "src/example.c:.text"),
                                              (MODULE, 0x5030, "remove_symbol", ""),
                                              (MODULE, 0x5030, "section_start", "src/next.c:.text")])
        after = self.snapshot()
        self.cli("apply")
        self.assertEqual(self.snapshot(), after)
        self.assertIn("end:0x00005010", self.delinks())
        self.assertIn("end:0x00006010", self.delinks("w2_us"))

    def test_two_addresses_in_one_command_keep_last(self):
        self.cli("section-end", MODULE, "src/example.c", ".text", "0x5010", "0x5020")
        self.assertEqual(fixes.load_fixes(), [(MODULE, 0x5020, "section_end", "src/example.c:.text")])

    def add_bss(self, other_start=0x6010):
        for version, addr in [("b2_us", 0x5010), ("w2_us", other_start)]:
            path = fixes.config_dir(version, MODULE) / "delinks.txt"
            text = path.read_text().replace(
                "src/example.c:\n    .text start:0x00000000 end:0xffffffff\n",
                "src/example.c:\n    .text start:0x00000000 end:0xffffffff\n"
                f"    .bss start:{addr:#010x} end:{addr + 12:#010x}\n")
            path.write_text(text)

    def test_remove_section_maps_both_versions_and_replays_idempotently(self):
        self.add_bss()
        self.cli("remove-section", MODULE, "src/example.c", ".bss", "0x5010")
        for version in ("b2_us", "w2_us"):
            self.assertNotIn("    .bss", self.delinks(version))
            self.assertIn("src/example.c:\n    .text", self.delinks(version))
            self.assertIn("src/next.c:\n    .text", self.delinks(version))
        self.assertEqual(fixes.load_fixes(), [(MODULE, 0x5010, "remove_section", "src/example.c:.bss")])
        after = self.snapshot()
        self.cli("apply")
        self.cli("remove-section", MODULE, "src/example.c", ".bss", "0x5010")
        self.assertEqual(self.snapshot(), after)

    def test_remove_section_later_wrong_start_leaves_both_versions_unchanged(self):
        self.add_bss(other_start=0x6014)
        before = self.snapshot()
        with self.assertRaisesRegex(SystemExit, "does not start"):
            self.cli("remove-section", MODULE, "src/example.c", ".bss", "0x5010")
        self.assertEqual(self.snapshot(), before)

    def test_remove_section_missing_owner_is_rejected_without_changes(self):
        self.add_bss()
        before = self.snapshot()
        with self.assertRaisesRegex(SystemExit, "not registered"):
            self.cli("remove-section", MODULE, "src/missing.c", ".bss", "0x5010")
        self.assertEqual(self.snapshot(), before)


class DtcmMigrationTests(ConfigFixFixture):
    """Initialized data and zero-initialized state of a main SDK object belong in DTCM."""

    WHOLE = "lib/nitro/irq_table.c"

    def setUp(self):
        super().setUp()
        for addr in (0x7010, 0x7080, 0x7090):
            self.mapper.addresses[("dtcm", addr, "w2_us")] = addr + 0x800
        for version, main, dtcm in [("b2_us", 0x1000, 0x7000), ("w2_us", 0x2000, 0x7800)]:
            path = fixes.config_dir(version, ".") / "delinks.txt"
            path.write_text(path.read_text() + f"\n{self.WHOLE}:\n    complete\n"
                            f"    .text       start:{main:#010x} end:{main + 0x80:#010x}\n")
            self.sections(version, "dtcm", [(".data", dtcm, dtcm + 0x80),
                                            (".bss", dtcm + 0x80, dtcm + 0xa0)])

    def test_data_and_bss_use_their_dtcm_ranges_in_both_versions(self):
        self.cli("add-section", ".", self.WHOLE, ".dtcm.data", "0x20", "0x7010")
        self.cli("add-section", ".", self.WHOLE, ".dtcm.bss", "0x8", "0x7080")
        for version, dtcm in [("b2_us", 0x7000), ("w2_us", 0x7800)]:
            text = (fixes.config_dir(version, ".") / "delinks.txt").read_text()
            self.assertIn(f"    .dtcm.data  start:{dtcm + 0x10:#010x} end:{dtcm + 0x30:#010x}\n", text)
            self.assertIn(f"    .dtcm.bss   start:{dtcm + 0x80:#010x} end:{dtcm + 0x88:#010x}\n", text)
        self.assertIn(("dtcm", 0x7010, "w2_us"), self.mapper.calls)
        self.assertIn(("dtcm", 0x7080, "w2_us"), self.mapper.calls)

    def test_initialized_data_cannot_be_placed_in_dtcm_bss(self):
        before = self.snapshot()
        with self.assertRaisesRegex(SystemExit, "outside dtcm's .data"):
            self.cli("add-section", ".", self.WHOLE, ".dtcm.data", "0x8", "0x7090")
        self.assertEqual(self.snapshot(), before)

    def test_bss_size_separates_the_queue_from_module_alignment(self):
        for version, dtcm in [("b2_us", 0x7000), ("w2_us", 0x7800)]:
            path = fixes.config_dir(version, "dtcm") / "symbols.txt"
            path.write_text(f"IrqQueue kind:bss addr:{dtcm + 0x80:#010x}\n")
        self.cli("bss-size", "dtcm", "8", "0x7080")
        for version, dtcm in [("b2_us", 0x7000), ("w2_us", 0x7800)]:
            text = (fixes.config_dir(version, "dtcm") / "symbols.txt").read_text()
            self.assertEqual(text, f"IrqQueue kind:bss(size=0x8) addr:{dtcm + 0x80:#010x}\n")
        before = self.snapshot()
        self.cli("bss-size", "dtcm", "8", "0x7080")
        self.assertEqual(self.snapshot(), before)

    def test_bss_size_rejects_an_initialized_object(self):
        for version, dtcm in [("b2_us", 0x7000), ("w2_us", 0x7800)]:
            path = fixes.config_dir(version, "dtcm") / "symbols.txt"
            path.write_text(f"Table kind:data(any) addr:{dtcm + 0x80:#010x}\n")
        before = self.snapshot()
        with self.assertRaisesRegex(SystemExit, "expected one BSS symbol"):
            self.cli("bss-size", "dtcm", "8", "0x7080")
        self.assertEqual(self.snapshot(), before)

    def test_bss_size_cannot_leave_its_module_section(self):
        for version, dtcm in [("b2_us", 0x7000), ("w2_us", 0x7800)]:
            path = fixes.config_dir(version, "dtcm") / "symbols.txt"
            path.write_text(f"IrqQueue kind:bss addr:{dtcm + 0x80:#010x}\n")
        before = self.snapshot()
        with self.assertRaisesRegex(SystemExit, "leaves its module section"):
            self.cli("bss-size", "dtcm", "0x24", "0x7080")
        self.assertEqual(self.snapshot(), before)


class LtdMigrationTests(ConfigFixFixture):
    """A DSi part's entry in the LTD autoload moves into its whole's entry in ARM9 main as migrated sections."""

    PART = "lib/twl/part_twl.c"
    WHOLE = "lib/nitro/whole.c"

    def setUp(self):
        super().setUp()
        # White 2's LTD autoload sits 0x800 higher, so a migrated section's address is placed through that module
        for addr in (0x7010, 0x7410, 0x7420):
            self.mapper.addresses[(fixes.LTD_MODULE, addr, "w2_us")] = addr + 0x800
        for version, main, ltd in [("b2_us", 0x1000, 0x7000), ("w2_us", 0x2000, 0x7800)]:
            path = fixes.config_dir(version, ".") / "delinks.txt"
            path.write_text(path.read_text() + f"\n{self.WHOLE}:\n    complete\n"
                            f"    .text       start:{main:#010x} end:{main + 0x80:#010x}\n")
            self.sections(version, fixes.LTD_MODULE, [(".ltdmain", ltd, ltd + 0x400), (".bss", ltd + 0x400, ltd + 0x500)])
            path = fixes.config_dir(version, fixes.LTD_MODULE) / "delinks.txt"
            header = path.read_text().split("\n\n")[0]
            path.write_text(
                f"{header}\n\nlib/twl/before.c:\n    complete\n    .ltdmain    start:{ltd:#010x} end:{ltd + 0x10:#010x}\n"
                f"\n{self.PART}:\n    complete\n    .ltdmain    start:{ltd + 0x10:#010x} end:{ltd + 0x40:#010x}\n"
                f"    .bss        start:{ltd + 0x410:#010x} end:{ltd + 0x420:#010x}\n"
                f"\nlib/twl/after.c:\n    complete\n    .ltdmain    start:{ltd + 0x40:#010x} end:{ltd + 0x50:#010x}\n")

    def delinks(self, version, module):
        return (fixes.config_dir(version, module) / "delinks.txt").read_text()

    def test_migrated_sections_are_placed_through_the_ltd_autoload(self):
        self.cli("add-section", ".", self.WHOLE, ".ltdmain", "0x30", "0x7010")
        self.cli("add-section", ".", self.WHOLE, ".ltdmain.bss", "0x10", "0x7410")
        for version, main, ltd in [("b2_us", 0x1000, 0x7000), ("w2_us", 0x2000, 0x7800)]:
            self.assertIn(f"{self.WHOLE}:\n    complete\n    .text       start:{main:#010x} end:{main + 0x80:#010x}\n"
                          f"    .ltdmain    start:{ltd + 0x10:#010x} end:{ltd + 0x40:#010x}\n"
                          f"    .ltdmain.bss start:{ltd + 0x410:#010x} end:{ltd + 0x420:#010x}\n",
                          self.delinks(version, "."))
        self.assertIn((fixes.LTD_MODULE, 0x7410, "w2_us"), self.mapper.calls)
        after = self.snapshot()
        self.cli("apply")
        self.assertEqual(self.snapshot(), after)

    def test_migrated_section_outside_its_ltd_section_is_rejected(self):
        self.mapper.addresses[(fixes.LTD_MODULE, 0x7020, "w2_us")] = 0x7820
        # .ltdmain.bss lies in the LTD autoload's .bss, not in its .ltdmain
        self.assert_rejected_without_changes(
            lambda: self.cli("add-section", ".", self.WHOLE, ".ltdmain.bss", "0x10", "0x7020"), "outside")

    def test_remove_file_deletes_the_entry_in_both_versions_and_replays(self):
        self.cli("section-end", fixes.LTD_MODULE, self.PART, ".bss", "0x7420")
        self.cli("remove-file", fixes.LTD_MODULE, self.PART, "0x7010")
        for version, ltd in [("b2_us", 0x7000), ("w2_us", 0x7800)]:
            self.assertIn(f"end:{ltd + 0x10:#010x}\n\nlib/twl/after.c:\n", self.delinks(version, fixes.LTD_MODULE))
            self.assertNotIn(self.PART, self.delinks(version, fixes.LTD_MODULE))
        # The section fix of the removed entry would name an entry that is gone
        self.assertEqual(fixes.load_fixes(), [(fixes.LTD_MODULE, 0x7010, "remove_file", self.PART)])
        after = self.snapshot()
        self.cli("apply")
        self.assertEqual(self.snapshot(), after)

    def test_remove_file_rejects_an_address_not_of_its_sections(self):
        self.mapper.addresses[(fixes.LTD_MODULE, 0x7020, "w2_us")] = 0x7820
        self.assert_rejected_without_changes(
            lambda: self.cli("remove-file", fixes.LTD_MODULE, self.PART, "0x7020"), "no section at")

    def test_remove_file_rejects_an_unregistered_file(self):
        self.assert_rejected_without_changes(
            lambda: self.cli("remove-file", fixes.LTD_MODULE, "lib/twl/missing.c", "0x7010"), "not registered")


if __name__ == "__main__":
    unittest.main()

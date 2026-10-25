"""Nonmatching-row regressions without repository configs or ROMs.

    python3 -m unittest tools.decomp.test_check_nonmatching_rows
"""
import contextlib
import io
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.decomp import check_nonmatching_rows as rows


def row(names="`first`", addresses="`0x100` / `0x200`", path="src/example.c"):
    return f"| `{path}` | {names} | {addresses} | Open. |"


class NonmatchingRowsTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        self.doc = self.root / "docs/nonmatching-functions.md"
        self.doc.parent.mkdir()
        source = self.root / "src/example.c"
        source.parent.mkdir()
        source.write_text("void first(void)\n{\n}\nvoid second(void)\n{\n}\n")
        for attribute, value in (("ROOT", self.root), ("DOC", self.doc)):
            replacement = patch.object(rows, attribute, value)
            replacement.start()
            self.addCleanup(replacement.stop)

    def run_checker(self, text, fix=False, shared_aliases=False, divergent_white=False, migrated=False):
        self.doc.write_text(text)
        output = io.StringIO()
        symbols = {".": {"first": {0x100}, "second": {0x110}}}
        white = {".": {"first": {0x200}, "second": {0x210}}}
        if shared_aliases:
            symbols["."]["second"] = {0x100}
            white["."]["second"] = {0x200}
        if divergent_white:
            white["."]["second"] = {0x210}
        if migrated:
            # The DSi part of an ARM9 main file, in a migrated .ltdmain section, has its symbols in the LTD autoload
            symbols["ltd_autoload_0"] = {"second": {symbols["."].pop("second").pop()}}
            white["ltd_autoload_0"] = {"second": {white["."].pop("second").pop()}}
        with patch.object(rows, "modules_by_file", return_value={"src/example.c": "."}), \
                patch.object(rows, "symbols_by_module", side_effect=[symbols, white]), \
                patch.object(rows, "migrated_modules",
                             return_value={"src/example.c": {"ltd_autoload_0"}} if migrated else {}), \
                patch.object(rows.sys, "argv", ["check_nonmatching_rows.py"] + (["--fix"] if fix else [])), \
                contextlib.redirect_stdout(output), self.assertRaises(SystemExit) as result:
            rows.main()
        return result.exception.code, output.getvalue()

    def test_all_merge_markers_fail_even_when_unpaired(self):
        for marker in ("<<<<<<< HEAD", "||||||| d9ac5a6cb6", "=======", ">>>>>>> topic"):
            with self.subTest(marker=marker):
                code, output = self.run_checker(row() + "\n" + marker)
                self.assertEqual(code, 1)
                self.assertIn("docs/nonmatching-functions.md:2: unresolved merge marker", output)

    def test_observed_diff3_ancestor_and_repeated_rows_fail(self):
        # The damaged document retained an ancestor marker and nine otherwise valid repeated rows.
        entries = [row(f"`function{index}`") for index in range(9)]
        problems = rows.structural_problems(entries + ["||||||| d9ac5a6cb6"] + entries)
        self.assertEqual(len(problems), 10)
        self.assertIn("unresolved merge marker", problems[0])
        self.assertEqual(sum("duplicate function" in problem for problem in problems), 9)

    def test_grouped_and_single_rows_share_function_identity(self):
        grouped = row("`first`, `second`", "`0x100` / `0x200`, `0x110` / `0x210`")
        code, output = self.run_checker(grouped + "\n" + row("`second`", "`0x110` / `0x210`"))
        self.assertEqual(code, 1)
        self.assertIn("duplicate function second (first listed on line 1)", output)

    def test_distinct_functions_files_and_data_rows_are_independent(self):
        lines = [row(), row("`second`", "`0x110` / `0x210`"), row(path="src/other.c"),
                 row("(file `.data`)"), row("(file `.data`)")]
        self.assertEqual(rows.structural_problems(lines), [])
        code, output = self.run_checker("\n".join(lines[:2] + lines[3:]))
        self.assertEqual((code, output), (0, ""))

    def test_fix_repairs_address_but_preserves_structural_failure(self):
        for extra in ("||||||| d9ac5a6cb6", row()):
            with self.subTest(extra=extra):
                code, output = self.run_checker(row(addresses="`0x100` / `0x999`") + "\n" + extra, fix=True)
                self.assertEqual(code, 1)
                self.assertIn("fixed 1 White 2 address", output)
                self.assertNotIn("0x999", self.doc.read_text())

    def test_fix_valid_address_repair_succeeds(self):
        code, output = self.run_checker(row(addresses="`0x100` / `0x999`"), fix=True)
        self.assertEqual(code, 0)
        self.assertIn("fixed 1 White 2 address", output)
        self.assertIn("`0x100` / `0x00000200`", self.doc.read_text())

    def test_migrated_section_functions_are_found_in_the_ltd_autoload(self):
        code, output = self.run_checker(row("`first`, `second`", "`0x100` / `0x200`, `0x110` / `0x210`"), migrated=True)
        self.assertEqual(code, 0, output)
        code, output = self.run_checker(row("`second`", "`0x120` / `0x210`"), migrated=True)
        self.assertEqual(code, 1)
        self.assertIn("second is not at 0x00000120 in b2_us", output)

    def test_shared_pair_checks_every_named_function(self):
        code, output = self.run_checker(row("`first`, `second`"))
        self.assertEqual(code, 1)
        self.assertIn("second is not at 0x00000100 in b2_us", output)
        self.assertIn("second is not at 0x00000200 in w2_us", output)

    def test_shared_pair_accepts_aliases_at_the_same_addresses(self):
        code, output = self.run_checker(row("`first`, `second`"), shared_aliases=True)
        self.assertEqual((code, output), (0, ""))

    def test_grouped_row_checks_corresponding_pairs(self):
        code, output = self.run_checker(row("`first`, `second`", "`0x100` / `0x200`, `0x110` / `0x210`"))
        self.assertEqual((code, output), (0, ""))

    def test_fix_refuses_divergent_shared_white_addresses(self):
        text = row("`first`, `second`", "`0x100` / `0x999`")
        code, output = self.run_checker(text, fix=True, shared_aliases=True, divergent_white=True)
        self.assertEqual(code, 1)
        self.assertNotIn("(fixed)", output)
        self.assertEqual(self.doc.read_text(), text)

    def test_successful_fixes_pass_fresh_validation(self):
        cases = [
            (row(addresses="`0x100` / `0x999`"), False),
            (row(addresses="`0x100` / `0x100`"), False),
            (row("`first`, `second`", "`0x100` / `0x999`"), True),
            (row("`first`, `second`", "`0x100` / `0x100`"), True),
            (row("`first`, `second`", "`0x100` / `0x999`, `0x110` / `0x998`"), False),
        ]
        for text, aliases in cases:
            with self.subTest(text=text):
                code, _ = self.run_checker(text, fix=True, shared_aliases=aliases)
                self.assertEqual(code, 0)
                repaired = self.doc.read_text()
                code, output = self.run_checker(repaired, shared_aliases=aliases)
                self.assertEqual((code, output), (0, ""))


if __name__ == "__main__":
    unittest.main()

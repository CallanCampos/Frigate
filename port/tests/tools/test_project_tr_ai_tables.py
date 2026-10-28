#!/usr/bin/env python3
"""Validate fail-closed source selection for actual src/ov170/tr_ai.c command/VM regression compilation."""
import importlib.util
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[3]
TOOL = ROOT / "port/tools/project_tr_ai_tables.py"
spec = importlib.util.spec_from_file_location("projection", TOOL)
projection = importlib.util.module_from_spec(spec)
spec.loader.exec_module(projection)


def fixture(body="return 1;"):
    return ("typedef int BOOL;\nstatic const VMCommand sTrAICommands[] = {unused};\n" +
            "\n".join(f"BOOL {name}(void) {{ {body} }}" for name in projection.FUNCTIONS))


class ProjectionTests(unittest.TestCase):
    def test_actual_bodies_retained_and_modified_source_propagates(self):
        source = (ROOT / "src/ov170/tr_ai.c").read_text()
        output = projection.project(source, "src/ov170/tr_ai.c")
        self.assertNotIn("sTrAICommands[]", output)
        self.assertNotIn("static TrAIScriptCache *sScriptCache;", output)
        self.assertIn("} TrAIContext;", output)
        # Independently delimit these top-level definitions by the next source definition/comment boundary.
        for name in projection.FUNCTIONS:
            start = re.search(rf"(?m)^(?:BOOL|s32) {name}\([^;]*?\) \{{", source).start()
            end = source.index("\n}", start) + 2
            self.assertIn(source[start:end], output)
        changed = source.replace("s32 tableOffset = VM_Read32(vm);", "s32 tableOffset = VM_Read32(vm) + 0;")
        self.assertIn("s32 tableOffset = VM_Read32(vm) + 0;", projection.project(changed, "changed.c"))

    def test_crlf_definition_and_escaped_continuations_are_retained(self):
        body = '// ignored }\\\r\n still comment }\r\nconst char *s = "left\\\r\nright }"; return 1;'
        source = fixture(body).replace(";\nstatic", ";\r\nstatic")
        definition = f"BOOL AIEnd(void) {{ {body} }}"
        self.assertIn(definition, projection.project(source, "owner.c"))
        with tempfile.TemporaryDirectory() as folder:
            src, output = Path(folder) / "source.c", Path(folder) / "out.inc"
            src.write_bytes(source.encode())
            subprocess.run([sys.executable, str(TOOL), "--source", str(src), "--out", str(output)], check=True)
            self.assertIn(definition.encode(), output.read_bytes())

    def test_comments_strings_characters_and_escaped_quotes_do_not_close_body(self):
        body = r'''/* } */ const char *s = "escaped \" }"; char c = '}'; // }
        if (s) { return c; }'''
        source = fixture(body)
        output = projection.project(source, "owner.c")
        self.assertEqual(output.count(body), len(projection.FUNCTIONS))

    def test_missing_duplicate_and_unclosed_definitions_fail(self):
        source = fixture()
        cases = [source.replace("AIEnd", "Absent"), source + "\nBOOL AIEnd(void) { return 0; }",
                 source[:-1], source.replace("sTrAICommands", "MissingTable")]
        for text in cases:
            with self.subTest(text=text[-70:]), self.assertRaises(ValueError):
                projection.project(text, "owner.c")

    def test_unterminated_literals_comments_and_fake_definitions_fail(self):
        for tail in ['\n/*', '\n"unterminated', "\n'broken", '\n"line\nbreak"', '\n"raw\rCR"']:
            with self.subTest(tail=tail), self.assertRaises(ValueError):
                projection.project(fixture() + tail, "owner.c")
        with self.assertRaises(ValueError):
            projection.project(fixture().replace("BOOL AIEnd(void) { return 1; }", '// BOOL AIEnd(void) { return 1; }'),
                               "owner.c")

    def test_cli_failure_preserves_previous_output(self):
        with tempfile.TemporaryDirectory() as folder:
            source, output = Path(folder) / "owner.c", Path(folder) / "generated.inc"
            source.write_text(fixture())
            subprocess.run([sys.executable, str(TOOL), "--source", str(source), "--out", str(output)], check=True)
            before = output.read_bytes()
            source.write_text("malformed")
            result = subprocess.run([sys.executable, str(TOOL), "--source", str(source), "--out", str(output)],
                                    capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(output.read_bytes(), before)


if __name__ == "__main__":
    unittest.main()

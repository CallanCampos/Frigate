"""Effective compiler settings and their command consumers, without invoking MWCC."""
import contextlib
import io
import json
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import configure
from tools.decomp import compile_all, compiler_probe
from tools.build import objdiff_config


class SourceCompilerTests(unittest.TestCase):
    def test_game_library_and_external_settings(self):
        cases = [
            ("src/ov257/ctvt_draw.c", configure.MWCC_VERSION, False, "-ipa function"),
            ("src/unlisted.c", configure.MWCC_VERSION, False, "-ipa file"),
            ("lib/nnsys/src/snd/snd_capture.c", None, True, "-ipa function"),
            ("lib/nitro/src/card/card_pull_out.c", None, True, "-thumb"),
            ("lib/twl/src/syscall/syscall_twl_ltd.c", "2.0/sp2p2", True, "-ipa file"),
        ]
        for source, compiler, library, flag in cases:
            with self.subTest(source=source):
                settings = configure.source_compiler(Path(source))
                self.assertEqual(settings.is_library, library)
                self.assertIn(flag, settings.flags)
                if compiler:
                    self.assertEqual(settings.compiler, compiler)
                else:
                    self.assertEqual(settings.compiler, configure.library_of(Path(source))[0])
        with tempfile.TemporaryDirectory() as tmp:
            external = Path(tmp) / "ctvt_draw.c"
            self.assertEqual(configure.source_compiler(external), configure.SourceCompiler(
                configure.MWCC_VERSION, tuple(configure.CC_FLAGS), False))
            external.write_text("")
            repository = Path(tmp) / "repository"
            repository.mkdir()
            link = repository / "ctvt_draw.c"
            link.symlink_to(external)
            with patch.object(configure, "ROOT", repository), \
                    patch.dict(configure.FILE_FLAGS, {"ctvt_draw.c": ["-ipa function"]}):
                self.assertIsNone(configure.library_of(link))
                self.assertEqual(configure.source_compiler(link).flags, tuple(configure.CC_FLAGS))

    def test_flag_replacement_does_not_mutate_defaults(self):
        original = ["-O4,p", "-thumb", "-ipa file", "-sym on"]
        source = Path("src/ov257/ctvt_draw.c")
        with patch.dict(configure.FILE_FLAGS, {source.as_posix(): ["-O2,p", "-nothumb", "-ipa function"]}):
            self.assertEqual(configure.file_flags(source, original),
                             ["-sym on", "-O2,p", "-nothumb", "-ipa function"])
        self.assertEqual(original, ["-O4,p", "-thumb", "-ipa file", "-sym on"])
        settings = configure.source_compiler(source)
        with self.assertRaises(AttributeError):
            settings.compiler = "changed"
        self.assertIn("-ipa file", configure.CC_FLAGS)

    def test_manifest_rejects_invalid_entries(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest = Path(tmp) / "flags.toml"
            cases = [
                'other = {}',
                '[file_flags]\n"." = ["-ipa function"]',
                '[file_flags]\n"" = ["-ipa function"]',
                '[file_flags]\n"../src/x.c" = ["-ipa function"]',
                '[file_flags]\n"/src/x.c" = ["-ipa function"]',
                '[file_flags]\n"src/missing.c" = ["-ipa function"]',
                '[file_flags]\n"src/ov257/ctvt_draw.c" = "-ipa function"',
                '[file_flags]\n"src/ov257/ctvt_draw.c" = ["-thumb", "-nothumb"]',
                '[file_flags]\n"src/ov257/ctvt_draw.c" = ["-ipa function -thumb"]',
                '[file_flags]\n"src/ov257/ctvt_draw.c" = [""]',
            ]
            for text in cases:
                with self.subTest(text=text):
                    manifest.write_text(text)
                    with self.assertRaises(ValueError):
                        configure.load_source_flags(manifest)

    def test_compile_all_command_uses_effective_build_settings(self):
        sources = ["src/unlisted.c", "src/ov257/ctvt_draw.c", "lib/nnsys/src/snd/snd_capture.c"]
        for source in map(Path, sources):
            for version in configure.VERSIONS:
                with self.subTest(source=source, version=version), tempfile.TemporaryDirectory() as tmp, \
                        patch.object(compile_all.subprocess, "run", return_value=subprocess.CompletedProcess([], 0, "", "")) as run, \
                        patch.object(compile_all, "mwcc_command", return_value=["compiler"]) as command:
                    compile_all.compile_file(source, version, Path(tmp))
                    settings = configure.source_compiler(source)
                    command.assert_called_once_with(compile_all.TOOLS / "mwccarm" / settings.compiler / "mwccarm.exe")
                    args = run.call_args.args[0]
                    expected = shlex.split(" ".join(settings.flags))
                    self.assertEqual(args[1:1 + len(expected)], expected)
                    self.assertIn(configure.VERSIONS[version]["defines"][0], args)
                    self.assertIn("SJIS", args)
                    self.assertIn("include", args)
                    if settings.is_library:
                        self.assertEqual(run.call_args.kwargs["env"]["MWCIncludes"], ".")
                    compiler, flags = compiler_probe.probe_defaults(source)
                    self.assertEqual(compiler, settings.compiler if settings.is_library else None)
                    self.assertIn("-w off", flags)
                    self.assertEqual("-sym on" in flags, settings.is_library and "-sym on" in settings.flags)
                    self.assertEqual("-ipa function" in flags, "-ipa function" in settings.flags)

    def test_probe_game_defaults_to_all_and_sdk_to_own_compiler(self):
        for source in ["src/unlisted.c", "src/ov257/ctvt_draw.c", "lib/nnsys/src/snd/snd_capture.c"]:
            with self.subTest(source=source), \
                    patch.object(sys, "argv", ["probe", source]), \
                    patch.object(Path, "iterdir", return_value=[Path("first"), Path("second")]), \
                    patch.object(compiler_probe, "load_modules", return_value={}), \
                    patch.object(compiler_probe, "mwcc_command", return_value=["compiler"]) as command, \
                    patch.object(compiler_probe.subprocess, "run", return_value=subprocess.CompletedProcess([], 1, "", "")), \
                    contextlib.redirect_stdout(io.StringIO()):
                compiler_probe.main()
                names = [str(call.args[0]) for call in command.call_args_list]
                if source.startswith("src/"):
                    self.assertEqual(len(names), 2)
                    self.assertTrue(names[0].endswith("dsi/first/mwccarm.exe"))
                    self.assertTrue(names[1].endswith("dsi/second/mwccarm.exe"))
                else:
                    self.assertEqual(names, [str(compiler_probe.TOOLS / "mwccarm" /
                        configure.source_compiler(Path(source)).compiler / "mwccarm.exe")])

    def test_probe_cli_defaults_and_explicit_precedence(self):
        cases = [
            ([], "-ipa function"),
            (["--flags="], None),
            (["--flags=-O4,p -ipa file", "--opt=-O2,p", "--extra-flags=-ipa function"], "-O2,p"),
        ]
        for extra, expected in cases:
            with self.subTest(extra=extra), \
                    patch.object(sys, "argv", ["probe", "src/ov257/ctvt_draw.c", "--compilers=custom/compiler", *extra]), \
                    patch.object(compiler_probe, "load_modules", return_value={}), \
                    patch.object(compiler_probe, "mwcc_command", return_value=["compiler"]), \
                    patch.object(compiler_probe.subprocess, "run", return_value=subprocess.CompletedProcess([], 1, "", "")) as run, \
                    contextlib.redirect_stdout(io.StringIO()):
                compiler_probe.main()
                args = run.call_args.args[0]
                if expected:
                    self.assertTrue(all(word in args for word in shlex.split(expected)))
                else:
                    self.assertEqual(args[1:3], ["-d", "BLACK2"])
                if "--opt=-O2,p" in extra:
                    self.assertNotIn("-O4,p", args)
                    self.assertEqual(args[args.index("-ipa") + 1], "file")
                    self.assertEqual(args[args.index("-ipa", args.index("-ipa") + 1) + 1], "function")

    def test_objdiff_updates_only_overridden_game_scratch(self):
        units = [{"name": source, "metadata": {"source_path": source},
                  "scratch": {"compiler": "mwcc_40_1024", "c_flags": "explicit defaults"}}
                 for source in ["src/ov257/ctvt_draw.c", "src/unlisted.c", "lib/nnsys/src/snd/snd_capture.c"]]
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "objdiff.json"
            with patch.object(sys, "argv", ["objdiff", "b2_us", "--dsd=dsd", "--compiler=mwcc_40_1024", "--c-flags=explicit defaults", "-o", str(output)]), \
                    patch.object(objdiff_config.subprocess, "run", return_value=subprocess.CompletedProcess([], 0, json.dumps({"units": units}), "")), \
                    patch.object(objdiff_config, "source_modules", return_value={}), \
                    patch.object(Path, "exists", return_value=True):
                objdiff_config.main()
            scratches = [unit["scratch"] for unit in json.loads(output.read_text())["units"]]
            self.assertIn("-ipa function", scratches[0]["c_flags"])
            self.assertNotIn("-enc", scratches[0]["c_flags"])
            self.assertEqual(scratches[0]["compiler"], "mwcc_40_1024")
            self.assertEqual([item["c_flags"] for item in scratches[1:]], ["explicit defaults"] * 2)


if __name__ == "__main__":
    unittest.main()

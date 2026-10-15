"""Compile-only diagnostics tests use temporary sources and a fake compiler."""
import contextlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from tools.decomp import compile_all


class CompileAllTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.outputs = self.root / 'outputs.json'
        self.compiler = self.root / 'compiler.py'
        self.compiler.write_text(
            'import json, pathlib, sys\n'
            'args = sys.argv[2:]\n'
            'version = args[args.index("-d") + 1]\n'
            'record = json.loads(pathlib.Path(sys.argv[1]).read_text())[args[-1] + ":" + version]\n'
            'sys.stdout.write(record[1])\n'
            'sys.stderr.write(record[2])\n'
            'sys.exit(record[0])\n')
        values = {
            'ROOT': self.root,
            'TOOLS': self.root / 'tools',
            'VERSIONS': {'b2_us': {'defines': ['BLACK2']}, 'w2_us': {'defines': ['WHITE2']}},
            'CC_FLAGS': ['-default flag'],
            'MWCC_VERSION': 'default/compiler',
            'INCLUDE_DIRS': ['include'],
        }
        for name, value in values.items():
            patcher = patch.object(compile_all, name, value)
            patcher.start()
            self.addCleanup(patcher.stop)
        for name in ('download_tools', 'generate_constants'):
            patcher = patch.object(compile_all, name)
            mock = patcher.start()
            self.addCleanup(patcher.stop)
            setattr(self, name, mock)
        patcher = patch.object(compile_all, 'library_of', return_value=None)
        patcher.start()
        self.addCleanup(patcher.stop)
        patcher = patch.object(compile_all, 'mwcc_command', return_value=[
            sys.executable, str(self.compiler), str(self.outputs)])
        patcher.start()
        self.addCleanup(patcher.stop)

    def run_main(self, records, versions=None):
        for source, _ in records:
            path = self.root / source
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text('int example(void) { return 0; }\n')
        self.outputs.write_text(json.dumps({
            source + ':' + version: output for (source, version), output in records.items()}))
        argv = ['compile_all.py', '-j', '4']
        for version in versions or []:
            argv.extend(['--version', version])
        stdout, stderr = io.StringIO(), io.StringIO()
        with patch.object(sys, 'argv', argv), contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            with self.assertRaises(SystemExit) as raised:
                compile_all.main()
        self.download_tools.assert_called_with(self.root / 'tools')
        self.generate_constants.assert_called_with()
        return raised.exception.code, stdout.getvalue(), stderr.getvalue()

    def test_success_diagnostics_preserve_streams_and_do_not_fail(self):
        for out, err in [('', ''), ('stdout warning\n', ''), ('', 'stderr warning\n'),
                         ('stdout warning', 'stderr warning')]:
            with self.subTest(stdout=out, stderr=err):
                code, stdout, stderr = self.run_main({('src/example.c', 'BLACK2'): (0, out, err)}, ['b2_us'])
                self.assertEqual(code, 0)
                self.assertIn('1 of 1 compiled (1 files, b2_us)', stdout)
                for payload, own, other in [(out, stdout, stderr), (err, stderr, stdout)]:
                    if payload:
                        self.assertEqual(own.count(payload), 1)
                        self.assertNotIn(payload, other)
                        self.assertIn('src/example.c (b2_us):\n' + payload, own)
                if not err:
                    self.assertEqual(stderr, '')
                if not out:
                    self.assertNotIn('src/example.c', stdout)

    def test_failure_diagnostics_and_silent_failure(self):
        for out, err in [('compiler output\n', 'compiler error\n'), ('', '')]:
            with self.subTest(stdout=out, stderr=err):
                code, stdout, stderr = self.run_main({('src/example.c', 'BLACK2'): (7, out, err)}, ['b2_us'])
                self.assertEqual(code, 1)
                self.assertIn('0 of 1 compiled', stdout)
                if out:
                    self.assertIn('src/example.c (b2_us):\n' + out, stdout)
                    self.assertEqual(stdout.count(out), 1)
                    self.assertIn('src/example.c (b2_us):\n' + err, stderr)
                    self.assertEqual(stderr.count(err), 1)
                else:
                    self.assertIn('src/example.c (b2_us):', stderr)
                    self.assertIn('compiler exited with status 7', stderr)

    def test_concurrent_sources_and_versions_have_no_diagnostic_loss(self):
        records = {}
        for source in ('src/one.c', 'lib/example/src/two.c', 'src/sub/three.c'):
            for version in ('BLACK2', 'WHITE2'):
                token = source + '/' + version
                records[source, version] = (1 if token.endswith('three.c/WHITE2') else 0,
                                            'OUT:' + token + '\n', 'ERR:' + token + '\n')
        code, stdout, stderr = self.run_main(records)
        self.assertEqual(code, 1)
        self.assertIn('5 of 6 compiled (3 files, b2_us, w2_us)', stdout)
        for (source, define), (_, out, err) in records.items():
            version = 'b2_us' if define == 'BLACK2' else 'w2_us'
            self.assertEqual(stdout.count(out), 1)
            self.assertEqual(stderr.count(err), 1)
            self.assertNotIn(out, stderr)
            self.assertNotIn(err, stdout)
            self.assertIn(f'{source} ({version}):\n{out}', stdout)
            self.assertIn(f'{source} ({version}):\n{err}', stderr)

    def test_command_selection_environment_and_result_record(self):
        process = subprocess.CompletedProcess([], 0, 'warning\n', 'diagnostic\n')
        ambient = {'MWCIncludes': 'ambient/includes', 'COMPILE_TEST_SENTINEL': 'retained'}
        cases = [
            (Path('lib/example/src/file.c'), ('library/compiler', ['-library "two words"']),
             'library/compiler', ['-library', 'two words'], dict(ambient, MWCIncludes='.')),
            (Path('src/file.c'), None, 'default/compiler', ['-default', 'flag'], ambient),
        ]
        for source, library, compiler, flags, expected_env in cases:
            with self.subTest(source=source), patch.dict(os.environ, ambient, clear=True), \
                    patch.object(compile_all, 'library_of', return_value=library), \
                    patch.object(compile_all, 'mwcc_command', return_value=['compiler']) as command, \
                    patch.object(compile_all.subprocess, 'run', return_value=process) as run:
                result = compile_all.compile_file(source, 'w2_us', self.root / 'objects')
                command.assert_called_once_with(self.root / 'tools/mwccarm' / compiler / 'mwccarm.exe')
                expected_obj = self.root / 'objects/w2_us' / source.with_suffix('.o')
                run.assert_called_once_with([
                    'compiler', *flags, '-d', 'WHITE2', '-i', 'include',
                    '-o', str(expected_obj), str(source)], cwd=self.root, env=expected_env,
                    capture_output=True, text=True)
                self.assertEqual(dict(os.environ), ambient)
                self.assertIsNot(run.call_args.kwargs['env'], os.environ)
                self.assertEqual((result.source, result.version, result.returncode, result.stdout, result.stderr),
                                 (source, 'w2_us', 0, 'warning\n', 'diagnostic\n'))


if __name__ == '__main__':
    unittest.main()

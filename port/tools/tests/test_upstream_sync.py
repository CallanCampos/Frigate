"""Isolated Git fixtures for Frigate upstream PR sync; no network or ROMs.

Exercises preservation of native port changes and the upstream history/reference
boundaries implemented by port/tools/upstream_sync.py.
"""
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

MODULE = Path(__file__).resolve().parents[1] / 'upstream_sync.py'
spec = importlib.util.spec_from_file_location('upstream_sync', MODULE)
sync = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sync)


class GitFixture(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.git('init', '-b', 'main')
        self.git('config', 'user.email', 'fixture@example.invalid')
        self.git('config', 'user.name', 'Fixture')
        self.write('game.c', 'original\n')
        self.git('add', 'game.c')
        self.git('commit', '-m', 'Base')
        self.base = self.git('rev-parse', 'HEAD')
        self.git('branch', 'upstream')
        self.write('port/native.c', 'native port\n')
        self.git('add', 'port')
        self.git('commit', '-m', 'Native port')

    def tearDown(self):
        self.temp.cleanup()

    def git(self, *args):
        return subprocess.check_output(['git', '-C', str(self.root), *args], text=True, stderr=subprocess.DEVNULL).strip()

    def write(self, path, text):
        target = self.root / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text)

    def incoming(self, message='Upstream update', conflict=False):
        self.git('checkout', 'upstream')
        self.write('game.c', 'upstream\n')
        self.git('add', '.')
        self.git('commit', '-m', message)
        incoming = self.git('rev-parse', 'HEAD')
        self.git('checkout', 'main')
        if conflict:
            self.write('game.c', 'port game edit\n')
            self.git('add', '.')
            self.git('commit', '-m', 'Port game edit')
        return incoming

    def prepare(self, incoming):
        real_git = self.git
        def fake_git(*args):
            if args[0] == 'fetch':
                real_git('fetch', '.', 'upstream')
                return ''
            return real_git(*args)
        real_run = subprocess.run
        def in_repo(args, **kwargs):
            return real_run(args, cwd=self.root, **kwargs)
        with patch.object(sync, 'git', side_effect=fake_git), patch.object(sync, 'pending', return_value=[]), patch.object(sync, 'publish') as published, patch.object(sync.subprocess, 'run', side_effect=in_repo):
            sync.prepare()
            return published.call_args.args[0]


class SyncTests(GitFixture):
    def test_real_merge_preserves_port_and_two_parents(self):
        incoming = self.incoming()
        before = self.git('rev-parse', 'main')
        branch = self.prepare(incoming)
        self.assertTrue(branch.startswith(sync.PREFIX))
        self.assertEqual(self.git('rev-parse', 'HEAD^1'), before)
        self.assertEqual(self.git('rev-parse', 'HEAD^2'), incoming)
        self.assertEqual((self.root / 'port/native.c').read_text(), 'native port\n')
        self.assertEqual(self.git('rev-parse', 'main'), before)

    def test_conflict_never_publishes(self):
        with self.assertRaises(subprocess.CalledProcessError):
            self.prepare(self.incoming(conflict=True))

    def test_reference_stops_before_merge(self):
        with self.assertRaisesRegex(RuntimeError, 'reference'):
            self.prepare(self.incoming('See another/project#42'))
        self.assertEqual(self.git('branch', '--show-current'), 'main')

    def test_rewrite_stops(self):
        incoming = self.incoming()
        before = self.git('rev-parse', 'HEAD')
        merge = self.git('commit-tree', 'HEAD^{tree}', '-p', before, '-p', incoming,
                         '-m', 'Merge upstream fuddlesworth/pokebw2@' + incoming[:9] + '\n\nUpstream-Commit: ' + incoming)
        # GitHub's PR merge wraps the actual sync merge on its second parent.
        wrapper = self.git('commit-tree', 'HEAD^{tree}', '-p', before, '-p', merge,
                           '-m', 'Merge pull request from sync topic')
        self.git('reset', '--hard', wrapper)
        self.git('branch', '-f', 'upstream', self.base)
        with self.assertRaisesRegex(RuntimeError, 'rewritten'):
            self.prepare(self.base)

    def test_invalid_destinations(self):
        for branch in ['main', 'refs/heads/main', 'topic', 'codex/upstream-sync-../main']:
            with self.subTest(branch=branch), self.assertRaises((RuntimeError, subprocess.CalledProcessError)):
                sync.validate_branch(branch)

    def test_workflow_changes_stay_local(self):
        self.git('checkout', 'upstream')
        self.write('.github/workflows/changed.yml', 'name: incoming\n')
        self.git('add', '.')
        self.git('commit', '-m', 'Workflow update')
        incoming = self.git('rev-parse', 'HEAD')
        self.git('checkout', 'main')
        with self.assertRaisesRegex(RuntimeError, 'workflow changes'):
            self.prepare(incoming)

    def test_publish_uses_topic_only_without_force(self):
        branch = sync.PREFIX + 'fixture'
        with patch.object(sync, 'git', return_value='a' * 40), patch.object(sync, 'pending', return_value=[]), patch.object(sync, 'gh', return_value='created'), patch.object(sync, 'run', return_value='CallanCampos'), patch.object(sync.subprocess, 'run') as commands:
            sync.publish(branch)
            commands.assert_called_once_with(['git', 'push', 'origin', 'HEAD:refs/heads/' + branch], check=True)

    def test_app_publisher_never_requests_status_or_user_privileges(self):
        with patch.dict(os.environ, {'GITHUB_ACTIONS': 'true'}), patch.object(sync, 'git'), patch.object(sync, 'pending', return_value=[]), patch.object(sync, 'gh', return_value='created') as gh, patch.object(sync, 'run', side_effect=AssertionError('unexpected API call')), patch.object(sync.subprocess, 'run'):
            sync.publish(sync.PREFIX + 'app-fixture')
            create = gh.call_args.args
            self.assertEqual(create[:2], ('pr', 'create'))
            self.assertIn('--reviewer', create)
            self.assertNotIn('--approve', create)

    def test_workflow_uses_repository_scoped_revoked_app_token(self):
        workflow = (MODULE.parents[2] / '.github/workflows/port-sync.yml').read_text()
        token_inputs = workflow.split('id: sync-token', 1)[1].split('      - name:', 1)[0]
        self.assertIn('owner: CallanCampos', token_inputs)
        self.assertIn('repositories: Frigate', token_inputs)
        self.assertIn('permission-contents: write', token_inputs)
        self.assertIn('permission-pull-requests: write', token_inputs)
        self.assertEqual(token_inputs.count('permission-'), 2)
        self.assertIn('skip-token-revoke: false', token_inputs)
        self.assertIn('No default-token fallback is allowed.', workflow)
        self.assertNotIn('github.token', workflow)
        publishing = workflow.split('name: Prepare upstream PR using trusted main helper', 1)[1]
        self.assertIn('GH_TOKEN: ${{ steps.sync-token.outputs.token }}', publishing)
        self.assertNotIn('secrets.', publishing)
        self.assertNotIn('statuses:', workflow)

    def test_review_rule_keeps_human_review_without_solo_push_deadlock(self):
        rules = json.loads((MODULE.parents[2] / 'port/tools/reviews-ruleset.json').read_text())
        review = next(rule['parameters'] for rule in rules['rules'] if rule['type'] == 'pull_request')
        self.assertFalse(review['require_last_push_approval'])
        self.assertTrue(review['dismiss_stale_reviews_on_push'])
        self.assertTrue(review['require_code_owner_review'])
        self.assertEqual(review['required_approving_review_count'], 1)
        self.assertEqual(rules['bypass_actors'], [{'actor_id': 145432210, 'actor_type': 'User', 'bypass_mode': 'pull_request'}])

    def test_open_sync_is_frozen(self):
        with patch.object(sync, 'pending', return_value=[{'url': 'pending'}]), patch.object(sync, 'git') as git:
            sync.prepare()
            git.assert_not_called()


class ShellVerificationTests(GitFixture):
    def fixture(self):
        for name in ('sync_upstream.sh', 'upstream_sync.py'):
            self.write('port/tools/' + name, (MODULE.parent / name).read_text())
        self.write('.gitignore', 'orig/\nbuild/\n*.sha1\n')
        self.write('configure.py', 'import os, sys\nsys.exit(int(os.environ.get("CONFIGURE_FAIL", "0")))\n')
        self.write('port/CMakeLists.txt', '# fixture\n')
        self.git('add', '.')
        self.git('commit', '-m', 'Verification fixtures')
        self.git('remote', 'add', 'origin', 'https://github.com/CallanCampos/Frigate.git')
        for version in ('b2_us', 'w2_us'):
            self.write('orig/baserom_' + version + '.nds', 'fixture')
            self.write('build/' + version + '.nds', 'fixture')
            self.write(version + '.sha1', '0' * 40 + '  build/' + version + '.nds\n')
        bins = Path(self.temp.name).parent / (self.root.name + '-bins')
        bins.mkdir()
        self.addCleanup(lambda: __import__('shutil').rmtree(bins))
        for name in ('ninja', 'cmake', 'ctest', 'sha1sum'):
            target = bins / name
            target.write_text('#!/bin/sh\nexit 0\n')
            target.chmod(0o755)
        target = bins / 'gh'
        target.write_text('''#!/bin/sh
case "$1 $2" in
  "pr view")
    if [ -f "$GH_COUNT" ] && [ "$GH_CHANGE" = 1 ]; then
      printf '%040d\n' 0
    else
      git rev-parse HEAD
    fi
    touch "$GH_COUNT"
    ;;
  "api --method") printf '%s\n' "$*" >> "$GH_LOG" ;;
esac
''')
        target.chmod(0o755)
        environment = dict(os.environ, PATH=str(bins) + os.pathsep + os.environ['PATH'],
                           PYTHON='python3', GH_COUNT=str(bins / 'count'), GH_LOG=str(bins / 'log'), GH_CHANGE='0')
        return bins, environment

    def verify(self, environment):
        return subprocess.run(['bash', 'port/tools/sync_upstream.sh', '--verify-pr', '123'],
                              cwd=self.root, env=environment, text=True, capture_output=True)

    def test_shell_build_failure_leaves_pending(self):
        bins, environment = self.fixture()
        environment['CONFIGURE_FAIL'] = '1'
        result = self.verify(environment)
        self.assertNotEqual(result.returncode, 0)
        log = (bins / 'log').read_text()
        self.assertIn('state=pending', log)
        self.assertNotIn('state=success', log)

    def test_shell_remote_head_change_refuses_success(self):
        bins, environment = self.fixture()
        environment['GH_CHANGE'] = '1'
        result = self.verify(environment)
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('PR head changed', result.stderr)
        self.assertNotIn('state=success', (bins / 'log').read_text())

    def test_shell_untracked_source_refused(self):
        bins, environment = self.fixture()
        self.write('port/hal/untracked.c', 'uncommitted input\n')
        result = self.verify(environment)
        self.assertNotEqual(result.returncode, 0)
        self.assertFalse((bins / 'log').exists())


if __name__ == '__main__':
    unittest.main()

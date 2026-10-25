"""Isolated Git fixtures for Frigate upstream PR sync; no network or ROMs.

Exercises preservation of native port changes, the draft proposal of a conflicting
upstream head and the upstream history/reference boundaries implemented by
port/tools/upstream_sync.py.
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

    def in_repo(self):
        real_run = subprocess.run
        def run(args, **kwargs):
            return real_run(args, cwd=self.root, **kwargs)
        return patch.object(sync.subprocess, 'run', side_effect=run)

    def prepare(self, incoming):
        """Runs prepare() with both publishers replaced; returns the merge's branch (None for a draft)."""
        real_git = self.git
        def fake_git(*args):
            if args[0] == 'fetch':
                real_git('fetch', '.', 'upstream')
                return ''
            return real_git(*args)
        with patch.object(sync, 'git', side_effect=fake_git), patch.object(sync, 'pending', return_value=[]), patch.object(sync, 'publish') as self.published, patch.object(sync, 'propose_conflict') as self.drafted, self.in_repo():
            sync.prepare()
            return self.published.call_args.args[0] if self.published.called else None


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

    def test_clean_merge_proposes_no_draft(self):
        self.assertIsNotNone(self.prepare(self.incoming()))
        self.drafted.assert_not_called()

    def test_conflict_proposes_a_draft_of_upstream_head_and_merges_nothing(self):
        incoming = self.incoming(conflict=True)
        before = self.git('rev-parse', 'main')
        self.assertIsNone(self.prepare(incoming))
        self.published.assert_not_called()
        branch, base, upstream, files = self.drafted.call_args.args
        self.assertTrue(branch.startswith(sync.PREFIX))
        self.assertEqual((base, upstream, files), (before, incoming, ['game.c']))
        # No topic branch, no merge in progress, no file changed
        self.assertEqual(self.git('branch', '--show-current'), 'main')
        self.assertEqual(self.git('rev-parse', 'HEAD'), before)
        self.assertEqual(self.git('status', '--porcelain'), '')
        self.assertFalse((self.root / '.git/MERGE_HEAD').exists())
        self.assertEqual(self.git('branch', '--list', sync.PREFIX + '*'), '')

    def test_reference_stops_before_a_draft(self):
        with self.assertRaisesRegex(RuntimeError, 'reference'):
            self.prepare(self.incoming('See another/project#42', conflict=True))
        self.drafted.assert_not_called()

    def test_workflow_changes_stop_before_a_draft(self):
        self.git('checkout', 'upstream')
        self.write('game.c', 'upstream\n')
        self.write('.github/workflows/changed.yml', 'name: incoming\n')
        self.git('add', '.')
        self.git('commit', '-m', 'Workflow update')
        incoming = self.git('rev-parse', 'HEAD')
        self.git('checkout', 'main')
        self.write('game.c', 'port game edit\n')
        self.git('add', '.')
        self.git('commit', '-m', 'Port game edit')
        with self.assertRaisesRegex(RuntimeError, 'workflow changes'):
            self.prepare(incoming)
        self.drafted.assert_not_called()

    def test_rewrite_stops_before_a_draft(self):
        incoming = self.incoming(conflict=True)
        before = self.git('rev-parse', 'HEAD')
        merge = self.git('commit-tree', 'HEAD^{tree}', '-p', before, '-p', incoming,
                         '-m', 'Merge upstream fuddlesworth/pokebw2@' + incoming[:9] + '\n\nUpstream-Commit: ' + incoming)
        self.git('reset', '--hard', merge)
        self.git('branch', '-f', 'upstream', self.base)
        self.git('checkout', 'upstream')
        self.write('game.c', 'rewritten upstream\n')
        self.git('commit', '-am', 'Rewritten history')
        rewritten = self.git('rev-parse', 'HEAD')
        self.git('checkout', 'main')
        with self.assertRaisesRegex(RuntimeError, 'rewritten'):
            self.prepare(rewritten)
        self.drafted.assert_not_called()

    def test_workflow_in_an_upstream_merge_stops_before_a_draft(self):
        # A merge commit that adds a workflow file itself, which git log lists without a diff unless asked for one
        self.git('checkout', 'upstream')
        self.write('game.c', 'upstream\n')
        self.git('commit', '-am', 'Upstream game edit')
        self.git('checkout', '-b', 'side', 'upstream~1')
        self.write('side.c', 'side\n')
        self.git('add', 'side.c')
        self.git('commit', '-m', 'Side')
        self.git('checkout', 'upstream')
        self.git('merge', '--no-ff', '--no-commit', 'side')
        self.write('.github/workflows/evil.yml', 'on: push\n')
        self.git('add', '.github/workflows/evil.yml')
        self.git('commit', '-m', 'Merge side')
        incoming = self.git('rev-parse', 'HEAD')
        self.git('checkout', 'main')
        self.write('game.c', 'port game edit\n')
        self.git('commit', '-am', 'Port game edit')
        with self.assertRaisesRegex(RuntimeError, 'workflow changes'):
            self.prepare(incoming)
        self.drafted.assert_not_called()

    def test_conflicting_files_names_unusual_paths_as_they_are(self):
        self.git('checkout', 'upstream')
        self.write('dir é/a file.c', 'upstream\n')
        self.git('add', '.')
        self.git('commit', '-m', 'Upstream file')
        incoming = self.git('rev-parse', 'HEAD')
        self.git('checkout', 'main')
        self.write('dir é/a file.c', 'port\n')
        self.git('add', '.')
        self.git('commit', '-m', 'Port file')
        with self.in_repo():
            self.assertEqual(sync.conflicting_files(self.git('rev-parse', 'HEAD'), incoming), ['dir é/a file.c'])

    def test_conflicting_files_failure_stops(self):
        with self.in_repo(), self.assertRaisesRegex(RuntimeError, 'merge-tree failed'):
            sync.conflicting_files(self.git('rev-parse', 'HEAD'), '0' * 40)

    def test_pending_asks_for_draft_state_and_keeps_canonical_topics(self):
        pulls = [
            {'number': 1, 'url': 'a', 'headRefName': sync.PREFIX + 'x', 'headRepositoryOwner': {'login': 'CallanCampos'}, 'isDraft': True},
            {'number': 2, 'url': 'b', 'headRefName': sync.PREFIX + 'y', 'headRepositoryOwner': {'login': 'someone'}, 'isDraft': False},
            {'number': 3, 'url': 'c', 'headRefName': 'feature', 'headRepositoryOwner': {'login': 'CallanCampos'}, 'isDraft': False},
        ]
        with patch.object(sync, 'gh', return_value=json.dumps(pulls)) as gh:
            self.assertEqual(sync.pending(), pulls[:1])
        fields = gh.call_args.args[gh.call_args.args.index('--json') + 1].split(',')
        self.assertIn('isDraft', fields)
        self.assertIn('number', fields)

    def remote(self):
        """A bare repository as origin, outside the work tree, holding nothing yet."""
        remote = tempfile.TemporaryDirectory()
        self.addCleanup(remote.cleanup)
        origin = Path(remote.name) / 'origin.git'
        subprocess.run(['git', 'init', '--bare', '-q', str(origin)], check=True)
        self.git('remote', 'add', 'origin', str(origin))
        return origin

    def fast_forwards(self, branch, upstream):
        with patch.object(sync, 'git', side_effect=self.git), self.in_repo():
            return sync.topic_fast_forwards(branch, upstream)

    def test_topic_check_passes_a_draft_upstream_moved_past(self):
        self.remote()
        branch = sync.PREFIX + 'draft'
        draft = self.incoming(conflict=True)
        self.git('push', '-q', 'origin', f'{draft}:refs/heads/{branch}')
        self.assertTrue(self.fast_forwards(sync.PREFIX + 'unpublished', draft))
        self.git('checkout', 'upstream')
        self.write('more.c', 'more\n')
        self.git('add', 'more.c')
        self.git('commit', '-m', 'More upstream')
        newer = self.git('rev-parse', 'HEAD')
        self.git('checkout', 'main')
        self.assertTrue(self.fast_forwards(branch, newer))

    def test_topic_check_stops_a_draft_upstream_rewrote(self):
        self.remote()
        branch = sync.PREFIX + 'draft'
        draft = self.incoming(conflict=True)
        self.git('push', '-q', 'origin', f'{draft}:refs/heads/{branch}')
        self.git('checkout', '-B', 'upstream', self.base)
        self.write('game.c', 'rewritten upstream\n')
        self.git('commit', '-am', 'Rewritten upstream')
        rewritten = self.git('rev-parse', 'HEAD')
        self.git('checkout', 'main')
        self.assertFalse(self.fast_forwards(branch, rewritten))

    def test_topic_check_passes_a_checked_out_topic(self):
        self.remote()
        branch = sync.PREFIX + 'topic'
        self.git('push', '-q', 'origin', f'HEAD:refs/heads/{branch}')
        self.assertTrue(self.fast_forwards(branch, self.base))

    def test_conflicting_files_changes_nothing(self):
        clean = self.incoming()
        head = self.git('rev-parse', 'HEAD')
        with self.in_repo():
            self.assertEqual(sync.conflicting_files(head, clean), [])
        self.write('game.c', 'port game edit\n')
        self.git('add', '.')
        self.git('commit', '-m', 'Port game edit')
        head = self.git('rev-parse', 'HEAD')
        with self.in_repo():
            self.assertEqual(sync.conflicting_files(head, clean), ['game.c'])
        self.assertEqual(self.git('rev-parse', 'HEAD'), head)
        self.assertEqual(self.git('status', '--porcelain'), '')

    def test_draft_pushes_upstream_head_without_force(self):
        branch, base, upstream = sync.PREFIX + 'fixture', 'a' * 40, 'b' * 40
        with patch.object(sync.subprocess, 'run', return_value=subprocess.CompletedProcess([], 0)) as commands, patch.object(sync, 'gh', return_value='created') as gh:
            sync.propose_conflict(branch, base, upstream, ['game.c', 'src/x.c'])
        commands.assert_called_once_with(['git', 'push', 'origin', upstream + ':refs/heads/' + branch])
        create = gh.call_args.args
        self.assertEqual(create[:2], ('pr', 'create'))
        self.assertIn('--draft', create)
        self.assertNotIn('--reviewer', create)
        self.assertEqual(create[create.index('--title') + 1], sync.CONFLICT_TITLE)
        body = create[create.index('--body') + 1]
        for text in ('`game.c`', '`src/x.c`', f'git switch --no-track -c {branch} origin/main', f'--branch {branch}', upstream[:12]):
            self.assertIn(text, body)
        # A draft's text names no issue or pull request, here or in another repository
        self.assertIsNone(sync.REFERENCE.search(body))
        self.assertNotIn('#', body.replace('```', ''))

    def test_draft_keeps_file_names_in_code_spans(self):
        body = sync.conflict_body(sync.PREFIX + 'fixture', 'a' * 40, 'b' * 40, ['docs/see owner/repo#12`.md'])
        self.assertIn('- `docs/see owner/repo#12' + r'\u0060' + '.md`\n', body)

    def test_draft_lists_a_bounded_number_of_files(self):
        files = [f'src/f{i}.c' for i in range(sync.LISTED_FILES + 7)]
        body = sync.conflict_body(sync.PREFIX + 'fixture', 'a' * 40, 'b' * 40, files)
        self.assertIn(f'`src/f{sync.LISTED_FILES - 1}.c`', body)
        self.assertNotIn(f'`src/f{sync.LISTED_FILES}.c`', body)
        self.assertIn('and 7 more', body)

    def test_failed_draft_push_names_the_conflicts(self):
        with patch.object(sync.subprocess, 'run', return_value=subprocess.CompletedProcess([], 1)), patch.object(sync, 'gh') as gh:
            with self.assertRaisesRegex(RuntimeError, 'game.c'):
                sync.propose_conflict(sync.PREFIX + 'fixture', 'a' * 40, 'b' * 40, ['game.c'])
        gh.assert_not_called()

    def test_resolved_merge_fast_forwards_the_draft_and_records_upstream(self):
        incoming = self.incoming(conflict=True)
        before = self.git('rev-parse', 'main')
        # What a maintainer's local sync makes: from main, the merge with its trailer, the conflict resolved
        self.git('checkout', '-b', sync.PREFIX + 'fixture')
        message = f'Merge upstream fuddlesworth/pokebw2@{incoming[:9]}\n\nUpstream-Commit: {incoming}'
        self.assertNotEqual(subprocess.run(['git', '-C', str(self.root), 'merge', '--no-ff', '-m', message, incoming],
                                           capture_output=True).returncode, 0)
        self.write('game.c', 'resolved\n')
        self.git('add', 'game.c')
        self.git('commit', '--no-edit', '--cleanup=strip')
        merge = self.git('rev-parse', 'HEAD')
        # The draft's branch holds incoming: pushing the merge over it is a fast-forward
        self.assertEqual(subprocess.run(['git', '-C', str(self.root), 'merge-base', '--is-ancestor', incoming, merge]).returncode, 0)
        self.assertEqual(self.git('rev-parse', 'HEAD^1'), before)
        with patch.object(sync, 'git', side_effect=self.git):
            self.assertEqual(sync.last_upstream(incoming), incoming)

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
        with patch.dict(os.environ, {'GITHUB_ACTIONS': 'true'}), patch.object(sync, 'git'), patch.object(sync, 'pending', return_value=[]), patch.object(sync, 'gh', return_value='created') as gh, patch.object(sync, 'request_review') as review, patch.object(sync, 'run', side_effect=AssertionError('unexpected API call')), patch.object(sync.subprocess, 'run'):
            sync.publish(sync.PREFIX + 'app-fixture')
            create = gh.call_args.args
            self.assertEqual(create[:2], ('pr', 'create'))
            self.assertNotIn('--reviewer', create)
            review.assert_called_once_with({'url': 'created'})
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
        for proposal in ({'url': 'pending'}, {'url': 'draft', 'isDraft': True, 'number': 7}):
            with self.subTest(proposal=proposal), patch.object(sync, 'pending', return_value=[proposal]), patch.object(sync, 'request_review') as review, patch.object(sync, 'git') as git, patch.object(sync.subprocess, 'run') as commands:
                sync.prepare()
                git.assert_not_called()
                commands.assert_not_called()
                review.assert_called_once_with(proposal)

    def publish_over(self, proposal):
        branch = sync.PREFIX + 'fixture'
        proposal = dict(proposal, headRefName=branch, headRepositoryOwner={'login': 'CallanCampos'})
        with patch.object(sync, 'git', return_value='a' * 40), patch.object(sync, 'pending', return_value=[proposal]), patch.object(sync, 'request_review'), patch.object(sync, 'gh', return_value='') as gh, patch.object(sync, 'run', return_value='CallanCampos'), patch.object(sync.subprocess, 'run') as commands:
            sync.publish(branch)
        commands.assert_called_once_with(['git', 'push', 'origin', 'HEAD:refs/heads/' + branch], check=True)
        return [call.args for call in gh.call_args_list]

    def test_resolved_draft_becomes_the_ordinary_proposal(self):
        calls = self.publish_over({'url': 'draft', 'isDraft': True, 'number': 7})
        self.assertEqual(calls[0][:5], ('pr', 'edit', '7', '--title', sync.TITLE))
        self.assertEqual(calls[0][6], sync.merge_body())
        self.assertEqual(calls[1], ('pr', 'ready', '7'))
        self.assertEqual(len(calls), 2)

    def test_ready_proposal_left_as_it_is(self):
        self.assertEqual(self.publish_over({'url': 'pending', 'isDraft': False, 'number': 7}), [])

    def test_draft_left_as_it_is_when_its_text_fails_after_the_push(self):
        branch = sync.PREFIX + 'fixture'
        draft = {'url': 'draft', 'isDraft': True, 'number': 7, 'headRefName': branch, 'headRepositoryOwner': {'login': 'CallanCampos'}}
        failure = subprocess.CalledProcessError(1, ['gh'])
        with patch.object(sync, 'git'), patch.object(sync, 'pending', return_value=[draft]), patch.object(sync, 'gh', side_effect=failure), patch.object(sync.subprocess, 'run') as commands, patch('sys.stderr') as stderr:
            sync.publish(branch)
        commands.assert_called_once()
        self.assertIn('gh pr ready 7', ''.join(call.args[0] for call in stderr.write.call_args_list))

    def test_another_pending_topic_refuses_publication(self):
        other = {'url': 'other', 'isDraft': True, 'number': 7, 'headRefName': sync.PREFIX + 'other', 'headRepositoryOwner': {'login': 'CallanCampos'}}
        with patch.object(sync, 'git'), patch.object(sync, 'pending', return_value=[other]), patch.object(sync.subprocess, 'run') as commands:
            with self.assertRaisesRegex(RuntimeError, 'already pending'):
                sync.publish(sync.PREFIX + 'mine')
        commands.assert_not_called()


class PublicationRecoveryTests(unittest.TestCase):
    def proposal(self):
        return {'number': 37, 'url': 'https://github.com/CallanCampos/Frigate/pull/37',
                'headRefName': sync.PREFIX + 'fixture', 'isDraft': False}

    def details(self, **changes):
        details = dict(author={'login': 'frigate-upstream-sync[bot]'}, isDraft=False,
                       reviewRequests=[], reviews=[])
        details.update(changes)
        return details

    def test_creation_response_failure_recovers_existing_pr_without_push(self):
        proposal = self.proposal()
        with patch.object(sync, 'gh', side_effect=subprocess.CalledProcessError(1, ['gh'])), \
                patch.object(sync, 'pending', return_value=[proposal]), patch.object(sync, 'git') as git:
            self.assertEqual(sync.create_proposal(proposal['headRefName'], 'title', 'body'), proposal)
        git.assert_not_called()

    def test_real_creation_failure_stays_failed(self):
        with patch.object(sync, 'gh', side_effect=subprocess.CalledProcessError(1, ['gh'])), \
                patch.object(sync, 'pending', return_value=[]):
            with self.assertRaises(subprocess.CalledProcessError):
                sync.create_proposal(sync.PREFIX + 'fixture', 'title', 'body')

    def test_review_failure_warns_and_next_pending_run_repairs_only_metadata(self):
        proposal = self.proposal()
        details = json.dumps(self.details())
        responses = [details, subprocess.CalledProcessError(1, ['gh']), details, '']
        with patch.object(sync, 'gh', side_effect=responses) as gh, \
                patch.object(sync, 'pending', return_value=[proposal]), \
                patch.object(sync, 'git') as git, patch.object(sync.subprocess, 'run') as commands, \
                patch('sys.stderr') as stderr:
            sync.prepare()
            self.assertIn('next scheduled run retries', ''.join(c.args[0] for c in stderr.write.call_args_list))
            sync.prepare()
        git.assert_not_called()
        commands.assert_not_called()
        self.assertEqual(gh.call_args_list[1].args, ('pr', 'edit', '37', '--add-reviewer', 'CallanCampos'))
        self.assertEqual(gh.call_args_list[3].args, gh.call_args_list[1].args)

    def test_existing_request_review_and_owner_author_do_not_request_again(self):
        for changes in ({'reviewRequests': [{'login': 'CallanCampos'}]},
                        {'reviews': [{'author': {'login': 'CallanCampos'}, 'state': 'APPROVED'}]},
                        {'reviews': [{'author': {'login': 'CallanCampos'}, 'state': 'CHANGES_REQUESTED'}]},
                        {'author': {'login': 'CallanCampos'}}, {'isDraft': True}):
            with self.subTest(changes=changes), patch.object(sync, 'gh', return_value=json.dumps(self.details(**changes))) as gh:
                sync.request_review(self.proposal())
                self.assertEqual(gh.call_count, 1)

    def test_draft_never_requests_review(self):
        with patch.object(sync, 'gh') as gh:
            sync.request_review(dict(self.proposal(), isDraft=True))
        gh.assert_not_called()

    def test_filename_controls_cannot_end_markdown_paragraph_or_span(self):
        name = 'source\n\nowner/repo#123`\r\t\x1b.c'
        rendered = sync.display_filename(name)
        self.assertEqual(rendered, r'source\n\nowner/repo#123\u0060\r\t\u001b.c')
        body = sync.conflict_body(sync.PREFIX + 'fixture', 'a' * 40, 'b' * 40, [name])
        listed = body.split('Conflicting files:\n\n', 1)[1].split('\n\nTo resolve', 1)[0]
        self.assertEqual(listed, '- `' + rendered + '`')


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

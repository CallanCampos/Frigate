#!/usr/bin/env python3
"""Prepare bounded upstream merge PRs without executing incoming game/build code.

The scheduled workflow uses this trusted main-branch helper; local sync_upstream.sh
publishes only after checking both ROMs and the native port tests. When upstream
conflicts with Frigate's main, the helper merges nothing: it proposes a draft PR
of upstream's head alone, which a maintainer's resolved merge later fast-forwards
(port/docs/SYNC.md).
"""
import argparse
import json
import re
import subprocess
import sys

REPO = 'CallanCampos/Frigate'
PREFIX = 'codex/upstream-sync-'
REFERENCE = re.compile(r'[\w.-]+/[\w.-]+#\d+|github\.com/[^/\s]+/[^/\s]+/(?:issues|pull|discussions)/\d+', re.I)
TITLE = 'Merge upstream decomp updates'
CONFLICT_TITLE = 'Upstream decomp updates: conflicts to resolve'
LISTED_FILES = 50


def run(*args):
    return subprocess.check_output(args, text=True).strip()


def git(*args):
    return run('git', *args)


def gh(*args):
    return run('gh', *args, '--repo', REPO)


def pending():
    pulls = json.loads(gh('pr', 'list', '--state', 'open', '--base', 'main', '--limit', '1000', '--json', 'number,url,headRefName,headRepositoryOwner,isDraft'))
    return [p for p in pulls if p['headRefName'].startswith(PREFIX) and p['headRepositoryOwner']['login'].lower() == 'callancampos']


def validate_branch(branch):
    if not branch.startswith(PREFIX) or branch == 'main':
        raise RuntimeError('sync destination must start with ' + PREFIX + '; main is never a push destination')
    git('check-ref-format', '--branch', branch)


def publish(branch):
    validate_branch(branch)
    existing = pending()
    if existing and any(p['headRefName'] != branch for p in existing):
        raise RuntimeError('A bounded upstream sync PR is already pending: ' + existing[0]['url'])
    # Ordinary push only: updates can never replace previously published history.
    subprocess.run(['git', 'push', 'origin', f'HEAD:refs/heads/{branch}'], check=True)
    if existing:
        proposal = existing[0]
        # The resolved merge of a draft conflict proposal: the PR becomes the ordinary proposal it now holds. The merge
        # is pushed by now, so a failure here only leaves the text for a maintainer, and the caller still records its
        # local verification.
        if proposal.get('isDraft'):
            number = str(proposal['number'])
            try:
                gh('pr', 'edit', number, '--title', TITLE, '--body', merge_body())
                gh('pr', 'ready', number)
            except subprocess.CalledProcessError:
                print(f'warning: the merge is pushed, but PR {number} is still the draft; retitle it "{TITLE}" and run: '
                      f'gh pr ready {number} --repo {REPO}', file=sys.stderr)
        request_review(dict(proposal, isDraft=False) if proposal.get('isDraft') else proposal)
        print(proposal['url'])
        return
    proposal = create_proposal(branch, TITLE, merge_body())
    request_review(proposal)
    print(proposal['url'])


def create_proposal(branch, title, body, draft=False):
    """Recover a PR that GitHub created even if the response was lost; never push again."""
    args = ['pr', 'create', '--base', 'main', '--head', branch, '--title', title, '--body', body]
    if draft:
        args.append('--draft')
    try:
        url = gh(*args)
    except subprocess.CalledProcessError:
        proposals = [p for p in pending() if p['headRefName'] == branch]
        if not proposals:
            raise
        print('warning: PR creation response failed; recovered ' + proposals[0]['url'], file=sys.stderr)
        return proposals[0]
    return {'url': url}


def request_review(proposal):
    """Retry only missing review metadata, without changing a proposal's commits or body."""
    if proposal.get('isDraft'):
        return
    target = str(proposal.get('number') or proposal['url'])
    try:
        details = json.loads(gh('pr', 'view', target, '--json', 'author,isDraft,reviewRequests,reviews'))
        if details['isDraft'] or details['author']['login'].lower() == 'callancampos':
            return  # GitHub cannot request a self-review on an owner-authored local proposal.
        if any(r.get('login', '').lower() == 'callancampos' for r in details['reviewRequests']):
            return
        if any(r['author']['login'].lower() == 'callancampos' and r['state'] in ('APPROVED', 'CHANGES_REQUESTED')
               for r in details['reviews']):
            return  # Never dismiss/re-request an owner's completed review on an unchanged proposal.
        gh('pr', 'edit', target, '--add-reviewer', 'CallanCampos')
    except (subprocess.CalledProcessError, ValueError, KeyError, TypeError):
        print(f'warning: PR {target} exists, but owner review could not be requested; the next scheduled run retries. '
              'Owner approval and exact-head local verification remain required before merge.', file=sys.stderr)


def merge_body():
    return ('Bounded upstream merge preserving Frigate changes. Review the full merge and its PR CI results.\n\n'
            'Before merging, a maintainer verifies this exact head in a separate worktree prepared with '
            '`port/tools/bootstrap_checkout.sh`, where N is this PR\'s number:\n\n'
            f'```sh\ngh pr checkout N --repo {REPO}\nport/tools/sync_upstream.sh --verify-pr N\n```\n\n'
            'It builds both ROMs and checks their SHA1s, builds and tests the host in both versions, and posts '
            '`Frigate local verification` on the tested head (port/docs/SYNC.md). '
            'The owner\'s explicit approval follows; the hourly sync leaves this PR unchanged while it is open.')


def conflicting_files(base, upstream):
    # A merge in Git's object store only (git 2.38 or later): no file, index or branch changes. Exit 1 means it
    # conflicts; -z gives the tree, then each conflicting path as it is, unquoted, each ended by a NUL.
    result = subprocess.run(['git', 'merge-tree', '--write-tree', '--name-only', '--no-messages', '-z', base, upstream],
                            capture_output=True, text=True)
    if result.returncode not in (0, 1):
        raise RuntimeError('git merge-tree failed: ' + result.stderr.strip())
    if result.returncode == 0:
        return []
    files = list(dict.fromkeys(name for name in result.stdout.split('\0')[1:] if name))
    # Git also exits 1, naming nothing, when it can't merge at all (an unknown commit).
    if not files:
        raise RuntimeError('git merge-tree failed: ' + (result.stderr.strip() or 'a conflict without a file named'))
    return files


def display_filename(name):
    return json.dumps(name, ensure_ascii=True)[1:-1].replace('`', r'\u0060')


def conflict_body(branch, base, upstream, files):
    # JSON escapes controls (including newlines); escape backticks too, so a filename cannot end the code span
    # or its paragraph and cause GitHub to link an issue in another repository.
    listed = ''.join('- `' + display_filename(name) + '`\n' for name in files[:LISTED_FILES])
    if len(files) > LISTED_FILES:
        listed += f'- and {len(files) - LISTED_FILES} more\n'
    return (f'Upstream `{upstream[:12]}` conflicts with Frigate\'s `main` at `{base[:12]}`, so nothing is merged yet. '
            'This draft holds upstream\'s head unchanged: no Frigate commit and no conflict markers. It can\'t be merged '
            'as it is, and the hourly sync leaves it unchanged while it is open.\n\n'
            f'Conflicting files:\n\n{listed}\n'
            'To resolve it, in a worktree prepared with `port/tools/bootstrap_checkout.sh`:\n\n'
            f'```sh\ngit fetch origin\ngit switch --no-track -c {branch} origin/main\n'
            f'port/tools/sync_upstream.sh --branch {branch}\n```\n\n'
            'The tool merges upstream\'s newest head and stops at the conflicts (exit 3). Resolve them as '
            'port/docs/SYNC.md says (upstream\'s structure, the port\'s adaptation from game-changes.md reapplied), '
            'commit with `git commit --no-edit --cleanup=strip` and run it again. After both ROMs and the host pass, '
            'it pushes the merge to this branch, a fast-forward of upstream\'s head, and marks this PR ready for review. '
            'Don\'t use GitHub\'s Update branch here: a merge of `main` into this branch would stop that fast-forward.')


def propose_conflict(branch, base, upstream, files):
    # Upstream's own commit, pushed as it is: the branch gets no merge and no commit of Frigate's, and the resolved
    # merge that a maintainer pushes later has it among its ancestors, so that push is a fast-forward
    # (topic_fast_forwards checks it before the merge).
    if subprocess.run(['git', 'push', 'origin', f'{upstream}:refs/heads/{branch}']).returncode:
        raise RuntimeError('upstream conflicts in ' + ', '.join(display_filename(name) for name in files[:LISTED_FILES]) +
                           ', and pushing its head for a draft proposal failed; resolve it locally (port/docs/SYNC.md)')
    # No review request on a draft: GitHub notifies the code owners when the resolved merge makes it ready (publish).
    proposal = create_proposal(branch, CONFLICT_TITLE, conflict_body(branch, base, upstream, files), draft=True)
    print(proposal['url'])


def topic_fast_forwards(branch, upstream):
    """False when origin has the topic at a commit that neither upstream's head nor HEAD contains.

    The local sync merges upstream's newest head onto HEAD and pushes that to origin's topic. A draft conflict
    proposal's topic holds an older upstream head, which the merge contains unless upstream rewrote it, and a topic
    being repaired is checked out, so HEAD contains it; otherwise the push couldn't be a fast-forward.
    """
    listed = git('ls-remote', '--heads', 'origin', 'refs/heads/' + branch)
    if not listed:
        return True
    tip = listed.split()[0]
    git('fetch', '--no-tags', 'origin', 'refs/heads/' + branch)
    return any(subprocess.run(['git', 'merge-base', '--is-ancestor', tip, ref]).returncode == 0 for ref in (upstream, 'HEAD'))


def last_upstream(upstream):
    # GitHub PR merge commits wrap sync merges on their second-parent history.
    # Traverse the whole DAG in topological order, then validate actual sync
    # provenance: the trailer must name that merge's second parent.
    for merge in git('log', '--topo-order', '--merges', '--format=%H', 'HEAD').splitlines():
        message = git('show', '-s', '--format=%B', merge)
        trailers = re.findall(r'^Upstream-Commit: ([0-9a-f]{40})$', message, re.M)
        parents = git('show', '-s', '--format=%P', merge).split()
        if len(trailers) == 1 and len(parents) == 2 and parents[1] == trailers[0]:
            expected = 'Merge upstream fuddlesworth/pokebw2@' + trailers[0][:9]
            if message.splitlines()[0] == expected:
                return trailers[0]
    return git('merge-base', 'HEAD', upstream)


def prepare():
    existing = pending()
    if existing:
        for proposal in existing:
            request_review(proposal)
        print('Pending sync left unchanged: ' + existing[0]['url'])
        return
    base = git('rev-parse', 'HEAD')
    git('fetch', '--no-tags', 'https://github.com/fuddlesworth/pokebw2.git', 'main')
    upstream = git('rev-parse', 'FETCH_HEAD')
    last = last_upstream(upstream)
    if subprocess.run(['git', 'merge-base', '--is-ancestor', last, upstream]).returncode:
        raise RuntimeError('Upstream history rewritten; review locally before proceeding')
    if not subprocess.run(['git', 'merge-base', '--is-ancestor', upstream, base]).returncode:
        print('Already up to date')
        return
    for commit in git('rev-list', upstream, '--not', base).splitlines():
        if REFERENCE.search(git('show', '-s', '--format=%B', commit)):
            raise RuntimeError('Cross-repository reference requires explicit local authorization: ' + commit[:9])
    # Check every incoming commit, a merge's own changes included, as well as the resulting diff (including add/revert
    # pairs), so that no workflow file can reach a draft, which is pushed without a merge.
    if (git('log', '--full-history', '--diff-merges=first-parent', '--format=', '--name-only', f'{base}..{upstream}', '--', '.github/workflows')
            or git('diff', '--name-only', last, upstream, '--', '.github/workflows')):
        raise RuntimeError('Upstream workflow changes require local sync with maintainer credentials')
    branch = PREFIX + upstream[:12] + '-' + base[:12]
    validate_branch(branch)
    # Never a merge with conflict markers or a side chosen here: a conflict becomes a draft of upstream's head.
    files = conflicting_files(base, upstream)
    if files:
        propose_conflict(branch, base, upstream, files)
        return
    git('checkout', '-b', branch)
    subprocess.run(['git', 'merge', '--no-ff', '--no-edit', '-m', f'Merge upstream fuddlesworth/pokebw2@{upstream[:9]}\n\nUpstream-Commit: {upstream}', upstream], check=True)
    if git('diff', '--name-only', base, 'HEAD', '--', '.github/workflows'):
        raise RuntimeError('Merged workflow changes require local sync')
    publish(branch)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--publish', metavar='BRANCH')
    parser.add_argument('--last-upstream', metavar='REF')
    parser.add_argument('--check-topic', nargs=2, metavar=('BRANCH', 'UPSTREAM'))
    args = parser.parse_args()
    if args.last_upstream:
        print(last_upstream(args.last_upstream))
    elif args.check_topic:
        if not topic_fast_forwards(*args.check_topic):
            sys.exit(4)
    elif args.publish:
        publish(args.publish)
    else:
        prepare()


if __name__ == '__main__':
    try:
        main()
    except (RuntimeError, subprocess.CalledProcessError) as error:
        print('upstream sync stopped: ' + str(error), file=sys.stderr)
        sys.exit(1)

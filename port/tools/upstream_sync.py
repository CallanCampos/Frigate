#!/usr/bin/env python3
"""Prepare bounded upstream merge PRs without executing incoming game/build code.

The scheduled workflow uses this trusted main-branch helper; local sync_upstream.sh
publishes only after checking both ROMs and the native port tests.
"""
import argparse
import json
import os
import re
import subprocess
import sys

REPO = 'CallanCampos/Frigate'
PREFIX = 'codex/upstream-sync-'
REFERENCE = re.compile(r'[\w.-]+/[\w.-]+#\d+|github\.com/[^/\s]+/[^/\s]+/(?:issues|pull|discussions)/\d+', re.I)


def run(*args):
    return subprocess.check_output(args, text=True).strip()


def git(*args):
    return run('git', *args)


def gh(*args):
    return run('gh', *args, '--repo', REPO)


def pending():
    pulls = json.loads(gh('pr', 'list', '--state', 'open', '--base', 'main', '--limit', '1000', '--json', 'url,headRefName,headRepositoryOwner'))
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
        print(existing[0]['url'])
        return
    reviewer = ['--reviewer', 'CallanCampos']
    if os.environ.get('GITHUB_ACTIONS') != 'true' and run('gh', 'api', 'user', '--jq', '.login').lower() == 'callancampos':
        reviewer = []
    print(gh('pr', 'create', '--base', 'main', '--head', branch, *reviewer, '--title', 'Merge upstream decomp updates', '--body',
             'Bounded upstream merge preserving Frigate changes. Review the full merge and its PR CI results. '
             'Before merging, a maintainer must run the local sync on this exact head to verify both ROM SHA1s and the host build/tests. '
             'The hourly sync leaves this PR unchanged while it is open.'))


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
    # Check every incoming commit as well as the resulting diff (including add/revert pairs).
    if git('log', '--full-history', '--format=', '--name-only', f'{base}..{upstream}', '--', '.github/workflows'):
        raise RuntimeError('Upstream workflow changes require local sync with maintainer credentials')
    branch = PREFIX + upstream[:12] + '-' + base[:12]
    validate_branch(branch)
    git('checkout', '-b', branch)
    subprocess.run(['git', 'merge', '--no-ff', '--no-edit', '-m', f'Merge upstream fuddlesworth/pokebw2@{upstream[:9]}\n\nUpstream-Commit: {upstream}', upstream], check=True)
    if git('diff', '--name-only', base, 'HEAD', '--', '.github/workflows'):
        raise RuntimeError('Merged workflow changes require local sync')
    publish(branch)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--publish', metavar='BRANCH')
    parser.add_argument('--last-upstream', metavar='REF')
    args = parser.parse_args()
    if args.last_upstream:
        print(last_upstream(args.last_upstream))
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

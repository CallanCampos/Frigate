#!/usr/bin/env bash
# Merges the decomp's main (fuddlesworth/pokebw2) into the port's main, verifies the result with both ROMs and
# publishes it. Port sessions run this first in every iteration (port/docs/PLAN.md, section 8, step 1). It is also the
# fallback when the hourly port-sync Action can't finish a sync: see port/docs/SYNC.md.
#
# Usage: port/tools/sync_upstream.sh [options]
#
#   --branch NAME        the port branch to sync and publish, default main. The checkout can be on NAME or, in a
#                        worktree, on a branch of its own: HEAD is brought up to origin/NAME and pushed to NAME.
#   --no-build           merge only: no ROM build, no host build, and nothing pushed (an unbuilt merge is never
#                        published)
#   --no-push            leave the result unpublished
#   --metrics            also run port/tools/progress/run_all.sh into build/port-metrics
#   --abort-on-conflict  abort a conflicting merge instead of leaving it in progress
#   --allow-rewrite      merge although upstream rewrote its history (decide first, see SYNC.md)
#   --allow-references   merge and push although commit messages name issues, pull requests or discussions in other
#                        repositories (the user decides first, see SYNC.md)
#   -h, --help           show this text
#
# What it does:
#   1. Goes to the top of the clone. Requires origin to be CallanCampos/Frigate (https://github.com/... or
#      git@github.com:..., with or without .git; not the decomp or its fork CallanCampos/pokebw2). A clone made before
#      the repository's rename on 2026-10-18 still has its old name in origin's URL, which is refused too, with the
#      git remote set-url command that fixes it. Also requires upstream to be fuddlesworth/pokebw2 (added when
#      missing), no merge in progress and no changes to tracked files. It never stashes. It sets upstream's push URL to
#      DISABLED-port-never-pushes-upstream when it is anything else, as bootstrap_checkout.sh does, so that a push
#      there fails while fetching still works.
#   2. Fetches both and brings HEAD up to origin/NAME: a fast-forward, or a merge when HEAD has commits of its own,
#      which are then published too.
#   3. Merges upstream/main with the message "Merge upstream fuddlesworth/pokebw2@<short>" and an
#      "Upstream-Commit: <sha>" trailer, as the Action does. A conflict stops the script with the merge in progress:
#      resolve the files, `git add` them, `git commit --no-edit --cleanup=strip`, and run the script again. A rewrite
#      of upstream's history (the last merged upstream commit no longer on upstream main) stops it before any merge.
#      So does a commit that the push would add whose message names an issue, pull request or discussion in another
#      repository (OWNER/REPO#N or its github.com URL), whatever the port repository's visibility: GitHub could show a
#      reference from this repository on that item's timeline, without its details while the repository is private.
#      Only the commits' short SHAs are printed.
#      Messages with a closing keyword and a bare issue number ("fixes #3") only get a warning: on main they close
#      the port's own issue with that number.
#   4. Runs `python3 configure.py b2_us w2_us` and `ninja` (once more when ninja exits 139, wibo's intermittent
#      crash). ninja checks every module and both SHA1s; the script then checks both ROM files against b2_us.sha1
#      and w2_us.sha1 itself, so that a build with nothing to do also proves them.
#   5. When port/CMakeLists.txt exists: configures, builds and tests the host build in build/port.
#   6. Pushes HEAD to origin/NAME, only after steps 4 and 5 passed. When the push is refused because NAME moved,
#      fetches, merges again, rebuilds only what the new commits change, and retries, three attempts in all. It never
#      force-pushes.
#   7. Prints a summary block for port/docs/STATUS.md, which names any upstream workflow files the push changes.
#
# Exit status: 0 done, 1 failed, 2 bad usage or an origin that isn't the port repository, 3 merge conflict, 4 upstream
# history rewritten, 5 commit messages name issues in other repositories.
set -euo pipefail

PORT_REPO=CallanCampos/Frigate
# The port repository's name until the user renamed it on 2026-10-18. GitHub redirects it, but an origin that still
# names it is refused with the command that updates the URL, so that every clone names the repository as it is now.
PORT_REPO_BEFORE_RENAME=CallanCampos/pokebw2-native
UPSTREAM_REPO=fuddlesworth/pokebw2
UPSTREAM_URL=https://github.com/fuddlesworth/pokebw2.git
# The port never pushes upstream (ADR 0002): the upstream remote's push URL is this name, which no push can reach. The
# same value as in bootstrap_checkout.sh.
UPSTREAM_PUSH_DISABLED=DISABLED-port-never-pushes-upstream
UPSTREAM_BRANCH=main
PYTHON=${PYTHON:-python3}

# A commit message that names an issue, pull request or discussion in another repository, in one of the forms GitHub
# links: OWNER/REPO#N, or its github.com URL. The same patterns as in .github/workflows/port-sync.yml.
REFERENCE_SHORT='[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+#[0-9]+'
REFERENCE_URL='github\.com/[^/[:space:]]+/[^/[:space:]]+/(issues|pull|discussions)/[0-9]+'
# A closing keyword with an issue of the same repository ("fixes #3", "Closes: GH-3")
CLOSING='(^|[^[:alnum:]_])(close[sd]?|fix(e[sd])?|resolve[sd]?):?[[:space:]]+(#|GH-)[0-9]+'

branch=main
build=1
push=1
metrics=0
abort_on_conflict=0
allow_rewrite=0
allow_references=0

usage() {
    awk 'NR > 1 && /^#/ { sub(/^# ?/, ""); print; next } NR > 1 { exit }' "$0"
}

say() {
    printf '==> %s\n' "$*"
}

die() {
    local code=$1
    shift
    printf 'sync_upstream: %s\n' "$*" >&2
    exit "$code"
}

while [ $# -gt 0 ]; do
    case "$1" in
        --branch)
            [ $# -ge 2 ] || die 2 "--branch needs a branch name"
            branch=$2
            shift 2
            ;;
        --branch=*)
            branch=${1#--branch=}
            shift
            ;;
        --no-build) build=0; push=0; shift ;;
        --no-push) push=0; shift ;;
        --metrics) metrics=1; shift ;;
        --abort-on-conflict) abort_on_conflict=1; shift ;;
        --allow-rewrite) allow_rewrite=1; shift ;;
        --allow-references) allow_references=1; shift ;;
        -h | --help) usage; exit 0 ;;
        *) usage >&2; die 2 "unknown option: $1" ;;
    esac
done
[ -n "$branch" ] || die 2 "--branch needs a branch name"

root=$(git rev-parse --show-toplevel 2>/dev/null) || die 1 "run this inside the port clone"
cd "$root"

origin_ref=refs/remotes/origin/$branch
upstream_ref=refs/remotes/upstream/$UPSTREAM_BRANCH

is_upstream_url() {
    case "$1" in
        *[/:]fuddlesworth/pokebw2 | *[/:]fuddlesworth/pokebw2.git | *[/:]fuddlesworth/pokebw2/) return 0 ;;
    esac
    return 1
}

lower() {
    printf '%s' "$1" | tr '[:upper:]' '[:lower:]'
}

# Whether the URL $1 is the GitHub repository $2 (OWNER/REPO), over https or ssh, with or without .git. GitHub names
# are case-insensitive.
is_github_url() {
    local url repo
    url=$(lower "$1")
    repo=$(lower "$2")
    case "$url" in
        "https://github.com/$repo" | "https://github.com/$repo.git" | "https://github.com/$repo/" | \
            "git@github.com:$repo" | "git@github.com:$repo.git" | \
            "ssh://git@github.com/$repo" | "ssh://git@github.com/$repo.git")
            return 0
            ;;
    esac
    return 1
}

is_port_url() {
    is_github_url "$1" "$PORT_REPO"
}

# Refuses an origin URL that isn't the port repository's. $1 is the URL, $2 is --push for a push URL and empty for a
# fetch URL, and $3 is how many URLs of that kind origin has: with more than one, the fix names the URL it replaces.
check_origin_url() {
    local url=$1 push=$2 count=$3 new fix
    is_port_url "$url" && return 0
    if is_github_url "$url" "$PORT_REPO_BEFORE_RENAME"; then
        case "$(lower "$url")" in
            git@*) new=git@github.com:$PORT_REPO.git ;;
            ssh://*) new=ssh://git@github.com/$PORT_REPO.git ;;
            *) new=https://github.com/$PORT_REPO.git ;;
        esac
        fix="git remote set-url ${push:+$push }origin $new"
        if [ "$count" -gt 1 ]; then
            fix="$fix $url"
        fi
        die 2 "origin is $url, which names the port repository as it was before the user renamed it $PORT_REPO on" \
            "2026-10-18. GitHub redirects the old name, but origin must name the repository as it is now. Update" \
            "the URL, then run this again:"$'\n'"    $fix"
    fi
    case "$(lower "$url")" in
        *[/:]fuddlesworth/pokebw2 | *[/:]fuddlesworth/pokebw2.git | *[/:]fuddlesworth/pokebw2/ | \
            *[/:]callancampos/pokebw2 | *[/:]callancampos/pokebw2.git | *[/:]callancampos/pokebw2/)
            die 2 "origin is $url, the decomp or its fork, so this looks like the decomp checkout. Run this in the" \
                "port clone, whose origin is $PORT_REPO."
            ;;
    esac
    die 2 "origin is $url; it must be $PORT_REPO (https://github.com/$PORT_REPO.git or" \
        "git@github.com:$PORT_REPO.git)"
}

# --- 1. Checks -------------------------------------------------------------------------------------------------------

# Every URL configured for origin, for fetching and pushing, must be the port repository: the reference check below
# asks GitHub about that repository, and a push anywhere else (the decomp, or the fork CallanCampos/pokebw2, which is in
# upstream's fork network) would publish the port there. The URLs are checked as configured; url.*.insteadOf
# rewrites in the git configuration are not looked at.
git config --get remote.origin.url >/dev/null 2>&1 ||
    die 2 "there is no origin remote; it must be $PORT_REPO (https://github.com/$PORT_REPO.git)"
for origin_key in url pushurl; do
    origin_urls=$(git config --get-all "remote.origin.$origin_key" || true)
    origin_count=$(printf '%s' "$origin_urls" | grep -c '' || true)
    origin_push=""
    if [ "$origin_key" = pushurl ]; then
        origin_push=--push
    fi
    while IFS= read -r origin_url; do
        [ -n "$origin_url" ] || continue
        check_origin_url "$origin_url" "$origin_push" "$origin_count"
    done <<EOF
$origin_urls
EOF
done
if upstream_urls=$(git remote get-url --all upstream 2>/dev/null); then
    while IFS= read -r upstream_url; do
        is_upstream_url "$upstream_url" || die 1 "the upstream remote is $upstream_url, not $UPSTREAM_REPO"
    done <<EOF
$upstream_urls
EOF
else
    say "Adding the upstream remote, $UPSTREAM_URL"
    git remote add upstream "$UPSTREAM_URL"
fi
# Also when the remote was added by hand or before bootstrap_checkout.sh ran. A push URL replaces the fetch URL for
# pushes only, so fetching is unchanged.
if [ "$(git config --get-all remote.upstream.pushurl || true)" != "$UPSTREAM_PUSH_DISABLED" ]; then
    say "Setting the upstream remote's push URL to $UPSTREAM_PUSH_DISABLED: the port never pushes there"
    git config --replace-all remote.upstream.pushurl "$UPSTREAM_PUSH_DISABLED"
fi
if git rev-parse -q --verify MERGE_HEAD >/dev/null; then
    die 1 "a merge is in progress. Resolve it and run 'git commit --no-edit --cleanup=strip' (or" \
        "'git merge --abort'), then run this again."
fi
if ! git diff --quiet || ! git diff --cached --quiet; then
    die 1 "tracked files have changes. Commit them or set them aside first (not with a bare git stash: the stash" \
        "is shared with other sessions)."
fi
current=$(git symbolic-ref --quiet --short HEAD) || die 1 "HEAD is detached; check out a branch first"

# Results for the summary, printed on exit
sync_date=$(date '+%Y-%m-%d')
upstream_before=""
merged_count=0
rom_result="not run"
host_result="not run"
tests_result="not run"
push_result="not pushed"
metrics_result="not measured (run with --metrics)"
if [ "$build" = 0 ]; then
    rom_result="not run (--no-build)"
    host_result="not run (--no-build)"
    tests_result="not run (--no-build)"
fi
if [ "$build" = 0 ]; then
    push_result="not pushed (--no-build)"
elif [ "$push" = 0 ]; then
    push_result="not pushed (--no-push)"
fi
workflow_changes=""
summary_ready=0

# Prints the fields of a port/docs/STATUS.md entry that a sync knows; the session fills in the others
summary() {
    local status=$?
    [ "$summary_ready" = 1 ] || return 0
    local outcome="done" upstream_now pr=""
    case "$status" in
        0) ;;
        3) outcome="stopped at a merge conflict" ;;
        4) outcome="stopped: upstream rewrote its history" ;;
        5) outcome="stopped: commit messages name issues in other repositories" ;;
        *) outcome="FAILED (exit status $status)" ;;
    esac
    upstream_now=$(git rev-parse --short=9 "$upstream_ref" 2>/dev/null || echo unknown)
    pr=$(git log -1 --format=%s "$upstream_ref" 2>/dev/null | sed -n 's/^Merge pull request #\([0-9]*\) .*/\1/p')
    printf '\n---- for port/docs/STATUS.md; add Tiers, Trap stubs, 64-bit warnings, Work done, Next, Blockers ----\n'
    printf '## %s: upstream sync\n\n' "$sync_date"
    printf -- '- **Upstream:** `%s`' "$upstream_now"
    if [ -n "$pr" ]; then
        printf ' (merge of PR #%s)' "$pr"
    fi
    if [ "$merged_count" -gt 0 ]; then
        printf '; %s commits merged since `%s`\n' "$merged_count" "$(git rev-parse --short=9 "$upstream_before")"
    else
        printf '; nothing new merged by this run\n'
    fi
    printf -- '- **Port:** `%s` on %s, %s\n' "$(git rev-parse --short=9 HEAD)" "$branch" "$push_result"
    printf -- '- **ROM:** %s\n' "$rom_result"
    printf -- '- **Host build:** %s\n' "$host_result"
    printf -- '- **Tests:** T0 as above; ctest: %s\n' "$tests_result"
    printf -- '- **Tiers:** %s\n' "$metrics_result"
    if [ -n "$workflow_changes" ]; then
        printf -- '- **Upstream workflows changed:** %s. Once pushed, check `gh workflow list --all -R %s`:' \
            "$workflow_changes" "$PORT_REPO"
        printf ' `build.yml` must still be disabled; ask the user before disabling any other workflow\n'
    fi
    printf -- '----\nsync_upstream: %s\n' "$outcome"
}
trap summary EXIT

# --- 2 and 3. Merges -------------------------------------------------------------------------------------------------

fetch_all() {
    git fetch --no-tags origin "+refs/heads/$branch:$origin_ref" || die 1 "could not fetch $branch from origin"
    git fetch --no-tags upstream "+refs/heads/$UPSTREAM_BRANCH:$upstream_ref" ||
        die 1 "could not fetch $UPSTREAM_BRANCH from upstream"
}

stop_on_conflict() {
    local what=$1
    local files
    files=$(git diff --name-only --diff-filter=U)
    if [ -z "$files" ]; then
        if git rev-parse -q --verify MERGE_HEAD >/dev/null; then
            git merge --abort || true
        fi
        die 1 "merging $what failed without conflicting files; git's message is above"
    fi
    printf '\nsync_upstream: merging %s conflicts in:\n' "$what" >&2
    printf '%s\n' "$files" | sed 's/^/  /' >&2
    if [ "$abort_on_conflict" = 1 ]; then
        git merge --abort
        printf 'The merge was aborted (--abort-on-conflict).\n' >&2
    else
        printf 'The merge is left in progress. Resolve those files, then run:\n' >&2
        printf '  git add FILE...\n  git commit --no-edit --cleanup=strip\n  %s\n' "port/tools/sync_upstream.sh" >&2
        printf 'or give up with: git merge --abort\n' >&2
    fi
    exit 3
}

# Brings HEAD up to origin/NAME: nothing to do when HEAD already has it, a fast-forward when HEAD is behind, and the
# project's usual merge when HEAD has commits of its own
update_from_origin() {
    if git merge-base --is-ancestor "$origin_ref" HEAD; then
        return 0
    fi
    if git merge-base --is-ancestor HEAD "$origin_ref"; then
        git merge --ff-only --quiet "$origin_ref"
        return 0
    fi
    say "HEAD has commits that origin/$branch doesn't; merging origin/$branch"
    git merge --no-edit "$origin_ref" || stop_on_conflict "origin/$branch"
}

# The upstream commit that the newest sync merge recorded in its Upstream-Commit trailer, or the merge base when no
# sync merge has one yet
last_upstream() {
    local message sha
    message=$(git log --merges -E --grep='^Upstream-Commit: [0-9a-f]{40}$' -n 1 --format=%B HEAD)
    sha=$(printf '%s\n' "$message" | sed -n 's/^Upstream-Commit: \([0-9a-f]\{40\}\)$/\1/p' | tail -n 1)
    if [ -z "$sha" ]; then
        sha=$(git merge-base HEAD "$upstream_ref")
    fi
    printf '%s\n' "$sha"
}

own_merge=""
own_merge_parent=""

# Stops before a merge whose push would add commits that name an issue, pull request or discussion in another
# repository: GitHub records such a reference on the target's timeline, naming the repository the commit was pushed to
# (port/docs/SYNC.md, ADR 0002). People who can't read a private repository still see the reference, without its
# details, so the check runs whatever the port repository's visibility, and only the user's --allow-references lets
# such commits through. The range is every commit that the push would add to origin/NAME: the new upstream commits and
# HEAD's own. Only short SHAs are printed, never the text that matched.
check_references() {
    local upstream=$1 found count closing
    found=$(git rev-list -i -E --reverse --abbrev-commit --abbrev=9 --grep="$REFERENCE_SHORT" \
        --grep="$REFERENCE_URL" "$upstream" HEAD --not "$origin_ref" | paste -sd ' ' -)
    closing=$(git rev-list -i -E --reverse --abbrev-commit --abbrev=9 --grep="$CLOSING" \
        "$upstream" HEAD --not "$origin_ref" | paste -sd ' ' -)
    if [ -n "$closing" ]; then
        say "Warning: these commits name an issue of this repository after a closing keyword (\"fixes #3\"). Once" \
            "they are on main, GitHub closes the port's own issue with that number, if it is open; reopen it then:" \
            "$closing"
    fi
    [ -n "$found" ] || return 0
    count=$(printf '%s\n' $found | wc -l | tr -d ' ')
    if [ "$allow_references" = 1 ]; then
        say "$count commits that the push would add name issues, pull requests or discussions in other repositories;" \
            "merging anyway (--allow-references): $found"
        return 0
    fi
    die 5 "$count commits that the push would add name issues, pull requests or discussions in other repositories:" \
        "$found. Pushed to $PORT_REPO, they could put a reference from it on those items' timelines, with its" \
        "details while the repository is public and without them while it is private. Stopped before merging." \
        "Stop and ask the user, who alone decides whether to run this again with --allow-references; see 'Commit" \
        "messages that name other repositories' in port/docs/SYNC.md."
}

merge_upstream() {
    local upstream last count short message_file
    upstream=$(git rev-parse "$upstream_ref")
    # Checked before "already merged", which also holds when upstream went back to an older commit
    last=$(last_upstream)
    if ! git merge-base --is-ancestor "$last" "$upstream"; then
        if [ "$allow_rewrite" = 1 ]; then
            say "Upstream no longer contains $last, the last upstream commit merged here; merging anyway" \
                "(--allow-rewrite)"
        else
            die 4 "upstream $UPSTREAM_BRANCH no longer contains $last, the last upstream commit merged here, so its" \
                "history was rewritten. Nothing was merged. Read 'Upstream rewrote its history' in" \
                "port/docs/SYNC.md, then run again with --allow-rewrite if a merge is right."
        fi
    fi
    # Also when upstream is already merged here but not yet pushed
    check_references "$upstream"
    if git merge-base --is-ancestor "$upstream" HEAD; then
        say "Upstream $UPSTREAM_REPO@$(git rev-parse --short=9 "$upstream") is already merged"
        return 0
    fi
    [ -n "$upstream_before" ] || upstream_before=$last
    count=$(git rev-list --count "HEAD..$upstream")
    short=$(git rev-parse --short=9 "$upstream")
    say "Merging $count upstream commits, $(git rev-parse --short=9 "$last")..$short"
    message_file=$(git rev-parse --git-path PORT_SYNC_MSG)
    printf 'Merge upstream %s@%s\n\nUpstream-Commit: %s\n' "$UPSTREAM_REPO" "$short" "$upstream" > "$message_file"
    own_merge_parent=$(git rev-parse HEAD)
    if ! git merge --no-ff --no-edit -F "$message_file" "$upstream"; then
        rm -f "$message_file"
        stop_on_conflict "$UPSTREAM_REPO@$short"
    fi
    rm -f "$message_file"
    own_merge=$(git rev-parse HEAD)
    merged_count=$((merged_count + count))
}

say "Fetching origin/$branch and upstream/$UPSTREAM_BRANCH"
fetch_all
summary_ready=1
if [ "$current" != "$branch" ]; then
    say "On branch $current: syncing it with origin/$branch and publishing it there"
fi
update_from_origin
merge_upstream
local_commits=$(git rev-list --count "$origin_ref..HEAD")
if [ "$local_commits" -gt 0 ]; then
    say "$local_commits commits are not on origin/$branch yet:"
    git log --oneline --first-parent -n 20 "$origin_ref..HEAD"
fi

# --- 4 and 5. Builds -------------------------------------------------------------------------------------------------

# Homebrew's LLVM is keg-only, and configure.py needs its clang and llvm-objcopy
for llvm_bin in /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin; do
    if [ -d "$llvm_bin" ]; then
        PATH=$llvm_bin:$PATH
        export PATH
        break
    fi
done

# The ROM build reads nothing in port/, so trees that differ only there build the same ROMs
rom_fingerprint() {
    git ls-tree HEAD | awk -F '\t' '$2 != "port"' | git hash-object --stdin
}

sha1_check() {
    if command -v sha1sum >/dev/null 2>&1; then
        sha1sum -c "$1"
    else
        shasum -a 1 -c "$1"
    fi
}

# configure.py treats a tools/dsd without tools/dsd.rev as hand-built and never replaces it, so a linked dsd stays at
# whatever version it is when upstream pins another. bootstrap_checkout.sh leaves tools/dsd to configure.py.
warn_linked_dsd() {
    if [ -L tools/dsd ] && [ ! -e tools/dsd.rev ]; then
        say "Warning: tools/dsd links to $(readlink tools/dsd) and has no tools/dsd.rev, so configure.py keeps it" \
            "whatever its version. Remove the link to let configure.py download the pinned dsd."
    fi
}

rom_built=""
rom_build() {
    local version rom status log warnings
    for version in b2_us w2_us; do
        [ -e "orig/baserom_$version.nds" ] ||
            die 1 "orig/baserom_$version.nds is missing; run port/tools/bootstrap_checkout.sh"
    done
    command -v ninja >/dev/null 2>&1 || die 1 "ninja is not installed"
    warn_linked_dsd
    rom_result="FAILED"
    say "Configuring: $PYTHON configure.py b2_us w2_us"
    "$PYTHON" configure.py b2_us w2_us || die 1 "configure.py failed"
    mkdir -p build
    log=build/port-sync-ninja.log
    say "Building and checking both ROMs with ninja (a full build takes about 5 minutes); log in $log"
    status=0
    ninja > "$log" 2>&1 || status=$?
    if [ "$status" -eq 139 ]; then
        say "ninja exited 139, wibo's intermittent crash; running it once more"
        status=0
        ninja >> "$log" 2>&1 || status=$?
    fi
    if [ "$status" -ne 0 ]; then
        tail -n 40 "$log" >&2
        rom_result="FAILED: ninja exited $status, see $log"
        die 1 "ninja failed with exit status $status; the end of $log is above"
    fi
    tail -n 3 "$log"
    for version in b2_us w2_us; do
        [ -f "$version.sha1" ] || die 1 "$version.sha1 is missing; configure.py writes it"
        rom=$(awk '{ print $2; exit }' "$version.sha1")
        [ -f "$rom" ] || die 1 "$rom was not built"
        if ! sha1_check "$version.sha1"; then
            rom_result="FAILED: $rom does not match $version.sha1"
            die 1 "$rom does not match $version.sha1"
        fi
    done
    warnings=$(grep -c 'warning:' "$log" || true)
    rom_result="both SHA1s match (b2_us $(cut -c1-9 b2_us.sha1), w2_us $(cut -c1-9 w2_us.sha1))"
    if [ "$warnings" -gt 0 ]; then
        rom_result="$rom_result; $warnings compiler warnings among the files this build compiled"
    fi
    if ! git diff --quiet; then
        say "Warning: the build changed tracked files: $(git diff --name-only | tr '\n' ' ')"
    fi
    rom_built=$(rom_fingerprint)
}

host_built=""
host_build() {
    if [ ! -f port/CMakeLists.txt ]; then
        host_result="not yet (no port/CMakeLists.txt)"
        tests_result="not yet"
        return 0
    fi
    command -v cmake >/dev/null 2>&1 || die 1 "port/CMakeLists.txt exists but cmake is not installed"
    host_result="FAILED"
    say "Host build in build/port"
    # A new build directory uses Ninja, which the ROM build needs anyway, so that the asm oracle can read its flags; an
    # existing one keeps its generator, which CMake won't change
    if [ -f build/port/CMakeCache.txt ]; then
        cmake -S port -B build/port || die 1 "cmake could not configure port/"
    else
        cmake -S port -B build/port -G Ninja || die 1 "cmake could not configure port/"
    fi
    cmake --build build/port --parallel || die 1 "the host build failed"
    host_result="built (build/port)"
    tests_result="FAILED"
    ctest --test-dir build/port --output-on-failure || die 1 "host tests failed"
    tests_result="passed"
    host_built=$(git rev-parse 'HEAD^{tree}')
}

if [ "$build" = 1 ]; then
    rom_build
    host_build
fi

# --- 6. Push ---------------------------------------------------------------------------------------------------------

# The upstream workflow files that the push would change. A new one is enabled in the port repository once pushed, and
# one that needs upstream's secrets then fails there on every push.
note_workflow_changes() {
    workflow_changes=$(git diff --name-only "$origin_ref" HEAD -- .github/workflows \
        ':(exclude).github/workflows/port-*' | paste -sd ' ' -)
}
note_workflow_changes

# After a refused push: brings in what origin/NAME and upstream gained since. When the only unpublished commit is the
# sync merge made above, it is dropped and made again on the new origin/NAME, which keeps one merge per sync.
resync() {
    if [ -n "$own_merge" ] && [ "$(git rev-parse HEAD)" = "$own_merge" ] &&
        git merge-base --is-ancestor "$own_merge_parent" "$origin_ref"; then
        git reset --quiet --keep "$origin_ref"
        own_merge=""
        merged_count=0
    else
        update_from_origin
    fi
    merge_upstream
    if [ "$build" = 1 ]; then
        if [ "$(rom_fingerprint)" != "$rom_built" ]; then
            rom_build
        fi
        if [ -f port/CMakeLists.txt ] && [ "$(git rev-parse 'HEAD^{tree}')" != "$host_built" ]; then
            host_build
        fi
    fi
}

if [ "$push" = 1 ]; then
    if [ "$(git rev-parse HEAD)" = "$(git rev-parse "$origin_ref")" ]; then
        push_result="nothing new to push"
    else
        attempt=1
        while :; do
            refused_at=$(git rev-parse "$origin_ref")
            if git push origin "HEAD:$branch"; then
                push_result="pushed to origin/$branch"
                break
            fi
            [ "$attempt" -lt 3 ] || die 1 "origin/$branch kept moving; gave up after 3 attempts"
            attempt=$((attempt + 1))
            fetch_all
            if [ "$(git rev-parse "$origin_ref")" = "$refused_at" ]; then
                push_result="FAILED"
                die 1 "the push was refused although origin/$branch didn't move; git's message is above"
            fi
            say "origin/$branch moved; merging again (attempt $attempt of 3)"
            resync
            note_workflow_changes
        done
    fi
fi

# --- Metrics ---------------------------------------------------------------------------------------------------------

if [ "$metrics" = 1 ]; then
    if [ ! -d extract/b2_us ] || [ ! -d extract/w2_us ]; then
        metrics_result="FAILED: extract/ is missing"
        die 1 "--metrics needs extract/b2_us and extract/w2_us: run ninja extract/b2_us/config.yaml" \
            "extract/w2_us/config.yaml"
    fi
    say "Measuring with port/tools/progress/run_all.sh (about 5 minutes)"
    metrics_result="FAILED"
    sh port/tools/progress/run_all.sh "$root" "$root/build/port-metrics" || die 1 "the metrics run failed"
    metrics_result="see build/port-metrics/tables_b2_us.md and tables_w2_us.md"
    say "Metrics are in $root/build/port-metrics"
fi

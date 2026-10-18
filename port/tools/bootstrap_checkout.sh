#!/usr/bin/env bash
# Prepares a fresh clone or worktree of the port repository for the ROM build and for Claude Code. From a decomp
# checkout that already builds, it links what is never committed: the base ROMs in orig/, the Python environment
# .venv, and the tools that configure.py downloads (wibo, objdiff-cli, sjiswrap.exe). It copies tools/mwccarm (about
# 42 MB) and leaves tools/dsd to configure.py. For Claude Code, it gives the checkout a CLAUDE.local.md that imports
# port/CLAUDE.md, registers port/tools/hooks/port_guard.py in .claude/settings.local.json as a PreToolUse hook for Bash,
# Monitor and the desktop app's terminal panel (mcp__terminal__run_in_terminal), and lists both files in
# .worktreeinclude, so that Claude Code copies them into each worktree it creates. It disables the push URL of the
# upstream remote. It never replaces an existing file or link: it adds what CLAUDE.local.md, .worktreeinclude and
# .claude/settings.local.json lack, replaces only the hook that an earlier version of it registered, and says what to
# add by hand when it can't do that safely.
#
# Usage: port/tools/bootstrap_checkout.sh [--no-links] [DECOMP_CHECKOUT]
#
# DECOMP_CHECKOUT defaults to $POKEBW2_DECOMP, else ~/dev/decomp/pokebw2. --no-links skips the decomp checkout and
# does only the Claude Code and remote steps, for a clone set up by the decomp's README instead. The script prepares
# the clone or worktree it is in, whatever the current directory.
#
# Notes:
# - tools/dsd is not linked: configure.py keeps a tools/dsd that has no tools/dsd.rev whatever its version, so a link
#   would stay at the decomp checkout's version after upstream pins another. configure.py downloads the pinned dsd
#   (DSD_VERSION) and writes tools/dsd.rev.
# - tools/mwccarm is copied, not linked: when any compiler that configure.py needs is missing, it extracts every
#   compiler again, which through a link would rewrite the decomp checkout's compilers while its builds use them.
# - configure.py replaces an outdated wibo or sjiswrap.exe by renaming a new download over the path, which replaces
#   the link here and leaves the decomp checkout's file alone. It downloads objdiff-cli only when it is missing.
# - /orig, /CLAUDE.local.md and /.worktreeinclude go in info/exclude of the git directory that all worktrees share:
#   the tree ignores only orig/*.nds, so a linked orig would show as untracked, and the other two hold this machine's
#   setup. Claude Code copies a .worktreeinclude entry only when git ignores it. The tree's .gitignore already ignores
#   .claude/settings.local.json; if it stops doing so, the script excludes that file too.
# - In the desktop app, a worktree session takes settings and hooks from the clone's root, not from the worktree, so
#   the clone's root needs this script as much as each worktree does.
# - The hook runs the guard from the session's own checkout ($CLAUDE_PROJECT_DIR), else from the clone's main worktree,
#   since a worktree on upstream/main or on an older port commit has no port/tools/hooks/port_guard.py. When neither
#   has it, the hook refuses every command with a message saying so, on purpose.
# - upstream's push URL becomes DISABLED-port-never-pushes-upstream, so a push there fails even outside Claude Code.
#   Fetching is unchanged.
set -euo pipefail

die() {
    printf 'bootstrap_checkout: %s\n' "$*" >&2
    exit 1
}

no_links=0
while [ $# -gt 0 ]; do
    case "$1" in
        -h | --help)
            awk 'NR > 1 && /^#/ { sub(/^# ?/, ""); print; next } NR > 1 { exit }' "$0"
            exit 0
            ;;
        --no-links)
            no_links=1
            shift
            ;;
        -*) die "unknown option $1" ;;
        *) break ;;
    esac
done
[ $# -le 1 ] || die "usage: port/tools/bootstrap_checkout.sh [--no-links] [DECOMP_CHECKOUT]"
[ "$no_links" = 0 ] || [ $# -eq 0 ] || die "--no-links takes no DECOMP_CHECKOUT"

here=$(cd "$(dirname "$0")" && pwd)
root=$(git -C "$here" rev-parse --show-toplevel) || die "$here is not in a git checkout"

# link PATH [NOTE]: links PATH of the decomp checkout to the same path here, unless something is there already
link() {
    local rel=$1 note=${2:-}
    local src=$source_dir/$1 dst=$root/$1
    if [ -L "$dst" ]; then
        if [ "$(readlink "$dst")" = "$src" ]; then
            echo "  $rel: already linked"
        else
            echo "  $rel: kept, it links to $(readlink "$dst")"
        fi
    elif [ -e "$dst" ]; then
        echo "  $rel: kept, a real file is there"
    elif [ ! -e "$src" ]; then
        echo "  $rel: not in the decomp checkout${note:+; $note}"
    else
        mkdir -p "$(dirname "$dst")"
        ln -s "$src" "$dst"
        echo "  $rel: linked"
    fi
}

# copy PATH [NOTE]: copies the directory PATH of the decomp checkout to the same path here, unless something is there
copy() {
    local rel=$1 note=${2:-}
    local src=$source_dir/$1 dst=$root/$1
    if [ -L "$dst" ]; then
        echo "  $rel: kept, it links to $(readlink "$dst"); configure.py may rewrite what it links to"
    elif [ -e "$dst" ]; then
        echo "  $rel: kept, it is already there"
    elif [ ! -d "$src" ]; then
        echo "  $rel: not in the decomp checkout${note:+; $note}"
    else
        mkdir -p "$(dirname "$dst")"
        cp -Rp "$src" "$dst"
        echo "  $rel: copied"
    fi
}

# append_line FILE LINE: adds LINE at the end of FILE, which it creates if needed, on a line of its own
append_line() {
    if [ -s "$1" ] && [ -n "$(tail -c 1 "$1")" ]; then
        printf '\n' >> "$1"
    fi
    printf '%s\n' "$2" >> "$1"
}

if [ "$no_links" = 0 ]; then
    source_dir=${1:-${POKEBW2_DECOMP:-$HOME/dev/decomp/pokebw2}}
    [ -d "$source_dir" ] || die "there is no decomp checkout at $source_dir; pass its path, or use --no-links"
    source_dir=$(cd "$source_dir" && pwd -P)
    [ -f "$source_dir/configure.py" ] || die "$source_dir is not a pokebw2 checkout: it has no configure.py"
    [ "$(cd "$root" && pwd -P)" != "$source_dir" ] || die "$source_dir is this checkout; pass the decomp checkout"

    echo "From $source_dir into $root:"
    link orig "the ROM build needs orig/baserom_b2_us.nds and orig/baserom_w2_us.nds"
    link .venv "create it as the README says"
    link tools/wibo "configure.py downloads it"
    link tools/sjiswrap.exe "configure.py downloads it"
    link tools/objdiff-cli "configure.py downloads it"
    copy tools/mwccarm "configure.py downloads it"

    if [ -L "$root/tools/dsd" ] && [ ! -e "$root/tools/dsd.rev" ]; then
        echo "  tools/dsd: links to $(readlink "$root/tools/dsd") without tools/dsd.rev, so configure.py keeps it" \
            "whatever its version; remove the link to let configure.py download the pinned dsd"
    elif [ ! -e "$root/tools/dsd" ]; then
        echo "  tools/dsd: left to configure.py, which downloads the pinned version"
    fi
fi

echo "Claude Code and remotes, in $root:"

common=$(git -C "$root" rev-parse --git-common-dir)
case "$common" in
    /*) ;;
    *) common=$root/$common ;;
esac
exclude=$common/info/exclude
mkdir -p "$common/info"
patterns="/orig /CLAUDE.local.md /.worktreeinclude"
if ! git -C "$root" check-ignore -q --no-index .claude/settings.local.json; then
    patterns="$patterns /.claude/settings.local.json"
fi
added=""
for pattern in $patterns; do
    if ! grep -qxF "$pattern" "$exclude" 2>/dev/null; then
        append_line "$exclude" "$pattern"
        added="$added $pattern"
    fi
done
[ -z "$added" ] || echo "  info/exclude: added$added ($exclude)"

if [ ! -e "$root/CLAUDE.local.md" ]; then
    append_line "$root/CLAUDE.local.md" '@port/CLAUDE.md'
    echo "  CLAUDE.local.md: created, importing port/CLAUDE.md"
elif grep -qxF '@port/CLAUDE.md' "$root/CLAUDE.local.md"; then
    echo "  CLAUDE.local.md: already imports port/CLAUDE.md"
else
    append_line "$root/CLAUDE.local.md" '@port/CLAUDE.md'
    echo "  CLAUDE.local.md: added @port/CLAUDE.md after what was there"
fi

for entry in CLAUDE.local.md .claude/settings.local.json; do
    if ! grep -qxF "$entry" "$root/.worktreeinclude" 2>/dev/null; then
        append_line "$root/.worktreeinclude" "$entry"
        echo "  .worktreeinclude: added $entry"
    fi
done

if [ ! -f "$root/port/tools/hooks/port_guard.py" ]; then
    echo "  .claude/settings.local.json: not changed, this checkout has no port/tools/hooks/port_guard.py"
else
    mkdir -p "$root/.claude"
    # Adds the hook with Python's json, or prints it when the file isn't JSON it can safely rewrite
    failed="  .claude/settings.local.json: python3 failed; add the hook as port/README.md shows"
    python3 - "$root/.claude/settings.local.json" <<'EOF' || echo "$failed"
import json
import os
import sys
import tempfile

path = sys.argv[1]
# The session's own checkout may lack the guard (a worktree on upstream/main or on an older port commit), so the hook
# falls back to the clone's main worktree, and refuses every command when neither has it: a shell a guard can't check
# stays closed. port/README.md shows the same command.
command = (
    'd=${CLAUDE_PROJECT_DIR:-.}; for r in "$d" "$(git -C "$d" worktree list --porcelain 2>/dev/null'
    " | sed -n '1s/^worktree //p')\"; do"
    ' [ -f "$r/port/tools/hooks/port_guard.py" ] && exec python3 "$r/port/tools/hooks/port_guard.py"; done;'
    ' echo "port_guard: port/tools/hooks/port_guard.py is neither in $d nor in the clone\'s main worktree, so no'
    " command can be checked. Check out the port's main branch in the clone's root, or start the session in a"
    ' checkout that has port/." >&2; exit 2'
)
matcher = "Bash|Monitor|mcp__terminal__run_in_terminal"
entry = {"matcher": matcher, "hooks": [{"type": "command", "command": command, "timeout": 10}]}
# What earlier versions of this script registered: an entry with one of these is brought up to date
OLD = {('Bash|Monitor', 'python3 "$CLAUDE_PROJECT_DIR/port/tools/hooks/port_guard.py"'), ("Bash|Monitor", command)}


def by_hand(why):
    print(f"  .claude/settings.local.json: not changed, {why}. Add this hook to it by hand:")
    for line in json.dumps({"hooks": {"PreToolUse": [entry]}}, indent=2).splitlines():
        print("    " + line)
    sys.exit(0)


settings = {}
existed = os.path.lexists(path)
if existed:
    if os.path.islink(path) or not os.path.isfile(path):
        by_hand("it isn't a plain file")
    try:
        with open(path, encoding="utf-8") as f:
            text = f.read()
        settings = json.loads(text) if text.strip() else {}
    except (OSError, ValueError) as error:
        by_hand(f"it doesn't read as JSON ({error})")
if not isinstance(settings, dict):
    by_hand("it isn't a JSON object")
hooks = settings.setdefault("hooks", {})
if not isinstance(hooks, dict):
    by_hand('its "hooks" isn\'t an object')
pre = hooks.setdefault("PreToolUse", [])
if not isinstance(pre, list):
    by_hand('its "hooks"."PreToolUse" isn\'t a list')
updated = False
for group in list(pre):
    group_hooks = group.get("hooks") if isinstance(group, dict) else None
    for hook in list(group_hooks) if isinstance(group_hooks, list) else []:
        if not (isinstance(hook, dict) and "port_guard.py" in str(hook.get("command", ""))):
            continue
        if group.get("matcher") == matcher and hook.get("command") == command:
            print("  .claude/settings.local.json: already runs port_guard.py")
            sys.exit(0)
        if (group.get("matcher"), hook.get("command")) not in OLD:
            print("  .claude/settings.local.json: not changed, it runs port_guard.py in a hook of its own; compare it"
                  " with the hook in port/README.md")
            sys.exit(0)
        # This script's own earlier hook: replaced by the current one
        group_hooks.remove(hook)
        if not group_hooks:
            pre.remove(group)
        updated = True
pre.append(entry)
fd, temporary = tempfile.mkstemp(dir=os.path.dirname(path), prefix=".settings.local.", suffix=".json")
with os.fdopen(fd, "w", encoding="utf-8") as f:
    json.dump(settings, f, indent=2)
    f.write("\n")
os.chmod(temporary, os.stat(path).st_mode & 0o777 if existed else 0o644)
os.replace(temporary, path)
done = "updated" if updated else "added" if existed else "created with"
print(f"  .claude/settings.local.json: {done} the port_guard.py hook")
EOF
fi

disabled=DISABLED-port-never-pushes-upstream
if ! git -C "$root" remote get-url upstream >/dev/null 2>&1; then
    echo "  upstream: no such remote; add it as port/README.md says, then run this again to disable its push URL"
elif [ "$(git -C "$root" config --get-all remote.upstream.pushurl || true)" = "$disabled" ]; then
    echo "  upstream: push URL already disabled"
else
    git -C "$root" config --unset-all remote.upstream.pushurl || true
    git -C "$root" remote set-url --push upstream "$disabled"
    echo "  upstream: push URL set to $disabled; fetching is unchanged"
fi

# gh's default repository: port_guard.py lets a gh write without -R through only when it is the port. When
# gh repo set-default named a remote, gh-resolved is "base" and the repository is that remote's URL. A clone made
# before the repository was renamed CallanCampos/Frigate (2026-10-18) has the old name in origin's URL until it is
# updated, so the default it resolves to is the old name.
gh_default=$(git -C "$root" config --get-regexp '^remote\..*\.gh-resolved$' || true)
gh_remote=""
case "$gh_default" in
    *" base")
        gh_remote=${gh_default%% *}
        gh_remote=${gh_remote#remote.}
        gh_remote=${gh_remote%.gh-resolved}
        gh_default=$(git -C "$root" remote get-url "$gh_remote" 2>/dev/null || true)
        ;;
esac
case "$(printf '%s' "$gh_default" | tr '[:upper:]' '[:lower:]')" in
    *[[:space:]/:]callancampos/frigate | *[[:space:]/:]callancampos/frigate.git | *[[:space:]/:]callancampos/frigate/)
        echo "  gh: the default repository is the port"
        ;;
    *)
        if [ "$gh_remote" = origin ]; then
            echo "  gh: the default repository is origin, whose URL isn't CallanCampos/Frigate's (a clone made" \
                "before the rename keeps the old name); run" \
                "git remote set-url origin https://github.com/CallanCampos/Frigate.git"
        else
            echo "  gh: the default repository isn't the port; run gh repo set-default CallanCampos/Frigate"
        fi
        ;;
esac

for version in b2_us w2_us; do
    if [ ! -e "$root/orig/baserom_$version.nds" ]; then
        echo "Warning: orig/baserom_$version.nds is missing; the ROM build and sync_upstream.sh need both base ROMs."
    fi
done

# What configure.py and ninja need from the system, as docs/building.md lists it
llvm_path=""
for llvm_bin in /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin; do
    if [ -d "$llvm_bin" ]; then
        llvm_path=$llvm_bin
        break
    fi
done
if [ -z "$llvm_path" ] && { ! command -v clang >/dev/null 2>&1 || ! command -v llvm-objcopy >/dev/null 2>&1; }; then
    echo "Missing: clang and llvm-objcopy (macOS: brew install llvm)."
fi
command -v ninja >/dev/null 2>&1 || echo "Missing: ninja (macOS: brew install ninja)."
command -v sha1sum >/dev/null 2>&1 || echo "Missing: sha1sum, which the build runs (macOS: brew install coreutils)."
if [ "$(uname -s)" = Darwin ] && [ "$(uname -m)" = arm64 ] && ! arch -x86_64 /usr/bin/true >/dev/null 2>&1; then
    echo "Missing: Rosetta 2, which the x86-64 wibo needs (softwareupdate --install-rosetta)."
fi

echo
echo "Next, in $root:"
if [ -n "$llvm_path" ]; then
    echo "  export PATH=\"$llvm_path:\$PATH\""
fi
echo "  python3 configure.py"
echo "  ninja constants_headers extract/b2_us/config.yaml extract/w2_us/config.yaml"
echo "Then the full build with both SHA1s (about 5 minutes), or a sync, which runs it:"
echo "  ninja"
echo "  port/tools/sync_upstream.sh"

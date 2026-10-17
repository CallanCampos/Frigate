#!/usr/bin/env bash
# Prepares a fresh clone or worktree of the port repository for the ROM build. From a decomp checkout that already
# builds, it links what is never committed: the base ROMs in orig/, the Python environment .venv, and the tools that
# configure.py downloads (wibo, objdiff-cli, sjiswrap.exe). It copies tools/mwccarm (about 42 MB) and leaves tools/dsd
# to configure.py. It never replaces an existing file or link.
#
# Usage: port/tools/bootstrap_checkout.sh [DECOMP_CHECKOUT]
#
# DECOMP_CHECKOUT defaults to $POKEBW2_DECOMP, else ~/dev/decomp/pokebw2. The script prepares the clone or worktree it
# is in, whatever the current directory.
#
# Notes:
# - tools/dsd is not linked: configure.py keeps a tools/dsd that has no tools/dsd.rev whatever its version, so a link
#   would stay at the decomp checkout's version after upstream pins another. configure.py downloads the pinned dsd
#   (DSD_VERSION) and writes tools/dsd.rev.
# - tools/mwccarm is copied, not linked: when any compiler that configure.py needs is missing, it extracts every
#   compiler again, which through a link would rewrite the decomp checkout's compilers while its builds use them.
# - configure.py replaces an outdated wibo or sjiswrap.exe by renaming a new download over the path, which replaces
#   the link here and leaves the decomp checkout's file alone. It downloads objdiff-cli only when it is missing.
# - /orig and /CLAUDE.local.md are added to the repository's info/exclude: the tree ignores only orig/*.nds, so a
#   linked orig would show as untracked, and CLAUDE.local.md holds notes for this machine only.
set -euo pipefail

die() {
    printf 'bootstrap_checkout: %s\n' "$*" >&2
    exit 1
}

case "${1:-}" in
    -h | --help)
        awk 'NR > 1 && /^#/ { sub(/^# ?/, ""); print; next } NR > 1 { exit }' "$0"
        exit 0
        ;;
esac
[ $# -le 1 ] || die "usage: port/tools/bootstrap_checkout.sh [DECOMP_CHECKOUT]"

here=$(cd "$(dirname "$0")" && pwd)
root=$(git -C "$here" rev-parse --show-toplevel) || die "$here is not in a git checkout"
source_dir=${1:-${POKEBW2_DECOMP:-$HOME/dev/decomp/pokebw2}}
[ -d "$source_dir" ] || die "there is no decomp checkout at $source_dir; pass its path"
source_dir=$(cd "$source_dir" && pwd -P)
[ -f "$source_dir/configure.py" ] || die "$source_dir is not a pokebw2 checkout: it has no configure.py"
[ "$(cd "$root" && pwd -P)" != "$source_dir" ] || die "$source_dir is this checkout; pass the decomp checkout"

echo "From $source_dir into $root:"

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

link orig "the ROM build needs orig/baserom_b2_us.nds and orig/baserom_w2_us.nds"
link .venv "create it as the README says"
link tools/wibo "configure.py downloads it"
link tools/sjiswrap.exe "configure.py downloads it"
link tools/objdiff-cli "configure.py downloads it"

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

copy tools/mwccarm "configure.py downloads it"

if [ -L "$root/tools/dsd" ] && [ ! -e "$root/tools/dsd.rev" ]; then
    echo "  tools/dsd: links to $(readlink "$root/tools/dsd") without tools/dsd.rev, so configure.py keeps it" \
        "whatever its version; remove the link to let configure.py download the pinned dsd"
elif [ ! -e "$root/tools/dsd" ]; then
    echo "  tools/dsd: left to configure.py, which downloads the pinned version"
fi

for version in b2_us w2_us; do
    if [ ! -e "$root/orig/baserom_$version.nds" ]; then
        echo "Warning: orig/baserom_$version.nds is missing; the ROM build and sync_upstream.sh need both base ROMs."
    fi
done

exclude=$(git -C "$root" rev-parse --git-path info/exclude)
case "$exclude" in
    /*) ;;
    *) exclude=$root/$exclude ;;
esac
mkdir -p "$(dirname "$exclude")"
for pattern in /orig /CLAUDE.local.md; do
    if ! grep -qxF "$pattern" "$exclude" 2>/dev/null; then
        printf '%s\n' "$pattern" >> "$exclude"
        echo "Added $pattern to $exclude"
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

#!/usr/bin/env bash
# Merges a regenerated source-buka-2003 into the port and rebuilds both
# programs.
#
#   tools/port/sync.sh [NEW_SOURCE_REF]     (default: origin/source-buka-2003)
#
# Each regeneration of the source branch is a new root commit, unrelated to
# the last one. The merge takes the root the port merged last as its base, so
# it brings in exactly what the regeneration changed, and records the new
# root as the merge's second parent. On a conflict git stops: resolve, `git
# add` the files, `git commit`, then run this script again with --build-only.
set -euo pipefail

cd "$(git rev-parse --show-toplevel)"
build_only=false
target=origin/source-buka-2003
for argument in "$@"; do
    case "$argument" in
        --build-only) build_only=true ;;
        *) target=$argument ;;
    esac
done

if ! $build_only; then
    if [ -n "$(git status --porcelain --untracked-files=no)" ]; then
        echo "the working tree has changes; commit or stash them first" >&2
        exit 1
    fi
    case "$target" in
        origin/*) git fetch origin "${target#origin/}" ;;
    esac
    old=$(git log -1 --format=%H --grep='^Generated-By: homm1 clean$' HEAD)
    new=$(git rev-parse "$target")
    if [ "$old" = "$new" ]; then
        echo "already on $target ($new)"
    else
        echo "merging $(git log -1 --format=%s "$new") (base $old)"
        git rev-parse "$new" >"$(git rev-parse --git-path MERGE_HEAD)"
        echo "Merge $(git log -1 --format=%s "$new")" >"$(git rev-parse --git-path MERGE_MSG)"
        if ! git merge-recursive "$old" -- HEAD "$new"; then
            echo "resolve the conflicts, git add and git commit, then rerun with --build-only" >&2
            exit 1
        fi
        git commit --no-edit
    fi
fi

nix develop -c python3 catalog.py check
nix develop -c python3 catalog.py update --check
nix develop -c python3 build.py --target all
nix develop .#port -c cmake -S . -B build/port -G Ninja
nix develop .#port -c ninja -C build/port
nix develop .#port -c ctest --test-dir build/port --output-on-failure

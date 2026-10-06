#!/usr/bin/env bash
# Replays the port's commits onto a regenerated source-buka-2003 and rebuilds
# both programs.
#
#   tools/port/sync.sh [NEW_SOURCE_REF]     (default: origin/source-buka-2003)
#
# The port branch is the source branch's single root commit plus the port's
# commits; a regenerated source branch is a new root. The replay is
# `git rebase --rebase-merges --onto NEW OLD_ROOT`, which keeps the merges of
# the side branches; a merge's own conflict resolutions are redone by hand. On
# a conflict git stops: resolve, run `git rebase --continue`, then run this
# script again with --build-only.
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
    old=$(git rev-list --max-parents=0 HEAD)
    new=$(git rev-parse "$target")
    if [ "$old" = "$new" ]; then
        echo "already on $target ($new)"
    else
        echo "replaying $(git rev-list --count "$old"..HEAD) port commits from $old onto $new"
        git rebase --rebase-merges --onto "$new" "$old"
    fi
fi

nix develop -c python3 catalog.py check
nix develop -c python3 catalog.py update --check
nix develop -c python3 build.py --target all
nix develop .#port -c cmake -S . -B build/port -G Ninja
nix develop .#port -c ninja -C build/port
nix develop .#port -c ctest --test-dir build/port --output-on-failure

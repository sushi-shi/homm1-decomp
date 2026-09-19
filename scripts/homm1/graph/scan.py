# Ported from scripts/gruntz/graph/scan.py at b1de0e555576a215898907b8ec8ed5423368883e.
"""homm1.graph.scan - repo-local `#include` deps for the `cl` edges.

The `cl` rule lists only the .cpp as its input, so without the headers as
implicit deps a header-only edit (a struct-offset fix in a shared .h) does not
rebuild the dependent object, and the next diff is read off STALE code. MSVC
5.0 rejects `/showIncludes` (D4002), so cl can emit no depfile and the scan is
what we have.

That makes the dep lists CONFIGURE-TIME facts, baked from the include graph as
it stood. Any edit that changes that graph - a new `#include`, a renamed or
deleted header - invalidates them silently. `scanned()` therefore returns
every repo-local file the scan READ, and the generator edge depends on the
whole set: the scan invalidates itself.

Resolution mirrors cl's own search: a quoted include relative to the including
file's directory first (the `All-aggregated.cpp` pattern pulls sibling .cpp
files that way), then the `include/` tree that homm1.tool.cl puts on `/I`.
System headers resolve to nothing under the repo and are skipped - the MSVC
and DX SDK headers are pinned by the toolchain and never change under a build.
"""

from __future__ import annotations

import os
import re

from homm1.core.inputs import REPO

_INCLUDE_RE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]', re.M)


class Scanner:
    """Memoized include scanner. One instance per configure run."""

    def __init__(self) -> None:
        self._direct: dict[str, list[str]] = {}
        self._read: set[str] = set()

    def scanned(self) -> set[str]:
        """Every repo-local file read so far - the generator edge's dep set."""
        return set(self._read)

    def _direct_includes(self, rel: str) -> list[str]:
        """The repo-local files `rel` includes directly, repo-relative.

        `Path.resolve()` is a syscall per component, and the include walk
        restarts per unit over the same shared headers, so this used to cost
        ~180k resolves. The repo has no symlinked headers and the paths are
        already repo-relative: string normpath is both correct and ~40x
        cheaper.
        """
        hit = self._direct.get(rel)
        if hit is not None:
            return hit
        out: list[str] = []
        self._direct[rel] = out          # cycle guard: self-referential includes
        self._read.add(rel)
        try:
            text = (REPO / rel).read_text(encoding="utf-8", errors="replace")
        except OSError:
            return out
        parent = os.path.dirname(rel)
        for inc in _INCLUDE_RE.findall(text):
            for base in (parent, "include"):
                hrel = os.path.normpath(os.path.join(base, inc))
                if hrel.startswith(".."):
                    continue             # outside the repo - not our dep
                if (REPO / hrel).exists():
                    out.append(hrel)
                    break
        return out

    def headers(self, source: str) -> list[str]:
        """The transitive repo-local include closure of `source`, sorted."""
        seen: set[str] = set()
        stack = [source]
        while stack:
            for hrel in self._direct_includes(stack.pop()):
                if hrel not in seen:
                    seen.add(hrel)
                    stack.append(hrel)
        return sorted(seen)

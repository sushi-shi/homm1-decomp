"""homm1.lsp.query - the refs / hover verbs (and the index builder).

A target is either `file:line[:col]` (1-based, as grep/editors print) or a
symbol name. Symbols go through clangd's workspace-symbol index: `CGrunt`
matches the class, `CGrunt::GetAI` the qualified member; an ambiguous query
lists every candidate and exits rather than guessing. Cross-TU reference
answers come from the background index (build/clangd/.cache/) - early runs on
a cold cache may be partial; `homm1 lsp index` warms it.

A target that LOOKS like a point (`path:line`) but names no readable file is
reported as the missing file it is - never re-tried as a symbol, which used to
answer 'no workspace symbol matches include/Foo.h:12'.
"""

from __future__ import annotations

import re
import time
from pathlib import Path

from homm1.core.paths import REPO
from homm1.tool import ToolError, clangd

_IDENT = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
_POINT = re.compile(r"^(.+?):(\d+)(?::(\d+))?$")


def rel(uri_or_path: str) -> str:
    p = uri_or_path.removeprefix("file://")
    try:
        return str(Path(p).relative_to(REPO))
    except ValueError:
        return p


def fmt_location(loc: dict) -> str:
    r = loc["range"]["start"]
    return f"{rel(loc['uri'])}:{r['line'] + 1}:{r['character'] + 1}"


def parse_point(target: str) -> tuple[Path, int, int | None] | None:
    """(path, 1-based line, 1-based col|None) when `target` names a real file
    position; None means it is a symbol query. A `path:line` whose file does
    not exist is an ERROR here - falling through to the symbol index would
    answer a file question with 'no workspace symbol matches'."""
    m = _POINT.match(target)
    if not m:
        return None
    path = (REPO / m.group(1)).resolve()
    if not path.is_file():
        raise SystemExit(f"[lsp] no such file: {m.group(1)} (a `file:line[:col]`"
                         f" target is resolved against {REPO})")
    return path, int(m.group(2)), int(m.group(3)) if m.group(3) else None


def positions_to_probe(path: Path, line: int, col: int | None):
    """1-based -> 0-based; without a column, probe each identifier on the
    line right-to-left (the declared name sits rightmost on C++ decls)."""
    if col is not None:
        yield line - 1, col - 1
        return
    lines = path.read_text(errors="replace").splitlines()
    if not 1 <= line <= len(lines):
        raise SystemExit(f"[lsp] {rel(str(path))} has no line {line}")
    for m in reversed(list(_IDENT.finditer(lines[line - 1]))):
        yield line - 1, m.start()


def _symbol_label(s: dict) -> str:
    container = s.get("containerName") or ""
    return f"{container}{'::' if container else ''}{s['name']}"


def resolve_symbol(lsp: clangd.Clangd, name: str) -> tuple[Path, int, int]:
    """One (path, line0, char0) for `name` via workspace/symbol - exact
    (qualified) matches win; anything ambiguous is listed, not guessed."""
    # shards load lazily after a didOpen; retry briefly while they fill
    lsp.open_file(clangd.first_compdb_file())
    syms = []
    for _ in range(10):
        syms = lsp.workspace_symbols(name)
        if syms:
            break
        time.sleep(2)
    if not syms:
        raise SystemExit(f"[lsp] no workspace symbol matches {name!r} "
                         f"(cold index? run `homm1 lsp index`)")
    exact = [s for s in syms if _symbol_label(s) == name] \
        or [s for s in syms if s["name"] == name]
    if len(exact) != 1:
        pool = exact or syms
        print(f"[lsp] {name!r} is ambiguous ({len(pool)} candidates):")
        for s in pool[:25]:
            print(f"  {_symbol_label(s)}  {fmt_location(s['location'])}")
        raise SystemExit(2)
    loc = exact[0]["location"]
    r = loc["range"]["start"]
    return Path(loc["uri"].removeprefix("file://")), r["line"], r["character"]


def _settled_references(lsp: clangd.Clangd, path: Path, ln: int, ch: int) -> list:
    """references, re-queried until the answer stops growing: the background
    index loads its shards asynchronously after the CDB kick, and the first
    reply can be the open file's sites only. Two equal consecutive counts
    (or ~10s) is 'settled' - much cheaper than a full wait_for_index."""
    result = lsp.references(path, ln, ch)
    if not result:
        return result
    for _ in range(10):
        time.sleep(1)
        again = lsp.references(path, ln, ch)
        if len(again) == len(result):
            return again
        result = again
    return result


def run_point_or_symbol(verb: str, target: str) -> int:
    """refs / hover on a `file:line[:col]` point or a symbol name."""
    point = parse_point(target)
    try:
        lsp = clangd.Clangd()
    except ToolError as e:
        raise SystemExit(f"[lsp] {e}") from e
    try:
        # opening a compdb-listed file kicks clangd's lazy CDB discovery,
        # which loads the background-index shards cross-TU refs come from
        lsp.open_file(clangd.first_compdb_file())
        if point:
            path, line, col = point
            probes = list(positions_to_probe(path, line, col))
        else:
            path, line0, char0 = resolve_symbol(lsp, target)
            probes = [(line0, char0), *positions_to_probe(path, line0 + 1, None)]
        lsp.open_file(path)
        for ln, ch in probes:
            if verb == "hover":
                result = lsp.hover(path, ln, ch)
                if result:
                    print(result["contents"]["value"])
                    return 0
            else:
                result = _settled_references(lsp, path, ln, ch)
                if result:
                    for loc in result:
                        print(fmt_location(loc))
                    print(f"({len(result)} reference(s), declaration included)")
                    return 0
        print("(no result)")
        return 1        # answered-NO: a valid point with nothing there
    finally:
        lsp.close()


def run_index() -> int:
    try:
        lsp = clangd.Clangd()
    except ToolError as e:
        raise SystemExit(f"[lsp] {e}") from e
    try:
        # CDB discovery (and thus indexing) starts lazily on the first didOpen
        lsp.open_file(clangd.first_compdb_file())
        lsp.wait_for_index()
    finally:
        lsp.close()
    return 0

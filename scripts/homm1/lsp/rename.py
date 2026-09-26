"""homm1.lsp.rename - the type-aware bulk member renamer.

Renames a class's data members across the WHOLE tree via clangd's
`textDocument/rename`: one call requests a cross-TU WorkspaceEdit for the
declaration and its indexed references (member access, `&C::m_x`, offsetof,
designated init). Macro bodies and token-pasted identifiers may be omitted;
the returned edit is not proof that every use was found. Rename keys on the
symbol's USR, so only the named class's member moves and never a same-named
field of a different struct - dozens of unrelated classes reuse names like
`m_5c`, and a text sed would wreck them.

    homm1 lsp rename CGameObject::m_5c m_screenX m_60=m_screenY
    homm1 lsp rename CGameObject --map cgo.map [--dry-run]

The class's field decls are found in its header (auto-located under include/,
or --header). Every returned edit is verified to currently read the OLD name
before anything is written; edits apply bottom-to-top per file so positions
stay valid; only src/** and include/** are writable (vendor/SDK edits are
skipped and reported). All fields rename against clangd's ORIGINAL in-memory
buffers, so a batch is internally consistent and a re-run is idempotent (the
already-renamed old names are simply absent).

Cross-file completeness needs the background index; the tool blocks until it
settles. The wine `cl` build is the safety net: an index-missed site fails to
compile (`m_old is not a member of C`) because the decl itself was renamed.
Also reparse unused template bodies with the declaration-name audit.
Identifier changes can reorder uninitialized storage; compare actual objects
and typed referents. The tool only PRINTS the re-prove
ritual (homm1 build - the MAX gate) - it never runs it.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

from homm1.core.paths import INCLUDE, REPO
from homm1.lsp.query import rel
from homm1.tool import ToolError, clangd


# --------------------------------------------------------------------------- #
# source helpers: mask comments/strings, locate a class body + a field decl
# --------------------------------------------------------------------------- #
def mask_noncode(text: str) -> str:
    """A same-length copy with comment/string/char spans blanked (newlines
    kept), so brace matching and identifier scans ignore literals. Positions
    map 1:1 for offset -> line/col conversion."""
    n = len(text)
    out = list(text)
    i = 0
    while i < n:
        c = text[i]
        if c == "/" and i + 1 < n and text[i + 1] == "/":
            while i < n and text[i] != "\n":
                out[i] = " "
                i += 1
            continue
        if c == "/" and i + 1 < n and text[i + 1] == "*":
            k = i
            while k < n and not (text[k] == "*" and k + 1 < n
                                 and text[k + 1] == "/"):
                if text[k] != "\n":
                    out[k] = " "
                k += 1
            end = min(k + 2, n)
            for p in range(k, end):
                if text[p] != "\n":
                    out[p] = " "
            i = end
            continue
        if c in "\"'":
            quote = c
            out[i] = " "
            k = i + 1
            while k < n:
                if text[k] == "\\" and k + 1 < n:
                    out[k] = out[k + 1] = " "
                    k += 2
                    continue
                if text[k] == quote:
                    out[k] = " "
                    k += 1
                    break
                if text[k] != "\n":
                    out[k] = " "
                k += 1
            i = k
            continue
        i += 1
    return "".join(out)


def find_class_block(masked: str, class_name: str) -> tuple[int, int] | None:
    """(open_brace, close_brace_exclusive) of the DEFINITION of
    struct/class `class_name`; forward declarations (no `{`) are skipped."""
    pat = re.compile(r"\b(?:struct|class)\s+" + re.escape(class_name)
                     + r"\b(?:\s*:\s*[^{;]+)?\s*\{")
    for m in pat.finditer(masked):
        brace = masked.find("{", m.start())
        if brace < 0:
            continue
        depth, i, n = 0, brace, len(masked)
        while i < n:
            if masked[i] == "{":
                depth += 1
            elif masked[i] == "}":
                depth -= 1
                if depth == 0:
                    return brace, i + 1
            i += 1
    return None


def offset_to_linecol(text: str, offset: int) -> tuple[int, int]:
    line = text.count("\n", 0, offset)
    return line, offset - (text.rfind("\n", 0, offset) + 1)


def find_field_decl(text: str, masked: str, block: tuple[int, int],
                    field: str) -> tuple[int, int] | None:
    """The field's DECLARATION identifier position (0-based line, col) inside
    the class block: `<type> field [array] (;|: bitfield)`."""
    start, end = block
    ident = re.compile(r"\b" + re.escape(field) + r"\s*(?:\[[^\]]*\])?\s*(?::|;)")
    hits = list(ident.finditer(masked, start, end))
    if not hits:
        return None
    if len(hits) > 1:
        print(f"  warn: {field}: {len(hits)} decl-like matches in the class "
              f"body; using the first", file=sys.stderr)
    return offset_to_linecol(text, hits[0].start())


def locate_header(cls: str, header_arg: str | None) -> Path:
    if header_arg:
        p = Path(header_arg)
        return p if p.is_absolute() else (REPO / p).resolve()
    rx = re.compile(r"\b(?:struct|class)\s+" + re.escape(cls) + r"\b[^;{]*\{")
    for h in sorted(INCLUDE.rglob("*.h")):
        if rx.search(mask_noncode(h.read_text(errors="replace"))):
            return h
    raise SystemExit(f"[lsp rename] no header defining {cls} under include/ "
                     f"(pass --header)")


# --------------------------------------------------------------------------- #
# WorkspaceEdit normalization + application
# --------------------------------------------------------------------------- #
def uri_to_path(uri: str) -> Path:
    return Path(uri.removeprefix("file://"))


def normalize_edits(ws_edit) -> dict[Path, list]:
    """WorkspaceEdit -> {Path: [TextEdit,...]}, both response shapes."""
    out: dict[Path, list] = {}
    if not ws_edit:
        return out
    for uri, edits in (ws_edit.get("changes") or {}).items():
        out.setdefault(uri_to_path(uri), []).extend(edits)
    for dc in ws_edit.get("documentChanges") or []:
        out.setdefault(uri_to_path(dc["textDocument"]["uri"]), []).extend(
            dc.get("edits", []))
    return out


def line_starts(text: str) -> list[int]:
    starts = [0]
    for i, ch in enumerate(text):
        if ch == "\n":
            starts.append(i + 1)
    return starts


def is_ours(path: Path) -> bool:
    """Only OUR reconstructed sources are writable; vendor/SDK are not."""
    try:
        parts = path.resolve().relative_to(REPO).parts
    except ValueError:
        return False
    return bool(parts) and parts[0] in ("src", "include") \
        and "vendor" not in parts


# --------------------------------------------------------------------------- #
# the verb
# --------------------------------------------------------------------------- #
def parse_mapping(args) -> tuple[str, list[tuple[str, str]]]:
    """(class, [(old, new), ...]) from `Class::old new [old=new ...]`,
    `Class [old=new ...]` and --map, duplicates warned away."""
    cls, sep, first_old = args.target.partition("::")
    rest = list(args.rest)
    mapping: list[tuple[str, str]] = []
    seen: set[str] = set()

    def add(old: str, new: str) -> None:
        if not old or not new:
            raise SystemExit(f"[lsp rename] bad pair {old!r}={new!r}")
        if old in seen:
            print(f"  warn: duplicate old field {old}; ignoring later",
                  file=sys.stderr)
            return
        seen.add(old)
        mapping.append((old, new))

    if sep:
        if not rest or "=" in rest[0]:
            raise SystemExit(f"[lsp rename] {args.target}: the new name must "
                             f"follow (rename {cls}::{first_old} <m_new>)")
        add(first_old, rest.pop(0))
    if args.map_file:
        map_path = Path(args.map_file)
        if not map_path.is_file():
            raise SystemExit(f"[lsp rename] no mapping file at {args.map_file} "
                             "(one `old=new` or `old new` pair per line)")
        for raw in map_path.read_text().splitlines():
            line = raw.split("#", 1)[0].strip()
            if not line:
                continue
            parts = re.split(r"[=\s]+", line)
            if len(parts) != 2:
                raise SystemExit(f"[lsp rename] bad map line: {raw!r}")
            add(parts[0], parts[1])
    for p in rest:
        if "=" not in p:
            raise SystemExit(f"[lsp rename] extra pair must be old=new: {p!r}")
        old, new = p.split("=", 1)
        add(old.strip(), new.strip())
    if not mapping:
        raise SystemExit("[lsp rename] no old=new pairs (Class::old new, "
                         "positional old=new, or --map)")
    return cls, mapping


def run_rename(args) -> int:
    cls, mapping = parse_mapping(args)
    header = locate_header(cls, args.header)
    print(f"class {cls}: header {rel(str(header))}", file=sys.stderr)

    htext = header.read_text()
    hmask = mask_noncode(htext)
    block = find_class_block(hmask, cls)
    if block is None:
        raise SystemExit(f"[lsp rename] no definition (with body) of {cls} "
                         f"in {rel(str(header))}")

    positions: dict[str, tuple[tuple[int, int], str]] = {}
    for old, new in mapping:
        pos = find_field_decl(htext, hmask, block, old)
        if pos is None:
            print(f"  skip {old}: no decl in {cls} body (already renamed?)",
                  file=sys.stderr)
            continue
        positions[old] = (pos, new)

    if args.audit:
        run_audit(cls, [old for old, _ in mapping])
        if not positions:
            return 0
    if not positions:
        print("nothing to rename (all fields missing).")
        return 1

    try:
        lsp = clangd.Clangd()
    except ToolError as e:
        raise SystemExit(f"[lsp rename] {e}") from e
    per_file: dict[Path, list] = {}
    per_field: dict[str, tuple[int, int, str]] = {}
    skipped_foreign: set[str] = set()
    try:
        # kick lazy CDB discovery -> background indexing, then wait it out:
        # cross-file completeness is the whole point of the LSP rename
        lsp.open_file(clangd.first_compdb_file())
        lsp.open_file(header)
        print("index: waiting for the background index to settle...",
              file=sys.stderr)
        lsp.wait_for_index()

        for old, ((line, col), new) in positions.items():
            edits = normalize_edits(lsp.rename(header, line, col, new))
            if not edits:
                print(f"  warn: {old}->{new}: clangd returned NO edits "
                      f"(index gap? decl position wrong?)", file=sys.stderr)
                continue
            nfiles = nsites = 0
            for path, tedits in edits.items():
                if not is_ours(path):
                    skipped_foreign.add(str(path))
                    continue
                # every edit must currently read the OLD name - guards a
                # mis-resolved symbol renaming the wrong token
                content = path.read_text()
                starts = line_starts(content)
                for e in tedits:
                    r = e["range"]
                    s = starts[r["start"]["line"]] + r["start"]["character"]
                    t = starts[r["end"]["line"]] + r["end"]["character"]
                    if content[s:t] != old:
                        raise SystemExit(
                            f"[lsp rename] {old}->{new}: an edit in "
                            f"{rel(str(path))} does NOT read {old!r} - "
                            f"aborting (stale index?)")
                per_file.setdefault(path, []).extend(
                    (e["range"], e["newText"]) for e in tedits)
                nfiles += 1
                nsites += len(tedits)
            per_field[old] = (nfiles, nsites, new)
            print(f"  {old} -> {new}: {nsites} site(s) in {nfiles} file(s)")
    finally:
        lsp.close()

    for f in sorted(skipped_foreign):
        print(f"  (skipped foreign file: {rel(f)})", file=sys.stderr)
    total_sites = sum(v[1] for v in per_field.values())
    print(f"\ntotal: {total_sites} site(s) across {len(per_file)} file(s) "
          f"({len(per_field)} field(s))")
    if args.dry_run:
        print("[dry-run] no files written.")
        return 0

    for path, edits in per_file.items():
        content = path.read_text()
        starts = line_starts(content)
        resolved = sorted(
            ((starts[r["start"]["line"]] + r["start"]["character"],
              starts[r["end"]["line"]] + r["end"]["character"], new_text)
             for r, new_text in edits),
            key=lambda x: x[0], reverse=True)
        prev_start = None
        for s, t, _ in resolved:
            if prev_start is not None and t > prev_start:
                raise SystemExit(f"[lsp rename] overlapping edits in "
                                 f"{rel(str(path))} - aborting")
            prev_start = s
        for s, t, new_text in resolved:
            content = content[:s] + new_text + content[t:]
        path.write_text(content)
    print(f"wrote {len(per_file)} file(s).")
    print("\nverify the rename against the actual compiler and objects:\n    homm1 build      # rebuild + verify check "
          "(the MAX gate)")
    return 0


# --------------------------------------------------------------------------- #
# optional clang-query audit: type-scoped member accesses of the OLD names,
# so a clangd index gap is visible (and hand-fixable) instead of silent
# --------------------------------------------------------------------------- #
def run_audit(cls: str, fields: list[str]) -> None:
    import json
    db = json.loads(clangd.CDB.read_text())
    tus = [e["file"] for e in db if e["file"].startswith("src/")]
    print(f"[audit] clang-query over {len(tus)} TUs for {cls} member "
          f"accesses", file=sys.stderr)
    for field in fields:
        matcher = (
            f'match memberExpr(member(hasName("{field}")), hasObjectExpression('
            f'anyOf(hasType(pointsTo(recordDecl(hasName("{cls}")))),'
            f'hasType(recordDecl(hasName("{cls}")))))).bind("x")')
        try:
            out = clangd.clang_query(matcher, tus)
        except ToolError as e:
            print(f"[audit] {e}", file=sys.stderr)
            return
        sites = sorted(set(re.findall(r"(\S+:\d+:\d+): note: \"x\" binds",
                                      out)))
        print(f"  {cls}::{field}: {len(sites)} memberExpr access site(s)")
        for s in sites:
            print(f"    {s}")

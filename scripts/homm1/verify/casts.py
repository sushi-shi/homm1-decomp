"""homm1.verify.casts - the cast campaign's one check (fast tier).

Three ported behaviors, one module (they are one campaign):

  * the LEDGER: every reinterpret_cast is either structurally FORCED, carries
    a reason from the closed vocabulary within three lines, or is OPEN.
    OPEN is the debt driven to zero. Reviewed casts remain visible in TOTAL:
    original source can prove a type-erasing callback boundary that C++ cannot
    express without a cast, so a forced seam is an audited model fact rather
    than textual debt. The gate fails when OPEN exceeds the committed
    unexplained-casts floor (the board's baseline carries it).
  * SELF-RECURSION (merged from self_recursion.py): a one-line accessor whose
    only statement calls the same-arity overload of ITSELF is always the
    seam-sweep footgun (the sweep rewrote the seam's own body). Calls to a
    different-arity overload are ordinary forwarding. FATAL, no allowlist.
  * NESTED static_casts (merged from nested_static_casts.py, full tier): a
    libclang AST scan for a static_cast whose operand is another static_cast.
    The board's `nested static_casts` floor ratchets it.

    python3 -m homm1.verify.casts            # summary + the OPEN worklist
    python3 -m homm1.verify.casts --gate     # ledger + self-recursion gate
    python3 -m homm1.verify.casts --nested   # the AST scan (needs compdb)
"""

from __future__ import annotations

import collections
import re
import sys

from homm1.core.paths import BUILD, REPO

ROOTS = ("src", "include")

# A cast matching one of these is structurally forced; the label says why.
FORCED = [
    ("mfc-position",
     r"GetHeadPosition\(\)|GetStartPosition\(\)|<POSITION>|m_posCache"),
    ("mfc-voidref-out", r"void\s*\*\s*&"),
    ("win32-abi",
     r"<H[A-Z]\w*>|<LP[A-Z]\w*>|<LPARAM>|<WPARAM>|<LRESULT>|<DLGPROC>|<WNDPROC>"),
    ("i64-halves-pun", r"<i64\s*\*>\s*\(\s*&|<u64\s*\*>\s*\(\s*&"),
]

# The closed reason vocabulary. THE WINDOW IS THREE LINES ABOVE THE CAST and
# it bites (clang-format has un-explained casts by pushing the word out of
# range); the wrap-immune placement is a trailing comment on the cast's line.
REASON = re.compile(
    r"language-forced|API-forced|forced by|byte-forced|byte-evidenced|no reloc|"
    r"bare imm|one seam|at one seam|the pun|overlay|faithful|PROVEN|proven|"
    r"@identity-TODO", re.I)

CAST = re.compile(r"reinterpret_cast\s*<")

# `<ret> NAME(args) [const] { return NAME(...); }` - a seam-sweep candidate.
_SELF = re.compile(
    r"\b(?P<name>[A-Za-z_]\w*)\s*\((?P<params>[^;{)]*)\)\s*"
    r"(?:const\s*)?\{\s*return\s+(?P=name)\s*\((?P<args>[^;]*)\)\s*;",
    re.S,
)


def _arity(items: str) -> int:
    """Count top-level comma-separated items in the matched short form."""
    if not items.strip():
        return 0
    depth = 0
    count = 1
    for char in items:
        if char in "([{":
            depth += 1
        elif char in ")]}":
            depth = max(0, depth - 1)
        elif char == "," and depth == 0:
            count += 1
    return count


def scan_ledger():
    """(forced Counter, {relpath: [(line, text)]} OPEN worklist)."""
    forced = collections.Counter()
    openv = collections.defaultdict(list)
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            lines = path.read_text(errors="replace").split("\n")
            for i, line in enumerate(lines):
                # a cast written INSIDE a comment is prose, not a site
                for _ in CAST.finditer(line.split("//", 1)[0]):
                    ctx = " ".join(lines[max(0, i - 3):i + 2])
                    label = None
                    for name, pat in FORCED:
                        if re.search(pat, line) or re.search(pat, ctx):
                            label = name
                            break
                    if label is None and REASON.search(ctx):
                        label = "explained-seam"
                    if label:
                        forced[label] += 1
                    else:
                        openv[str(path.relative_to(REPO))].append(
                            (i + 1, line.strip()[:96]))
    return forced, openv


def self_recursion() -> list[str]:
    """Accessors whose only statement calls themselves - always the bug."""
    out = []
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            text = path.read_text(errors="replace")
            for m in _SELF.finditer(text):
                name = m.group("name")
                if name in ("if", "for", "while", "switch", "return", "sizeof"):
                    continue
                if _arity(m.group("params")) != _arity(m.group("args")):
                    continue
                line = text.count("\n", 0, m.start()) + 1
                body = " ".join(text[m.start():m.end() + 60].split())
                out.append(f"{path.relative_to(REPO)}:{line}: '{name}' returns "
                           f"a call to itself - a seam sweep rewrote the "
                           f"seam's own body\n    {body[:110]}")
    return out


# --------------------------------------------------------------------------- #
# nested static_casts (libclang; the board's full-tier metric)                #
# --------------------------------------------------------------------------- #
CDB = BUILD / "clangd/compile_commands.json"
SOURCE_SUFFIXES = {".cpp", ".cc", ".cxx", ".h", ".hpp", ".inl"}
STATIC_CAST = re.compile(r"\bstatic_cast\s*<")
STRICT_MARKER = "GZ_STRICT_ENUMS"


def _cast_argument_open(text: str, start: int) -> int:
    target = text.find("<", start)
    if target < 0:
        return -1
    depth = 0
    for i in range(target, len(text)):
        if text[i] == "<":
            depth += 1
        elif text[i] == ">":
            depth -= 1
            if depth == 0:
                return text.find("(", i + 1)
    return -1


def _has_nested_spelling(text: str) -> bool:
    for start in (m.start() for m in STATIC_CAST.finditer(text)):
        open_paren = _cast_argument_open(text, start)
        if open_paren < 0:
            continue
        depth = 0
        for i in range(open_paren, len(text)):
            if text[i] == "(":
                depth += 1
            elif text[i] == ")":
                depth -= 1
                if depth == 0:
                    if STATIC_CAST.search(text, open_paren + 1, i):
                        return True
                    break
    return False


def _flags(entry: dict, *, header: bool) -> list[str]:
    args = list(entry.get("arguments") or entry["command"].split())
    src = entry["file"]
    out = ["--driver-mode=cl"]
    for arg in args[1:]:
        if (arg == "/c" or arg == src or arg.endswith(src)
                or arg == "-fdelayed-template-parsing"):
            continue
        out.append(arg)
    # clang-cl otherwise omits uninstantiated template bodies from the AST
    out.append("-fno-delayed-template-parsing")
    if header:
        out.append("/TP")
    return out


def _strict_flags(flags: list[str]) -> list[str]:
    return [a for a in flags
            if not a.startswith("-std=") and not a.lower().startswith("/std:")
            ] + ["/std:c++20", "/Zc:__cplusplus"]


def nested_static_casts() -> list[tuple[str, int, int, str, str, str]]:
    """(file, line, col, outer, inner, source) per AST-proven nested pair."""
    import json
    from pathlib import Path

    import clang.cindex as cidx
    transparent = {cidx.CursorKind.UNEXPOSED_EXPR, cidx.CursorKind.PAREN_EXPR}

    def _operand(node):
        children = list(node.get_children())
        if not children:
            return None
        probe = children[-1]
        for _ in range(12):
            if probe.kind not in transparent:
                return probe
            children = list(probe.get_children())
            if not children:
                return probe
            probe = children[-1]
        return probe

    if not CDB.is_file():
        raise FileNotFoundError(f"{CDB}: no compile database")
    entries = json.loads(CDB.read_text())
    if not entries:
        raise RuntimeError(f"{CDB}: empty compile database")
    by_source = {}
    for entry in entries:
        source = Path(entry["file"])
        source = source if source.is_absolute() else REPO / source
        by_source[source.resolve()] = entry

    candidates = []
    for root in (REPO / "src", REPO / "include"):
        if not root.is_dir():
            continue
        for path in root.rglob("*"):
            if path.suffix in SOURCE_SUFFIXES and path.is_file():
                try:
                    if _has_nested_spelling(path.read_text(errors="ignore")):
                        candidates.append(path.resolve())
                except OSError:
                    continue

    index = cidx.Index.create()
    hits = set()
    failed = []
    fallback = entries[0]
    candidate_names = {str(p.relative_to(REPO)) for p in candidates}

    def _rel(cursor):
        file = cursor.location.file
        if file is None:
            return None
        try:
            return str(Path(file.name).resolve().relative_to(REPO))
        except ValueError:
            return None

    for path in candidates:
        entry = by_source.get(path, fallback)
        relpath = str(path.relative_to(REPO))
        flags = _flags(entry, header=path.suffix in {".h", ".hpp", ".inl"})
        modes = [("retail", flags)]
        if STRICT_MARKER in path.read_text(errors="ignore"):
            modes.append(("strict", _strict_flags(flags)))
        for mode, mode_flags in modes:
            try:
                tu = index.parse(str(path), args=mode_flags)
            except cidx.TranslationUnitLoadError:
                failed.append(f"{relpath} ({mode})")
                continue
            stack = [tu.cursor]
            while stack:
                node = stack.pop()
                stack.extend(node.get_children())
                if node.kind != cidx.CursorKind.CXX_STATIC_CAST_EXPR:
                    continue
                inner = _operand(node)
                if inner is None \
                        or inner.kind != cidx.CursorKind.CXX_STATIC_CAST_EXPR:
                    continue
                source = _operand(inner)
                if source is None:
                    continue
                r = _rel(node)
                if r not in candidate_names:
                    continue
                hits.add((r, node.location.line, node.location.column,
                          node.type.spelling, inner.type.spelling,
                          source.type.spelling))
    if failed:
        raise RuntimeError("libclang could not parse nested-cast candidate(s): "
                           + ", ".join(sorted(failed)))
    return sorted(hits)


# --------------------------------------------------------------------------- #
def gate_findings() -> list[str]:
    """The fast-tier findings: self-recursion (always FATAL) + OPEN casts
    above the committed unexplained floor."""
    out = list(self_recursion())
    from homm1.verify.board import load_baseline
    _forced, openv = scan_ledger()
    n_open = sum(len(v) for v in openv.values())
    floor = load_baseline().get("unexplained casts")
    if floor is None:
        # FAIL-CLOSED: no floor means the ratchet cannot fail, so an absent
        # baseline file would silently retire this gate.
        out.append("cast ledger: no committed floor for 'unexplained casts' - "
                   "restore config/cleanliness/cleanliness-text-baseline.tsv or "
                   "bless with `homm1 verify board --update`")
    elif n_open > floor:
        files = ", ".join(sorted(openv)[:4])
        out.append(f"cast ledger: OPEN {n_open} exceeds the committed floor "
                   f"{floor} (new unexplained cast(s); first files: {files})")
    return out


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 verify casts", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--summary", action="store_true",
                    help="counts only - skip the per-file OPEN worklist")
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on self-recursion or OPEN above the committed floor")
    ap.add_argument("--nested", action="store_true",
                    help="run the libclang nested-static_cast scan")
    ap.add_argument("--max", type=int, default=None,
                    help="exit 1 when OPEN exceeds this number (an explicit ratchet)")
    a = ap.parse_args(argv)

    if a.nested:
        hits = nested_static_casts()
        for path, line, col, outer, inner, source in hits:
            print(f"{path}:{line}:{col}: nested static_cast "
                  f"({source} -> {inner} -> {outer})")
        print(f"nested static_casts: {len(hits)}")
        return 0

    forced, openv = scan_ledger()
    n_open = sum(len(v) for v in openv.values())
    n_forced = sum(forced.values())
    print(f"cast ledger: {n_forced + n_open} casts total  |  "
          f"{n_open} unexplained (the debt driven to 0)  |  "
          f"{n_forced} reviewed ABI/model seams")
    for name, n in forced.most_common():
        print(f"   {n:6d}  {name}")
    if not a.summary and openv:
        # EVERY file: this listing is the campaign worklist; a cap hides work.
        print("\nOPEN by file (each needs a model fix or a reason):")
        listed = 0
        for f, rows in sorted(openv.items(), key=lambda kv: (-len(kv[1]), kv[0])):
            print(f"   {len(rows):4d}  {f}")
            listed += len(rows)
        assert listed == n_open, f"by-file listing ({listed}) != OPEN ({n_open})"

    rec = self_recursion()
    for v in rec:
        print(v)
    if rec:
        print(f"self-recursion: {len(rec)} accessor(s) call themselves",
              file=sys.stderr)
    else:
        print("self-recursion: OK - no accessor returns a call to itself")

    if a.max is not None and n_open > a.max:
        print(f"cast-ledger: OPEN {n_open} exceeds the {a.max} ratchet")
        return 1
    if a.gate and (rec or gate_findings()):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

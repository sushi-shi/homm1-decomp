"""homm1.verify.enum_domains - the enum-domain layer's structural gate (fast).

Ported invariants (docs/enum-modeling-plan.md, docs/patterns/enum-domains.md):
  1. SPLIT-WIDTH AGREEMENT (fatal): every H1_ENUM_STORAGE(N, S) matches the
     domain's declared narrow storage.
  2. STORAGE NAMES A REAL DOMAIN (fatal).
  3. NO BARE `enum X { ... }` IN A HEADER (fatal; single-enumerator tag types
     exempt).
  4. EXPLICIT ENUMERATOR VALUES (warning only).
  5. RANGE TESTS NAME A BOUNDARY, NOT A MEMBER (fatal; the fix is a RENAME at
     the compared value - the compare FORM is load-bearing).
  6. Enumerators are SCREAMING_SNAKE (fatal).

    python3 -m homm1.verify.enum_domains [--gate] [-v]
"""

from __future__ import annotations

import argparse
import re

from homm1.core.paths import REPO
from homm1.verify.srcscan import blank_comments, source_files

DECL = re.compile(r"\bH1_ENUM_BEGIN\(\s*(\w+)\s*\)")
DECL_SPLIT = re.compile(r"\bH1_ENUM_BEGIN_SPLIT\(\s*(\w+)\s*,\s*(\w+)\s*\)")
DECL_FLAGS = re.compile(r"\bH1_ENUM_FLAGS_BEGIN\(\s*(\w+)\s*,\s*(\w+)\s*\)")
DECL_CONST = re.compile(r"\bH1_ENUM_CONST_BEGIN\(\s*(\w+)\s*\)")
FORWARD = re.compile(r"\bH1_ENUM_FORWARD(?:_SPLIT)?\(\s*(\w+)\s*(?:,\s*(\w+)\s*)?\)")
STORAGE = re.compile(r"\bH1_ENUM_(?:STORAGE|STORAGE_STEPPED|PARAM|RETURN|BITFIELD)"
                     r"\(\s*(\w+)\s*,\s*(\w+)\s*\)")
BARE_ENUM = re.compile(
    r"^[ \t]*(?:typedef[ \t]+)?enum[ \t]+(\w+)[ \t]*\{(?P<body>[^}]*)\}", re.M)
# `<<`/`>>` halves must not read as range tests (645 shifts matched before)
RANGE_TEST = re.compile(r"(?<![<>])([<>]=?)(?![<>])[ \t]*([A-Z][A-Z0-9_]{2,})\b")
MARKER_OK = re.compile(
    r"(_FIRST|_LAST|_BEGIN|_END|_COUNT|_NONE|_INVALID|_UNSET|_COLS|_ROWS|_PX"
    r"|_FULL|_EMPTY|_HALF|_MAX|_MIN|_MS)$")
ENUMERATOR = re.compile(r"^[ \t]*([A-Za-z_]\w*)[ \t]*(=)?", re.M)


def is_tag_type(body: str) -> bool:
    names = [x.strip() for x in body.split(",") if x.strip()]
    return len(names) == 1 and "=" not in body


def domain_blocks(text: str):
    pat = re.compile(
        r"\bH1_ENUM_(BEGIN|BEGIN_SPLIT|CONST_BEGIN|FLAGS_BEGIN)"
        r"\(\s*(\w+)\s*(?:,\s*(\w+)\s*)?\)(?P<body>.*?)"
        r"\bH1_ENUM_(?:END|END_SPLIT|CONST_END|FLAGS_END)\(", re.S)
    for m in pat.finditer(text):
        yield m.group(2), m.group(1), m.group(3), m.group("body")


def audit():
    fatal: list[str] = []
    warn: list[str] = []
    declared: dict[str, str | None] = {}
    decl_site: dict[str, str] = {}

    files = list(source_files())
    texts = {}
    for f in files:
        texts[f] = blank_comments(f.read_text(errors="replace"))

    for f in files:
        r = str(f.relative_to(REPO))
        t = texts[f]
        for m in DECL.finditer(t):
            declared.setdefault(m.group(1), None)
            decl_site.setdefault(m.group(1), r)
        for m in DECL_CONST.finditer(t):
            declared.setdefault(m.group(1), None)
            decl_site.setdefault(m.group(1), r)
        for rx in (DECL_SPLIT, DECL_FLAGS):
            for m in rx.finditer(t):
                declared[m.group(1)] = m.group(2)
                decl_site.setdefault(m.group(1), r)
        for m in FORWARD.finditer(t):
            declared.setdefault(m.group(1), m.group(2))

    for f in files:
        r = str(f.relative_to(REPO))
        t = texts[f]
        for m in STORAGE.finditer(t):
            dom, st = m.group(1), m.group(2)
            line = t[:m.start()].count("\n") + 1
            if dom not in declared:
                fatal.append(f"{r}:{line}: H1_ENUM_STORAGE names undeclared "
                             f"domain '{dom}' - typo, or the domain header is "
                             f"missing")
                continue
            want = declared[dom]
            if want is not None and st != want:
                fatal.append(f"{r}:{line}: '{dom}' is declared with narrow "
                             f"storage '{want}' ({decl_site.get(dom, '?')}) "
                             f"but stored as '{st}' here - two beliefs about "
                             f"retail's field width")

    for f in files:
        if f.suffix != ".h":
            continue
        r = str(f.relative_to(REPO))
        t = texts[f]
        for m in BARE_ENUM.finditer(t):
            if is_tag_type(m.group("body")):
                continue
            line = t[:m.start()].count("\n") + 1
            fatal.append(f"{r}:{line}: bare `enum {m.group(1)}` in a header - "
                         f"declare it with H1_ENUM_BEGIN/END so the strict "
                         f"build sees a real domain")

    enumerators: dict[str, str] = {}
    for f in files:
        r = str(f.relative_to(REPO))
        t = texts[f]
        for name, _kind, _st, body in domain_blocks(t):
            for em in ENUMERATOR.finditer(body):
                if em.group(2) is None and em.group(1).isupper():
                    warn.append(f"{r}: {name}::{em.group(1)} has no explicit "
                                f"value")
        for dom, kind, _st, body in domain_blocks(t):
            if kind == "CONST_BEGIN":
                continue
            for m in ENUMERATOR.finditer(body):
                enumerators.setdefault(m.group(1), dom)
        for dom, _kind, _st, body in domain_blocks(t):
            for em in re.finditer(r"^[ \t]*([A-Za-z_]\w*)[ \t]*=", body, re.M):
                name = em.group(1)
                if not re.fullmatch(r"[A-Z][A-Z0-9_]*", name):
                    fatal.append(f"{r}: {dom}::{name} is not SCREAMING_SNAKE")

    for f in files:
        if f.suffix != ".cpp":
            continue
        r = str(f.relative_to(REPO))
        t = texts[f]
        for m in RANGE_TEST.finditer(t):
            name = m.group(2)
            dom = enumerators.get(name)
            if dom is None or MARKER_OK.search(name):
                continue
            line = t[:m.start()].count("\n") + 1
            fatal.append(f"{r}:{line}: range test `{m.group(1)} {name}` names "
                         f"a MEMBER of {dom} - declare a _FIRST/_LAST "
                         f"(inclusive) or _BEGIN/_END (half-open) marker AT "
                         f"THAT VALUE and compare against it (rename only; "
                         f"changing the operator moves bytes)")

    return fatal, warn, declared


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="homm1 verify enum-domains",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on any FATAL invariant")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="also print the warnings and the resolved domain table")
    a = ap.parse_args(argv)
    fatal, warn, declared = audit()
    for f in fatal:
        print(f"   {f}")
    if a.verbose:
        for w in warn[:40]:
            print(f"   (warn) {w}")
        if len(warn) > 40:
            print(f"   (warn) ... and {len(warn) - 40} more")
    if fatal:
        print(f"[enum-domains] FATAL: {len(fatal)} defect(s); {len(warn)} "
              f"implicit-value warning(s)")
        return 1 if a.gate else 0
    print(f"[enum-domains] OK - {len(declared)} domain(s), split widths agree, "
          f"no bare header enums ({len(warn)} implicit-value warning(s))")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

"""homm1.walls.priors - every prior verdict on a row, before any A/B.

A wall row can already carry a written verdict, and there are TWO independent
stores that hold one.  Screening only the source comment reads half the record:
7 of 17 rows on one lane's worklist carried a `codex_wall_reviews.tsv` review
that nobody saw, and one of them listed the exact A/B that lane was queuing.

  source     the `//` block above the function's `RVA(...)` pin - what the
             matcher who parked it wrote, plus the `@early-stop` marker itself.
             Re-derive the residue; the prose can be stale.  ONE blank line may
             separate the block from the pin (measured: 14 pins in the tree are
             spelled that way and every one of them is a real verdict, four of
             them 30+ lines); a formatter directive alone is not a verdict.
  review     config/codex_wall_reviews.tsv, keyed by rva AND source hash, so a
             row is reported `current` only when the body has not changed since
             the review was written; `STALE` means the verdict predates an edit.

    homm1 walls priors <rva|mangled|CClass::Member>...
    homm1 walls priors --todo N          the head of the campaign queue
    homm1 walls priors --stdin           tokens from a pipe, one worklist
"""

from __future__ import annotations

from homm1.core.usage import logged

import argparse
import re
import sys

from homm1.core.paths import REPO

ROOTS = ("src", "include")
PIN = re.compile(r"\b(?:VA|RVA)(?:_COMPGEN|_DYNINIT)?\s*\(\s*(0x[0-9a-fA-F]+)")
# `RVA_COMPGEN(rva, size, MANGLED)` pins a COMDAT at the emitting unit; the BODY,
# and therefore any verdict written beside it, is at the definition - for
# `??0CPlay@@QAE@XZ` that is `inline CPlay::CPlay()` in Play.h, a whole
# `@early-stop` block the pin site cannot see.  384 such pins exist tree-wide.
COMPGEN = re.compile(r"\bRVA_(?:COMPGEN|DYNINIT)\s*\(\s*0x[0-9a-fA-F]+\s*,"
                     r"\s*[^,)]+,\s*([^)\s]+)\s*\)")
COMMENT = re.compile(r"^\s*(?://|/\*|\*)")
# A machine directive addressed to a formatter or linter carries no verdict, and
# a block made only of them must not read as one.
DIRECTIVE = re.compile(r"^\s*(?://|/\*)\s*(?:clang-format|NOLINT)")
BLANK_GAP = 1                          # blank lines tolerated between block and pin


def _pin_sites() -> dict[int, list[tuple[str, int]]]:
    """{rva: [(relative path, 1-based line of the pin)]} over the whole tree."""
    sites: dict[int, list[tuple[str, int]]] = {}
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            rel = str(path.relative_to(REPO))
            for i, line in enumerate(path.read_text(errors="replace").split("\n")):
                m = PIN.search(line)
                if m:
                    address = int(m.group(1), 16)
                    rva = address - 0x400000 if m.group(0).startswith("VA") else address
                    sites.setdefault(rva, []).append((rel, i + 1))
    return sites


def block_above(lines: list[str], line: int) -> list[str]:
    """The verdict comment block above a 1-based pin line, in source order.

    A single blank line between the block and the pin is ordinary breathing
    room, not a separator: reading only the CONTIGUOUS block hid 13 real
    verdicts, among them the 33-line `@early-stop` on CHomm1Mgr::HandleCommand,
    which is how that row reached a worklist labelled unadjudicated.  Two blank
    lines are a section break and are still respected.
    """
    i = line - 2                       # 0-based index of the line above the pin
    for _ in range(BLANK_GAP):
        if i >= 0 and not lines[i].strip():
            i -= 1
        else:
            break
    out: list[str] = []
    while i >= 0 and COMMENT.match(lines[i]):
        out.append(lines[i].strip())
        i -= 1
    out = [x for x in out if not DIRECTIVE.match(x)]
    return list(reversed(out))


def _comment_above(rel: str, line: int) -> list[str]:
    return block_above((REPO / rel).read_text(errors="replace").split("\n"), line)


CTOR = re.compile(r"^\?\?0(\w+)@@")
DTOR = re.compile(r"^\?\?1(\w+)@@")
METH = re.compile(r"^\?(\w+)@(\w+)@@")


def source_name(mangled: str) -> str | None:
    """`Class::Member` for the mangled forms that HAVE a source spelling.

    Deliberately narrow.  A compiler-generated thunk (`??_G` scalar deleting
    destructor, `??_E`, `??_L` vector ctor iterator) is not written by anyone,
    so there is no definition to find and inventing a name would manufacture a
    verdict out of whatever comment happens to sit near a same-named symbol.
    """
    m = CTOR.match(mangled)
    if m:
        return f"{m.group(1)}::{m.group(1)}"
    m = DTOR.match(mangled)
    if m:
        return f"{m.group(1)}::~{m.group(1)}"
    m = METH.match(mangled)
    if m:
        return f"{m.group(2)}::{m.group(1)}"
    return None


def is_definition(line: str, qualified: str) -> bool:
    """Is this line the DEFINITION of `qualified`, rather than a call to it?

    A qualified call inside another body (`CPlay::ReleaseResources();` in
    `~CPlay`) would otherwise hand back the comment above THAT function.  A
    definition opens a parameter list and does not terminate the statement.
    """
    if qualified + "(" not in line:
        return False
    return ";" not in line and "return" not in line


def definition_block(qualified: str) -> tuple[str, int, list[str]] | None:
    """(path, 1-based line, comment block) above `qualified`'s definition."""
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            lines = path.read_text(errors="replace").split("\n")
            for i, line in enumerate(lines):
                if is_definition(line, qualified):
                    block = block_above(lines, i + 1)
                    if block:
                        return str(path.relative_to(REPO)), i + 1, block
    return None


def _targets(a) -> list[str]:
    toks = list(a.target)
    if a.stdin:
        toks += [w for w in sys.stdin.read().split() if w]
    if a.todo:
        from homm1.walls.inventory import build
        toks += [r["rva"] for r in build(a.unit, todo=True)[:a.todo] if r["rva"]]
    return toks


def report(token: str, sites: dict, reviews: dict, fresh: set[int]) -> bool:
    """Print both stores for one row; True when either carried a verdict."""
    from homm1.walls.diagnose import _locate
    b, why = _locate(token)
    if b is None:
        print(f"\n{token}\n  [priors] {why}")
        return False
    from homm1.walls.inventory import baseline_rows, report_scores
    bank = baseline_rows().get(b.rva)
    _p, scores = report_scores()
    cur = scores.get((b.unit, b.name))
    head = f"0x{b.rva:06x}  {b.unit}/{b.name}"
    if cur is not None:
        head += f"   cur {cur:.2f}"
    if bank:
        head += f"  best {bank[0]:.2f}  hist {bank[1]:.2f}"
    print(f"\n{head}")

    found = False
    for rel, line in sites.get(b.rva, ()):
        block = _comment_above(rel, line)
        if block:
            found = True
            print(f"  source   {rel}:{line}")
            for text in block:
                print(f"      {text}")
        else:
            owner = COMPGEN.search((REPO / rel).read_text(
                errors="replace").split("\n")[line - 1])
            hit = definition_block(source_name(owner.group(1)) or "") \
                if owner else None
            if hit:
                found = True
                drel, dline, dblock = hit
                print(f"  source   {rel}:{line} is a COMDAT pin; the body and "
                      f"its verdict are at {drel}:{dline}")
                for text in dblock:
                    print(f"      {text}")
            elif owner:
                print(f"  source   {rel}:{line}  (COMDAT pin for "
                      f"{owner.group(1)}; no comment here or at its definition)")
            else:
                print(f"  source   {rel}:{line}  (no comment above the pin)")
    if b.rva not in sites:
        print("  source   no RVA() pin found in src/ or include/")

    row = reviews.get(b.rva)
    if row is None:
        print("  review   none")
    else:
        found = True
        state = "current" if b.rva in fresh else "STALE (body edited since)"
        print(f"  review   {row['status']}/{row['wall_class']}  [{state}]")
        print(f"      {row['evidence']}")
    return found


@logged
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        prog="homm1 walls priors", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("target", nargs="*", help="hex rva, mangled name, or CClass::Member")
    ap.add_argument("--todo", type=int, metavar="N",
                    help="also screen the first N rows of the campaign queue")
    ap.add_argument("--unit", help="restrict --todo to one unit")
    ap.add_argument("--stdin", action="store_true", help="read tokens from a pipe")
    a = ap.parse_args(argv)
    from homm1.walls import check_unit
    check_unit(a.unit)
    toks = _targets(a)
    if not toks:
        ap.error("give a target, --stdin, or --todo N")

    from homm1.walls.reviews import current as current_reviews, load
    sites, reviews = _pin_sites(), load()
    fresh = set(current_reviews())
    hits = sum(report(t, sites, reviews, fresh) for t in toks)
    print(f"\n[priors] {hits}/{len(toks)} row(s) already carry a written verdict")
    return 0


if __name__ == "__main__":
    sys.exit(main())

"""homm1.walls.stale_markers - `@early-stop` markers on 100% functions.

`// @early-stop` means "a complete reconstruction parked below 100%", and
grepping it is the final-sweep worklist - which only works while the markers
are TRUE. They are not self-clearing: a fix elsewhere flips a parked
function and nothing removes its marker. A marker on a function already at
100.0 is a lie; a worklist nobody can trust is worse than none.

Method: each marker owns the next plain VA() below it (VA_COMPGEN only as
a fallback - it labels an adjacent compiler-generated body); the address
joins the Model for the mangled name, the name joins the compare report.

    homm1 walls stale-markers [--summary] [--max N]
"""

from __future__ import annotations

from homm1.core.usage import logged

import re

from homm1.core.paths import REPO

ROOTS = ("src", "include")
MARKER = re.compile(r"^\s*//\s*@early-stop\b")
VA = re.compile(r"\bVA\s*\(\s*(0x[0-9a-fA-F]+)")
VA_COMPGEN = re.compile(r"\bVA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)")
QUALIFIED_DEF = re.compile(
    r"\b((?:[A-Za-z_]\w*::)+~?[A-Za-z_]\w*)\s*\("
)


def marker_sites():
    """Yield ``(relative_path, line, rva, qualified_name)`` for each marker.

    ``rva`` is ``None`` when the source marker cannot be joined to a following
    function annotation.  ``qualified_name`` is the next source definition's
    ``Class::Member`` spelling and covers header-inline bodies which correctly
    have no RVA annotation at their definition.  Keeping this scan in one place
    lets the stale-marker audit and actionable inventory share one ownership rule.
    """
    for root in ROOTS:
        for path in sorted((REPO / root).rglob("*")):
            if path.suffix not in (".cpp", ".h"):
                continue
            lines = path.read_text(errors="replace").split("\n")
            for i, line in enumerate(lines):
                if not MARKER.match(line):
                    continue
                addr = None
                qualified = None
                for j in range(i + 1, min(i + 40, len(lines))):
                    if qualified is None and (q := QUALIFIED_DEF.search(lines[j])):
                        qualified = q.group(1)
                    m = VA.search(lines[j])
                    if m:
                        addr = int(m.group(1), 16) - 0x400000
                        break
                if addr is None:
                    for j in range(i + 1, min(i + 4, len(lines))):
                        m = VA_COMPGEN.search(lines[j])
                        if m:
                            addr = int(m.group(1), 16) - 0x400000
                            break
                yield str(path.relative_to(REPO)), i + 1, addr, qualified


def marker_rvas() -> set[int]:
    """The mapped function RVAs currently parked by ``@early-stop``."""
    return {
        addr for _rel, _line, addr, _qualified in marker_sites()
        if addr is not None
    }


def marker_names() -> set[str]:
    """Source-qualified names for parked functions, including header inlines."""
    return {
        qualified for _rel, _line, _addr, qualified in marker_sites()
        if qualified is not None
    }


def scan():
    from homm1.model import resolve
    from homm1.walls.inventory import report_scores
    syms = {b.rva: b.name for b in resolve().functions if b.name}
    _path, sc = report_scores()
    pct = {}
    for (_u, name), p in sc.items():
        pct.setdefault(name, p)
    stale, live, unknown = [], 0, []
    from homm1.sema.index import short_name
    by_short = {short_name(name): name for name in pct}
    for rel, line, addr, qualified in marker_sites():
        name = syms.get(addr) if addr is not None else None
        if name is None and qualified is not None:
            name = by_short.get(qualified)
        if name is None or name not in pct:
            unknown.append((rel, line, addr))
        elif pct[name] >= 100.0:
            stale.append((rel, line, name))
        else:
            live += 1
    return stale, live, unknown


@logged
def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 walls stale-markers",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--summary", action="store_true")
    ap.add_argument("--max", type=int, default=None)
    a = ap.parse_args(argv)
    stale, live, unknown = scan()
    print(f"early-stop markers: {len(stale) + live + len(unknown)} total  |  "
          f"{live} LIVE (a real park)  |  {len(stale)} STALE (already 100%)  "
          f"|  {len(unknown)} unmapped")
    if not a.summary and stale:
        print("\nSTALE - the function is at 100.0, so the marker is a lie:")
        for rel, ln, name in sorted(stale):
            print(f"   {rel}:{ln}  {name[:64]}")
    if not a.summary and unknown:
        print("\nUNMAPPED (reported, never auto-deleted):")
        for rel, ln, addr in sorted(unknown)[:20]:
            print(f"   {rel}:{ln}  "
                  f"{'0x%06x' % addr if addr is not None else '(no RVA below)'}")
        if len(unknown) > 20:
            print(f"   ... {len(unknown) - 20} more")
    if a.max is not None and len(stale) > a.max:
        print(f"stale-markers: STALE {len(stale)} exceeds the {a.max} ratchet")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

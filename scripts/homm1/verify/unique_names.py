"""homm1.verify.unique_names - Model-consistency FATALs (normal tier).

Three checks, all pure Model joins:

  * NAME INJECTIVITY (ported verify_unique_names): MSVC5 keeps ONE COMDAT
    copy per symbol name, so N retail bodies at N RVAs had N DISTINCT names.
    One mangled FUNCTION name claimed at two RVAs contradicts the binary (the
    model already enforces the data side). Extent overlap is structural now:
    census rows are disjoint and a claim size may never cross the next
    admitted start - which is the fourth check:
  * MODEL VIOLATIONS = 0: every resolver violation (kind mismatch, size
    crossing, claim off the census, phantom shadow, ...) is a FATAL here -
    the model records them, this gate fails on them.
  * NEVER VACUOUS: zero claimed functions means the extraction itself broke.

    python3 -m homm1.verify.unique_names
"""

from __future__ import annotations

import sys


def findings() -> tuple[list[str], int]:
    from homm1.model import resolve
    model = resolve()
    out = []
    # SCOPE: our reconstruction's claims only ('src'/'src_compgen'). The
    # linker theorem binds what CL EMITS UNDER A NAME; provider labels are
    # outside it (a FID label legitimately repeats - the fuzzy-matcher
    # family), and an RVA_DYNINIT owner pin names the owning DATUM, which
    # several per-TU `$E` funclets legitimately share.
    _SRC = {"src", "src_compgen"}
    by_name: dict[str, set[int]] = {}
    n_claims = 0
    for b in model.functions:
        if b.name and b.channel in _SRC:
            n_claims += 1
            by_name.setdefault(b.name, set()).add(b.rva)
        for a in b.aliases:
            if a.channel in _SRC and a.name:
                by_name.setdefault(a.name, set()).add(b.rva)
    for name, rvas in sorted(by_name.items()):
        if len(rvas) > 1:
            locs = "  ".join(f"0x{r:06x}" for r in sorted(rvas))
            out.append(f"name-injectivity: {name} claimed at >1 RVA: {locs} "
                       f"(one name = one retail RVA; a persistent duplicate "
                       f"is a mis-model)")
    for v in model.violations:
        out.append(f"model violation: {v}")
    if n_claims == 0:
        out.append("unique-names: 0 claimed functions - extraction broke; "
                   "refusing to pass vacuously")
    return out, n_claims


def main(argv=None) -> int:
    import argparse
    argparse.ArgumentParser(
        prog="homm1 verify unique-names", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter).parse_args(argv)
    bad, n = findings()
    for b in bad:
        print(b, file=sys.stderr)
    if bad:
        print(f"unique-names: FATAL - {len(bad)} finding(s) over {n} claims",
              file=sys.stderr)
        return 1
    print(f"unique-names: OK - {n} fn claims, every mangled name at exactly "
          f"one RVA, 0 model violations")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

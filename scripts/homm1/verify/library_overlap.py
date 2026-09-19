"""homm1.verify.library_overlap - no src claim on a static-libs row (normal).

HOMM1.EXE statically links MFC + CRT; library code is never hand-
reconstructed - it gets a functions_static_libs.tsv / data_static_libs.tsv
row and game code calls it through the real headers. A retail RVA is
therefore a src RECONSTRUCTION xor a library CARVE-OUT (the vendored zlib
channel is the deliberate coexistence and lives in its own tables).

The Model's precedence quietly ALIASES a static-libs label under a winning
src claim; this gate makes that loud (the merged verify_library_overlap
verdict): every binding whose winner is a src channel and whose alias set
carries a static-libs claim is a double-claim - prune the false table row,
or carve the hand-copied library body out of src/.

LOW-confidence static-lib rows are leads, not claims (model policy filters
them before resolution), so they cannot alias and are NOT findings here -
that narrowing vs the frozen gate is deliberate and recorded.

    python3 -m homm1.verify.library_overlap
"""

from __future__ import annotations

import sys

_LIB = {"functions_static_libs", "data_static_libs"}


def findings() -> tuple[list[str], int]:
    from homm1.model import resolve
    model = resolve()
    out = []
    n_src = 0
    for b in model.functions + model.data:
        if not b.channel.startswith("src"):
            continue
        n_src += 1
        for a in b.aliases:
            if a.channel in _LIB:
                out.append(f"0x{b.rva:06x}  src {b.name} [{b.unit}] "
                           f"({b.channel})  <->  {a.channel} {a.name} - "
                           f"double-claim on the same retail bytes")
    if n_src == 0:
        out.append("library-overlap: parsed 0 src claims - extraction broke; "
                   "refusing to pass vacuously")
    return out, n_src


def main(argv=None) -> int:
    import argparse
    argparse.ArgumentParser(
        prog="homm1 verify library-overlap", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter).parse_args(argv)
    bad, n = findings()
    for b in bad:
        print(b, file=sys.stderr)
    if bad:
        # n == 0 is the vacuity guard firing, NOT a double-claim: calling it
        # one sends the reader hunting a table row that does not exist.
        if n == 0:
            print("library-overlap: FATAL - the model produced 0 src claims, "
                  "so nothing was compared. Build first (`homm1 build`); a "
                  "built tree with 0 claims means extraction broke.",
                  file=sys.stderr)
            return 1
        print(f"library-overlap: FATAL - {len(bad)} double-claim(s). Each "
              f"retail RVA is a src reconstruction XOR a library carve-out: "
              f"prune the false table row, or carve the copied body and call "
              f"the real routine via <Mfc.h>.", file=sys.stderr)
        return 1
    print(f"library-overlap: OK - {n} src claims, 0 static-libs overlap")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

"""homm1.compare - score the reconstruction against retail, and report it.

The slice takes two directories of COFF objects, normalizes both sides into a
disposable content-addressed view, pairs them BY NAME in an objdiff project,
runs objdiff-cli, and prints what moved. It measures; it never decides.

INPUT SURFACE (exactly four things, deliberately narrow):

  1. `config/units.toml` via `homm1.manifest.units` - the unit census, and the
     `[build]` platform/compiler strings the project file carries.
  2. A BASE object directory (cl's outputs, e.g. `build/objdiff/base/`).
     Treated as FROZEN: never recompiled, never modified.
  3. A TARGET object directory (the delink slice's output, e.g.
     `build/objdiff/target-new/`). Which units HAVE a target is read off this
     directory, never predicted.
  4. For a reference diff only: a previous objdiff `report.json`.

Compare consumes NO retail labels, NO providers, NO extraction fragments, NO
Model / bindings.tsv, NO `config/retail/` tables, and it never opens the retail
image. The single sanctioned cross-slice import is `homm1.delink.eh_band`, used
only for its pure name-spelling helpers (`registration_symbol`,
`unwind_symbol`, `funcinfo_symbol`, `unwindmap_symbol`, `is_band_symbol`) so the
normalizer spells a carved EH band the same way the delinker did. Nothing in
that import path reads the executable.

NO POLICY LIVES HERE. A wrong pairing, a missing target object, a symbol that
appears or disappears is an upstream (model / delink / source) fact to REPORT.
Compare does not patch it over, and it does not gate: score movement is printed,
not turned into an exit code (`--reference` prints every regression and still
exits 0). Only an operational failure - a missing directory, a normalizer
invariant break, objdiff-cli failing - is nonzero.

Modules:
    canonicalize.py  one object -> disposable comparison copy (+ sidecar)
    normalize.py     the batch driver over a unit list (stale-skip, stamp)
    project.py       objdiff.json pairing normalized base <-> normalized target
    run.py           the verb: normalize -> project -> objdiff-cli -> summary
"""

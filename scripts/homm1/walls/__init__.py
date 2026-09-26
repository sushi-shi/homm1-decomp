"""homm1.walls - the wall-breaking slice: the remaining matching campaign.

    homm1 walls inventory        the derived worklist (report x Model x
                                  match_baseline) - ascending historical MAX
    homm1 walls abstractions     classify every sub-100 row by the semantic
                                  level to inspect before source-shape work
    homm1 walls diagnose <fn>    classify one wall from the normalized pair:
                                  referent -> inline/call-set -> cfg -> regalloc
    homm1 walls inline-model     --gap <rva> reports the call-set delta and
                                  base-object definition evidence. VC5 inline
                                  budget predictions are not calibrated for VC4.
    homm1 walls semdiff <fn>     OPERAND-LEVEL adjudication of one pair:
                                  exclusive fp/disp/store/imm keys, plus the
                                  ordered referent sequence a masked diff
                                  structurally cannot show
    homm1 walls semsweep <tsv>   the same screen over a worklist range
    homm1 walls aggregate-copies rep-movs count sieve; a source/CFG lead,
                                  never proof until block merging is excluded
    homm1 walls aggdecl          aggregate-vs-scalar declaration sieve per
                                  member pair (--control re-proves it)
    homm1 walls aggscan          by-value aggregate argument sieve, keyed on
                                  the callee over the whole image
    homm1 walls valuetemp        by-value struct temp sieve: an inlined
                                  accessor's dead half-store in the frame
    homm1 walls eh-frame         /GX frame-presence + unwind-state sieve,
                                  cause-tagged (inline/merge/state-flow/object)
    homm1 walls global-refs      global read-COUNT sieve (the cached-global
                                  bug class; --calibrate = detector-bug rate)
    homm1 walls stale-markers    @early-stop markers sitting on 100% bodies
    homm1 walls review           source-hash-scoped reviewer ledger
    homm1 walls recheck          re-measure the counts a review certifies
                                  against today's pair
    homm1 walls priors           both prior-verdict stores for a worklist -
                                  the comment above the RVA() pin AND the
                                  review ledger row - screened before any A/B

The worklist is derived from the compare report every time. The review
ledger records reviewer progress and invalidates each row when its source
hash changes; it is not evidence that a reconstruction is correct.

Input surface: the Model, the compare out-dir (report.json + normalized
objs), config/match_baseline.tsv, tool.objdump/tool.cl, delink.coffx (the
shared COFF topology reader). Read-only except inline-model's scratch
harness compiles under build/inline-model/.
"""

from __future__ import annotations

from homm1.core.usage import logged

_SUBS = {"inventory": "homm1.walls.inventory",
         "abstractions": "homm1.walls.abstractions",
         "diagnose": "homm1.walls.diagnose",
         "inline-model": "homm1.walls.inline_model",
         "aggregate-copies": "homm1.walls.aggregate_copies",
         "eh-frame": "homm1.walls.eh_frame",
         "aggscan": "homm1.walls.aggscan",
         "aggdecl": "homm1.walls.aggdecl",
         "valuetemp": "homm1.walls.valuetemp",
         "global-refs": "homm1.walls.global_refs",
         "semdiff": "homm1.walls.semdiff",
         "semsweep": "homm1.walls.semdiff",
         "stale-markers": "homm1.walls.stale_markers",
         "priors": "homm1.walls.priors",
         "recheck": "homm1.walls.recheck",
         "review": "homm1.walls.reviews"}


def check_unit(unit: str | None) -> str | None:
    """`--unit` filters answer 0/none for a name nobody has - which reads as a
    clean result rather than a typo. Reject an unknown unit here instead."""
    if unit is None:
        return None
    from homm1.manifest import units as manifest_units
    known = {u["unit"] for u in manifest_units()}
    if unit in known:
        return unit
    import difflib
    import sys
    near = difflib.get_close_matches(unit, sorted(known), n=3)
    print(f"[walls] unknown unit {unit!r} - not in config/units.toml"
          + (f" (did you mean: {', '.join(near)}?)" if near else "")
          + "\n        `homm1 sema map units` lists the units that claim rows",
          file=sys.stderr)
    raise SystemExit(2)


@logged
def main(argv=None) -> int:
    import importlib
    import sys
    argv = list(sys.argv[1:] if argv is None else argv)
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__.strip())
        return 0 if argv else 2
    if argv[0] not in _SUBS:
        print(f"homm1 walls: unknown verb {argv[0]!r} (have: "
              f"{', '.join(_SUBS)})", file=sys.stderr)
        return 2
    mod = importlib.import_module(_SUBS[argv[0]])
    sys.argv = [f"homm1 walls {argv[0]}", *argv[1:]]
    entry = mod.sweep_main if argv[0] == "semsweep" else mod.main
    return entry(argv[1:])

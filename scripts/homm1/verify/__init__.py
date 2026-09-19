"""homm1.verify - the VERIFY/BANK slice: MAX-ledger banking + the regression
gate + the README score block.

    python3 -m homm1.verify status        current summary + the regression
                                           report (rva-keyed), bankable
                                           improvements, renames, losses -
                                           exit 0 always (it reports)
    python3 -m homm1.verify check         same computation; exit nonzero on a
                                           REAL regression (a fresh below-bank
                                           dip, an unbanked loss, or a hard
                                           report failure) - the MAX gate
    python3 -m homm1.verify bank          preconditions (bankable tree), then
                                           update config/match_baseline.tsv
                                           under the src_hash rules and refresh
                                           the README score block. A MANUAL
                                           act: nothing regenerates the
                                           baseline automatically.
    python3 -m homm1.verify fingerprints  refresh the per-function source
                                           fingerprint cache (clangd range
                                           hashes; bank runs this itself)
    python3 -m homm1.verify <gate>        run one ported gate/audit module
                                           (see the list below); `check
                                           --tier fast|normal|full|link`
                                           runs them in tiers (default
                                           fast,normal - the graph's edge)

Ported doctrine (from the frozen homm1-old/match/status.py, never imported):
the retail RVA is the BODY's identity (a vanished row whose rva is still
occupied is a rename/move, not a loss; the high-water travels by rva);
best_pct is gated by src_hash (same hash + a different % banks the high mark -
TU composition moved, not the source; a CHANGED hash resets best to cur - the
old peak belonged to source that no longer exists); hist_pct never resets
except when the rva moves under a name.

Input surface: build/objdiff/compare-new/report.json (falling back to
build/objdiff/report.json), config/match_baseline.tsv, the Model
(homm1.model.resolve) for rva/unit joins, config/units.toml for the module
rollup, and git for the bankable-tree precondition only.

Writes: config/match_baseline.tsv and README.md's marked block from `bank`
ONLY, and each gate's committed floor from its own `--update` bless ONLY -
never from a gate run. Gates otherwise write nothing but build/gen/ scratch
(the fingerprint cache, the .LIB symbol cache, the layout and data-access
maps), which is derived and regenerated.
"""

from __future__ import annotations

_SUBS = ("status", "check", "bank", "fingerprints")

#: the ported gate/audit modules, runnable as `homm1 verify <name>`. MOST are
#: also a tier member of `check --tier` (homm1.verify.tiers); the ones in
#: _QUERY_ONLY below are read-only oracles no tier runs - they answer a
#: question, they do not return findings.
_GATES = {"board": "homm1.verify.board", "bans": "homm1.verify.bans",
          "casts": "homm1.verify.casts",
          "constants": "homm1.verify.constants",
          "enum-domains": "homm1.verify.enum_domains",
          "label-style": "homm1.verify.label_style",
          "include-order": "homm1.verify.include_order",
          "unique-names": "homm1.verify.unique_names",
          "library-overlap": "homm1.verify.library_overlap",
          "tu-order": "homm1.verify.tu_order",
          "data-tu-order": "homm1.verify.data_tu_order",
          "dead-code": "homm1.verify.dead_code",
          "undefined-closure": "homm1.verify.undefined_closure",
          "vtables": "homm1.verify.vtables",
          "vtable-scan": "homm1.verify.vtable_scan",
          "alloc-size": "homm1.verify.alloc_size",
          "assert-relocs": "homm1.verify.assert_relocs",
          "data-relocs": "homm1.verify.data_relocs",
          "caller-callee": "homm1.verify.caller_callee",
          "data-access": "homm1.verify.data_access",
          "data-coverage": "homm1.verify.data_coverage",
          "library-data-refs": "homm1.verify.library_data_refs",
          "layout": "homm1.verify.layout",
          "link-tier": "homm1.verify.link_tier"}

#: runnable as `homm1 verify <name>` but in NO tier: read-only oracles, not
#: gates. `vtable-scan` enumerates the image's vtables (verify.vtables is the
#: gate over it); `layout` is the field-offset oracle verify.data_access
#: consumes. Neither returns findings, so neither can fail a build.
_QUERY_ONLY = ("layout", "library-data-refs", "vtable-scan")

#: Audits that are deliberately explicit because they parse the whole source
#: tree and are not part of a normal build tier.
_STANDALONE = ("constants",)

#: tier label -> verb, where the two spellings differ. homm1.verify.tiers
#: labels the bans row `vtable-bans` (so do docs/tooling-map.md and every
#: printed tier line), while the module and the verb are `bans`; without this
#: the label names no runnable command.
_ALIASES = {"vtable-bans": "bans"}


def _usage(stream=None) -> None:
    import sys
    out = stream or sys.stdout
    print(__doc__.strip(), file=out)
    gates = sorted(g for g in _GATES
                   if g not in _QUERY_ONLY and g not in _STANDALONE)
    print("\ngates (each also run by `check --tier`): " + ", ".join(gates),
          file=out)
    print("standalone audits (no tier runs these): "
          + ", ".join(sorted(_STANDALONE)), file=out)
    print("read-only oracles (no tier runs these): "
          + ", ".join(sorted(_QUERY_ONLY)), file=out)


def main(argv=None) -> int:
    import sys
    argv = list(sys.argv[1:] if argv is None else argv)
    if argv and argv[0] in _ALIASES:
        argv[0] = _ALIASES[argv[0]]
    known = _SUBS + tuple(_GATES)
    if not argv:
        _usage(sys.stderr)
        print("\nhomm1 verify: pick a verb or a gate from the lists above",
              file=sys.stderr)
        return 2
    if argv[0] in ("-h", "--help"):
        _usage()
        return 0
    if argv[0] not in known:
        _usage(sys.stderr)
        print(f"\nhomm1 verify: unknown verb/gate {argv[0]!r} - pick one of "
              f"the names listed above", file=sys.stderr)
        return 2
    sub, rest = argv[0], argv[1:]
    if sub == "fingerprints":
        from homm1.verify.fingerprints import main as fp_main
        return fp_main(rest)
    if sub in _GATES:
        import importlib
        mod = importlib.import_module(_GATES[sub])
        sys.argv = [f"homm1 verify {sub}", *rest]
        return mod.main(rest)
    from homm1.verify import verbs
    sys.argv = [f"homm1 verify {sub}", *rest]
    return {"status": verbs.cmd_status, "check": verbs.cmd_check,
            "bank": verbs.cmd_bank}[sub](rest)

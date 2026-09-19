"""homm1.verify.tiers - the tier registry `homm1 verify check --tier` runs.

  fast    source cleanliness (board, bans, casts, enum domains, label style,
          include order) - seconds, no build artifacts.
  normal  code/model joins (unique names, library overlap, TU order, dead
          code, undefined closure) - needs bindings + base/target objs.
  full    slower code-evidence audits (assert relocs and caller/callee).
  data    data/vtable audits, deliberately opt-in until the data campaign.
  link    candidate-EXE audits (sections, image diff, link defects) - needs
          `homm1 link`'s candidate image.

Default = fast+normal (what the graph's check edge runs); full/link opt in.
Every gate returns FINDINGS (a list of strings); a non-empty list fails the
gate, and the runner never writes anything - blessing a baseline is each
module's own manual verb.
"""

from __future__ import annotations

import time


def _board():
    from homm1.verify import board
    return board.gate()


def _bans():
    from homm1.verify import bans
    return [f"[{label}] {p}:{ln}: {tok}"
            for label, p, ln, tok in bans.scan()]


def _casts():
    from homm1.verify import casts
    return casts.gate_findings()


def _enum_domains():
    from homm1.verify import enum_domains
    fatal, _warn, _decl = enum_domains.audit()
    return fatal


def _label_style():
    from homm1.verify import label_style
    return label_style.violations()


def _include_order():
    from homm1.verify import include_order
    dupes, preludes, unordered, _manual, _changed = include_order.audit()
    out = []
    for rel, d in sorted(dupes.items()):
        out.append(f"duplicate include(s) in {rel}: {', '.join(d)}")
    for rel, w in sorted(preludes.items()):
        out.append(f"header missing prelude {rel}: {', '.join(w)}")
    for rel in unordered:
        out.append(f"include block out of canonical order: {rel}")
    return out


def _unique_names():
    from homm1.verify import unique_names
    bad, _n = unique_names.findings()
    return bad


def _library_overlap():
    from homm1.verify import library_overlap
    bad, _n = library_overlap.findings()
    return bad


def _tu_order():
    from homm1.verify import tu_order
    findings, _summary = tu_order.gate_findings()
    return findings


def _data_tu_order():
    from homm1.verify import data_tu_order
    return data_tu_order.gate_findings()


def _undefined_closure():
    from homm1.verify import undefined_closure
    return undefined_closure.gate_findings()


def _dead_code():
    from homm1.verify import dead_code
    return dead_code.gate_findings()


def _vtables():
    from homm1.verify import vtables
    return vtables.gate_findings()


def _alloc_size():
    from homm1.verify import alloc_size
    return alloc_size.gate_findings()


def _assert_relocs():
    from homm1.verify import assert_relocs
    return assert_relocs.gate_findings()


def _data_relocs():
    from homm1.verify import data_relocs
    return data_relocs.gate_findings()


def _caller_callee():
    from homm1.verify import caller_callee
    return caller_callee.gate_findings()


def _data_access():
    from homm1.verify import data_access
    return data_access.gate_findings()


def _data_coverage():
    from homm1.verify import data_coverage
    return data_coverage.gate_findings()


def _link_tier():
    from homm1.verify import link_tier
    return link_tier.gate_findings()


TIERS: dict[str, list[tuple[str, object]]] = {
    "fast": [
        ("board", _board),
        ("vtable-bans", _bans),
        ("casts", _casts),
        ("enum-domains", _enum_domains),
        ("label-style", _label_style),
        ("include-order", _include_order),
    ],
    "normal": [
        ("unique-names", _unique_names),
        ("library-overlap", _library_overlap),
        ("tu-order", _tu_order),
        ("dead-code", _dead_code),
        ("undefined-closure", _undefined_closure),
    ],
    "full": [
        ("assert-relocs", _assert_relocs),
        ("caller-callee", _caller_callee),
    ],
    "data": [
        ("data-tu-order", _data_tu_order),
        ("vtables", _vtables),
        ("alloc-size", _alloc_size),
        ("data-relocs", _data_relocs),
        ("data-access", _data_access),
        ("data-coverage", _data_coverage),
    ],
    "link": [
        ("link-tier", _link_tier),
    ],
}

DEFAULT = ("fast", "normal")


def parse_tiers(spec: str | None):
    if not spec:
        return list(DEFAULT)
    if spec == "none":
        return []
    names = [t.strip() for t in spec.split(",") if t.strip()]
    for t in names:
        if t not in TIERS:
            raise SystemExit(f"unknown tier {t!r} (pick from "
                             f"{', '.join(TIERS)}, or 'none')")
    return names


def _rerun_command(name: str) -> str:
    """`homm1 verify <gate>` - the spelling that actually reaches the module.

    Never derive it from the tier label: the `vtable-bans` row runs
    homm1.verify.BANS, so `python3 -m homm1.verify.vtable_bans` (the old
    mechanical name.replace) names a module that does not exist.
    """
    from homm1.verify import _ALIASES, _GATES
    verb = _ALIASES.get(name, name)
    if verb in _GATES:
        return f"homm1 verify {verb}"
    for gate, module in _GATES.items():
        if module.rsplit(".", 1)[-1] == verb.replace("-", "_"):
            return f"homm1 verify {gate}"
    return f"homm1 verify check --tier {name}"


def run(tier_names, *, max_findings: int = 12) -> int:
    """Run every gate in the named tiers; print a verdict per gate. Returns
    the number of FAILING gates."""
    failed = 0
    for tier in tier_names:
        for name, fn in TIERS[tier]:
            if fn is None:
                print(f"[verify {tier}] {name}: DEFERRED (not ported - an "
                      f"honestly absent gate, never a fake green; see "
                      f"docs/homm1-old-triage.md)")
                continue
            t0 = time.monotonic()
            try:
                findings = fn()
            # SystemExit is a BaseException: a gate that reports a missing
            # input by raising it (`no report.json - run homm1 compare`)
            # would otherwise abort the whole tier run, and every gate after
            # it would be silently skipped with no verdict at all.
            except SystemExit as exc:  # noqa: BLE001
                findings = [f"gate could not run: {exc} "
                            f"(re-run `{_rerun_command(name)}`)"]
            except Exception as exc:  # noqa: BLE001 - a broken gate is a failure
                findings = [f"gate crashed: {type(exc).__name__}: {exc} - a "
                            f"broken gate is a FAILURE, never a pass; re-run "
                            f"`{_rerun_command(name)}` for the traceback. A "
                            f"message naming a build/gen artifact means the "
                            f"tree is mid-build or that file is truncated: "
                            f"rebuild with `homm1 build`."]
            dt = time.monotonic() - t0
            if findings:
                failed += 1
                print(f"[verify {tier}] {name}: FAIL "
                      f"({len(findings)} finding(s), {dt:.1f}s)")
                for f in findings[:max_findings]:
                    print(f"    {f.splitlines()[0][:200]}")
                if len(findings) > max_findings:
                    print(f"    ... {len(findings) - max_findings} more "
                          f"({_rerun_command(name)})")
            else:
                print(f"[verify {tier}] {name}: OK ({dt:.1f}s)")
    return failed

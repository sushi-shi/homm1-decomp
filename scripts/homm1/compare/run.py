"""homm1.compare.run - the compare verb, end to end.

    python3 -m homm1.compare.run [--base-dir build/objdiff/base]
                                  [--target-dir build/objdiff/target-new]
                                  [--out-dir build/objdiff/compare-new]
                                  [--reference build/objdiff/report.json]

    base objs + target objs -> <out-dir>/{base,target}/   (normalized copies)
                            -> <out-dir>/objdiff.json     (the pairing)
                            -> <out-dir>/report.json      (objdiff-cli)
                            -> a per-unit and overall summary on stdout

With `--reference <report.json>` the per-function scores are diffed against that
earlier report, keyed by unit + symbol name: equal / improved / regressed /
appeared / disappeared, and EVERY regressed row is printed.

This verb REPORTS. Score movement never changes the exit code - a regression is
a fact for a human (or a later gate slice) to adjudicate, and its cause is
upstream in model/delink/source, never something compare papers over. Only an
operational failure exits nonzero.
"""

from __future__ import annotations

import argparse
from pathlib import Path

from homm1.compare import project as project_mod
from homm1.compare.normalize import normalize, units_with_a_target
from homm1.core.paths import BUILD
from homm1.tool import ToolError, objdiff

BASE_DIR = BUILD / "objdiff/base"
TARGET_DIR = BUILD / "objdiff/target-new"
OUT_DIR = BUILD / "objdiff/compare-new"


def _pct(measures: dict, key: str = "fuzzy_match_percent") -> float:
    return float(measures.get(key) or 0.0)


def functions(report: dict) -> dict[tuple[str, str], float]:
    """{(unit, symbol): fuzzy%} over a report's units."""
    out: dict[tuple[str, str], float] = {}
    for unit in report.get("units", []):
        name = unit.get("name", "")
        for fn in unit.get("functions", []):
            out[(name, fn.get("name", ""))] = _pct(fn)
    return out


def diff_reports(reference: dict, current: dict) -> dict:
    """Per-function score movement, keyed by unit + symbol name."""
    old, new = functions(reference), functions(current)
    shared = old.keys() & new.keys()
    equal = [k for k in shared if new[k] == old[k]]
    improved = [k for k in shared if new[k] > old[k]]
    regressed = sorted(k for k in shared if new[k] < old[k])
    return {
        "equal": len(equal),
        "improved": sorted(improved, key=lambda k: new[k] - old[k]),
        "regressed": sorted(regressed, key=lambda k: new[k] - old[k]),
        "appeared": sorted(new.keys() - old.keys()),
        "disappeared": sorted(old.keys() - new.keys()),
        "old": old,
        "new": new,
    }


def print_summary(report: dict, *, all_units: bool = False) -> None:
    m = report.get("measures", {})
    units = sorted(report.get("units", []), key=lambda u: (_pct(u["measures"]), u["name"]))
    shown = units if all_units else [u for u in units if _pct(u["measures"]) < 100.0]
    print(f"\n{'unit':<32} {'fuzzy%':>8} {'fns':>6} {'matched':>8} {'code':>9}")
    print("-" * 68)
    for u in shown:
        um = u["measures"]
        print(f"{u['name']:<32} {_pct(um):>8.2f} "
              f"{um.get('total_functions', 0):>6} "
              f"{um.get('matched_functions', 0):>8} "
              f"{um.get('total_code', 0):>9}")
    if not all_units:
        print(f"... and {len(units) - len(shown)} unit(s) at 100.00%")
    print("-" * 68)
    print(f"overall fuzzy {_pct(m):.5f}%  "
          f"functions {m.get('matched_functions', 0)}/{m.get('total_functions', 0)} "
          f"({_pct(m, 'matched_functions_percent'):.2f}%)  "
          f"code {m.get('matched_code', 0)}/{m.get('total_code', 0)} "
          f"({_pct(m, 'matched_code_percent'):.2f}%)  "
          f"units {m.get('total_units', 0)}")


def print_reference_diff(reference: dict, current: dict) -> dict:
    d = diff_reports(reference, current)
    old, new = d["old"], d["new"]
    ref_m, cur_m = reference.get("measures", {}), current.get("measures", {})
    print("\n=== vs reference ===")
    print(f"overall fuzzy   old {_pct(ref_m):.5f}%   new {_pct(cur_m):.5f}%   "
          f"delta {_pct(cur_m) - _pct(ref_m):+.5f}")
    print(f"functions       old {ref_m.get('total_functions', 0)}   "
          f"new {cur_m.get('total_functions', 0)}")
    print(f"equal {d['equal']}  improved {len(d['improved'])}  "
          f"regressed {len(d['regressed'])}  appeared {len(d['appeared'])}  "
          f"disappeared {len(d['disappeared'])}")
    if d["regressed"]:
        print(f"\n--- regressed ({len(d['regressed'])}) ---")
        print(f"{'unit':<28} {'symbol':<58} {'old%':>8} {'new%':>8} {'delta':>8}")
        for key in d["regressed"]:
            unit, symbol = key
            print(f"{unit:<28} {symbol:<58} {old[key]:>8.2f} {new[key]:>8.2f} "
                  f"{new[key] - old[key]:>+8.2f}")
    return d


def run(base_dir: Path = BASE_DIR, target_dir: Path = TARGET_DIR,
        out_dir: Path = OUT_DIR, *, units: list[str] | None = None,
        reference: Path | None = None, force: bool = False,
        all_units: bool = False, quiet: bool = False) -> dict:
    """normalize -> project -> objdiff-cli -> the loaded report."""
    base_dir, target_dir, out_dir = Path(base_dir), Path(target_dir), Path(out_dir)
    for label, path in (("base", base_dir), ("target", target_dir)):
        if not path.is_dir():
            raise ToolError(f"{label} object directory missing: {path}")

    from homm1.manifest import load as load_manifest
    manifest = load_manifest()
    census = manifest.get("unit", [])
    unit_names = units if units is not None else [u["unit"] for u in census]

    normalize(base_dir, target_dir, out_dir, unit_names, force=force, quiet=quiet)
    build = manifest.get("build", {})
    # The pairing census reads the DELINKER's directory (normalize mirrors that
    # set under <out_dir>/target/ keeping each object's own file name).
    project_mod.project(
        [{"unit": u} for u in unit_names], target_dir, out_dir,
        platform=build.get("platform", project_mod.DEFAULT_PLATFORM),
        compiler=build.get("compiler", project_mod.DEFAULT_COMPILER),
        base_subdir="base", target_subdir="target")

    report_path = objdiff.report(out_dir, out_dir / "report.json")
    report = objdiff.load(report_path)

    if not quiet:
        no_target = sorted(set(unit_names) - units_with_a_target(target_dir))
        if no_target:
            print(f"[compare] {len(no_target)} unit(s) have no delinked target and "
                  f"pair against dummy.obj: {', '.join(no_target[:8])}"
                  + (" ..." if len(no_target) > 8 else ""))
        print_summary(report, all_units=all_units)
        if reference is not None:
            print_reference_diff(objdiff.load(reference), report)
    return report


def main() -> int:
    import sys
    ap = argparse.ArgumentParser(
        prog="homm1 compare", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--base-dir", type=Path, default=BASE_DIR,
                    help=f"recompiled base objs (default: {BASE_DIR})")
    ap.add_argument("--target-dir", type=Path, default=TARGET_DIR,
                    help=f"delinked target objs (default: {TARGET_DIR})")
    ap.add_argument("--out-dir", type=Path, default=OUT_DIR,
                    help=f"normalized copies + report (default: {OUT_DIR})")
    ap.add_argument("--reference", type=Path,
                    help="an earlier report.json to diff per-function scores against")
    ap.add_argument("--force", action="store_true",
                    help="re-normalize every object, ignoring the stale check")
    ap.add_argument("--all-units", action="store_true",
                    help="list every unit, not only those below 100%%")
    a = ap.parse_args()
    try:
        run(a.base_dir, a.target_dir, a.out_dir, reference=a.reference,
            force=a.force, all_units=a.all_units)
    except ToolError as e:
        print(f"[compare] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

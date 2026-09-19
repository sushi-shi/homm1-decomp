"""homm1.graph.verbs - the three verbs that drive the graph.

    homm1 build [targets...] [-j N] [--force-delink] [-v]
    homm1 link  [--engine-lib] [--order F] ...     -> `ninja candidate`
    homm1 match [--reference R] [--all]            -> build, then the deltas
    homm1 play  [--retail]                         -> build + link, install
                                                       into the game env, run
                                                       scaled (play.sh)

`build` configures if the manifest is missing (after that ninja's generator
edge owns it) and runs the default target. `link` is the same graph with the
opt-in phase-2 target. `match` is the agent-facing one: it runs the build, then
reports the compare summary for exactly the units whose objects CHANGED - which
is the question a matcher actually asks, and the reason the report is an in-graph
edge rather than an unconditional call (a no-op build has nothing to report).

"Changed" is decided by CONTENT, not mtime: homm1.graph.cc writes objects
if-changed with the COFF timestamp stabilised, so a hash census before and
after the build names precisely the units whose codegen moved.
"""

from __future__ import annotations

import hashlib
import subprocess
import sys
from pathlib import Path

from homm1 import graph
from homm1.core.paths import REPO


def toolchain_repinned() -> bool:
    """True when $MSVC_DIR/$DXSDK_DIR/the delinker no longer match the manifest.

    The generator edge cannot answer this: ninja reruns it from FILES, and the
    toolchain is environment. So the driver checks it before handing over. A
    re-pin must reconfigure rather than merely rebuild, because whether the
    `rc` edge exists at all is decided at configure time - that is how a
    pre-r3 shell silently produced a candidate with no `.rsrc`.
    """
    from homm1.graph.emit import toolchain_id
    path = REPO / graph.TOOLCHAIN_ID
    if not path.exists():
        return (REPO / graph.NINJA).exists()
    return path.read_text() != toolchain_id()


def configure_if_needed(force: bool = False) -> None:
    """Emit build/build.ninja when it is absent, forced, or the toolchain moved."""
    repinned = toolchain_repinned()
    if force or repinned or not (REPO / graph.NINJA).exists():
        if repinned and not force:
            print("[configure] the pinned toolchain or delinker moved since this "
                  "manifest was written - reconfiguring", file=sys.stderr)
        from homm1.graph.emit import emit
        n, pruned = emit()
        print(f"[configure] wrote {graph.NINJA} ({n} units"
              + (f", pruned {pruned} orphan artifact(s)" if pruned else "") + ")")


def ninja(targets: list[str] = (), *, jobs: int | None = None,
          verbose: bool = False, keep_going: bool = False,
          extra: list[str] = ()) -> int:
    """Run ninja against the emitted manifest from the repo root."""
    argv = ["ninja", "-f", graph.NINJA]
    if jobs:
        argv += ["-j", str(jobs)]
    if verbose:
        argv.append("-v")
    if keep_going:
        argv += ["-k", "0"]
    argv += [*extra, *targets]
    return subprocess.run(argv, cwd=REPO).returncode


def object_census() -> dict[str, str]:
    """{unit: sha256} over the base objects present right now."""
    base = REPO / graph.BASE_DIR
    if not base.is_dir():
        return {}
    return {p.stem: hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(base.glob("*.obj"))}


def changed_units(before: dict[str, str], after: dict[str, str]) -> list[str]:
    return sorted(u for u in after if before.get(u) != after[u])


# --------------------------------------------------------------------------- #
# homm1 build
# --------------------------------------------------------------------------- #
def build_main(argv: list[str] | None = None) -> int:
    """Configure-if-needed, then ninja's default target."""
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 build", description=build_main.__doc__)
    ap.add_argument("targets", nargs="*", help="ninja targets (default: all)")
    ap.add_argument("-j", "--jobs", type=int)
    ap.add_argument("-v", "--verbose", action="store_true")
    ap.add_argument("--reconfigure", action="store_true",
                    help="re-emit build/build.ninja before building")
    ap.add_argument("--force-delink", action="store_true",
                    help="drop the delink stamp so the delinker re-runs even "
                         "though bindings.tsv did not change")
    a = ap.parse_args(argv)
    configure_if_needed(a.reconfigure)
    if a.force_delink:
        (REPO / graph.DELINK_STAMP).unlink(missing_ok=True)
    return ninja(a.targets, jobs=a.jobs, verbose=a.verbose)


# --------------------------------------------------------------------------- #
# homm1 link
# --------------------------------------------------------------------------- #
def manifest_targets() -> set[str]:
    """Every output the current Ninja manifest declares."""
    out: set[str] = set()
    try:
        text = (REPO / graph.NINJA).read_text(encoding="utf-8")
    except OSError:
        return out
    for line in text.replace("$\n", " ").splitlines():
        if line.startswith("build "):
            out.update(line[len("build "):].split(":", 1)[0].split())
    return out


def link_main(argv: list[str] | None = None) -> int:
    """Build the opt-in candidate executable and link map."""
    argv = list(sys.argv[1:] if argv is None else argv)
    from homm1.graph.link import main as link_direct
    if any(a in ("-h", "--help") for a in argv):
        sys.argv = ["homm1 link", *argv]
        return link_direct()
    configure_if_needed()
    if not argv:
        return ninja(["candidate"])
    targets = ["base"]
    if graph.RESOURCE_RES in manifest_targets():
        targets.append(graph.RESOURCE_RES)
    rc = ninja(targets)
    if rc:
        return rc
    sys.argv = ["homm1 link", *argv]
    return link_direct()


def _pct(measures: dict, key: str = "fuzzy_match_percent") -> float:
    return float(measures.get(key) or 0.0)


def print_changed(report: dict, units: list[str], *, functions: bool = True,
                  limit: int = 60) -> None:
    """The compare summary restricted to `units`, plus the overall line."""
    by_name = {u["name"]: u for u in report.get("units", [])}
    print(f"\n{'unit':<32} {'fuzzy%':>8} {'fns':>6} {'matched':>8} {'code':>9}")
    print("-" * 68)
    for name in sorted(units, key=lambda n: (_pct(by_name.get(n, {}).get(
            "measures", {})), n)):
        u = by_name.get(name)
        if u is None:
            print(f"{name:<32} {'(not in the report - no pairing)':>35}")
            continue
        m = u["measures"]
        print(f"{name:<32} {_pct(m):>8.2f} {m.get('total_functions', 0):>6} "
              f"{m.get('matched_functions', 0):>8} {m.get('total_code', 0):>9}")
    if functions:
        rows = [(n, fn) for n in units for fn in by_name.get(n, {}).get("functions", [])
                if _pct(fn) < 100.0]
        if rows:
            print(f"\n--- functions below 100% in the {len(units)} changed unit(s) "
                  f"({len(rows)}) ---")
            print(f"{'unit':<28} {'symbol':<58} {'fuzzy%':>8}")
            for name, fn in sorted(rows, key=lambda r: _pct(r[1]))[:limit]:
                print(f"{name:<28} {fn.get('name', ''):<58} {_pct(fn):>8.2f}")
            if len(rows) > limit:
                print(f"... and {len(rows) - limit} more")
    m = report.get("measures", {})
    print("-" * 68)
    print(f"overall fuzzy {_pct(m):.5f}%  "
          f"functions {m.get('matched_functions', 0)}/{m.get('total_functions', 0)} "
          f"({_pct(m, 'matched_functions_percent'):.2f}%)  "
          f"code {m.get('matched_code', 0)}/{m.get('total_code', 0)} "
          f"({_pct(m, 'matched_code_percent'):.2f}%)  "
          f"units {m.get('total_units', 0)}")


def match_main(argv: list[str] | None = None) -> int:
    """Build, then print the compare summary for the units that changed."""
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 match", description=match_main.__doc__)
    ap.add_argument("-j", "--jobs", type=int)
    ap.add_argument("-v", "--verbose", action="store_true")
    ap.add_argument("--reference", type=Path,
                    help="an earlier report.json to diff per-function scores against")
    ap.add_argument("--all", action="store_true",
                    help="summarise every unit below 100%%, not only the changed ones")
    ap.add_argument("--no-functions", dest="functions", action="store_false",
                    help="unit rows only")
    ap.add_argument("-k", "--keep-going", action="store_true",
                    help="build every edge that can still build, then summarise "
                         "anyway. The exit code still reports the failure and "
                         "the summary is flagged as possibly stale - use it "
                         "when one broken edge would otherwise hide 310 units")
    a = ap.parse_args(argv)

    configure_if_needed()
    before = object_census()
    rc = ninja(jobs=a.jobs, verbose=a.verbose, keep_going=a.keep_going)
    if rc and not a.keep_going:
        return rc
    after = object_census()
    changed = changed_units(before, after)

    from homm1.compare.run import print_reference_diff, print_summary
    from homm1.tool import objdiff
    report_path = REPO / graph.REPORT_JSON
    if not report_path.exists():
        print(f"[match] no report at {graph.REPORT_JSON} - the build produced "
              "none", file=sys.stderr)
        return 1
    report = objdiff.load(report_path)

    if rc:
        print(f"\n[match] BUILD FAILED (ninja rc={rc}) - the summary below is "
              "whatever the last complete report says and may be STALE",
              file=sys.stderr)
    print(f"\n[match] {len(changed)} unit object(s) changed"
          + (f": {', '.join(changed[:12])}" + (" ..." if len(changed) > 12 else "")
             if changed else " (nothing rebuilt)"))
    if a.all or not changed:
        print_summary(report, all_units=False)
    else:
        print_changed(report, changed, functions=a.functions)
    if a.reference is not None:
        try:
            reference = objdiff.load(a.reference)
        except (OSError, ValueError) as e:
            print(f"[match] --reference {a.reference} is not a readable "
                  f"objdiff report: {e}", file=sys.stderr)
            return rc or 2
        print_reference_diff(reference, report)
    return rc


VERBS = {"build": build_main, "link": link_main, "match": match_main}


def main() -> int:
    argv = sys.argv[1:]
    if not argv or argv[0] not in VERBS:
        print(f"usage: python3 -m homm1.graph.verbs "
              f"{{{'|'.join(VERBS)}}} [args]",
              file=sys.stderr)
        return 2
    return VERBS[argv[0]](argv[1:])


if __name__ == "__main__":
    raise SystemExit(main())

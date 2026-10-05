"""homm1.graph.verbs - the three verbs that drive the graph.

    homm1 build [targets...] [-j N] [--force-delink] [-v]
    homm1 link  [--engine-lib] [--order F] ...     -> `ninja candidate`
    homm1 match [--reference R] [--all]            -> build, then the deltas
    homm1 play  [--game PATH] [--retail]           -> build + link, run in the
                                                       game's Wine prefix

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
    from homm1.core.usage import run_process
    return run_process(argv, cwd=REPO)


def object_census() -> dict[str, str]:
    """{unit: sha256} over the base objects present right now."""
    base = REPO / graph.BASE_DIR
    if not base.is_dir():
        return {}
    return {p.relative_to(base).with_suffix("").as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(base.rglob("*.obj"))}


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
    rc = ninja(a.targets, jobs=a.jobs, verbose=a.verbose)
    if rc == 0:
        print_data_debt()
    return rc


def print_data_debt() -> None:
    """The two data-debt counts, every build, in both modes.

    `unprovisioned` is the delink worklist (build/gen/data_debt.tsv, derived by
    homm1.delink.pdb_synth); `placeholder externs` is the source-side closure
    (homm1.verify.undefined_closure). Strict mode fails on either; relaxed
    mode lists them, and this line keeps them from going unseen."""
    from homm1.core import data_matching
    from homm1.core.tsv import read as read_tsv
    from homm1.delink.pdb_synth import DATA_DEBT
    from homm1.verify import undefined_closure
    try:
        unprovisioned = len(read_tsv(DATA_DEBT)[2])
    except (OSError, ValueError):
        unprovisioned = None
    try:
        externs = len(undefined_closure.placeholder_externs())
    except (OSError, ValueError, SystemExit):
        externs = None

    def shown(n):
        return "?" if n is None else str(n)
    print(f"[homm1 build] data debt ({data_matching.label()}): "
          f"{shown(unprovisioned)} unprovisioned identit(ies) "
          f"({DATA_DEBT.relative_to(REPO)}), {shown(externs)} placeholder "
          f"extern(s) (`homm1 verify undefined-closure --list`)")



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
    from homm1.graph.emit import era_rc_available
    if graph.RESOURCE_RES not in manifest_targets() and era_rc_available():
        # The rc edge is decided at configure time; installing the pinned
        # RC/CVTRES later does not move the toolchain identity.
        configure_if_needed(force=True)
    if not argv:
        return ninja(["candidate"])
    targets = ["link-inputs"]
    if graph.RESOURCE_RES in manifest_targets():
        targets.append(graph.RESOURCE_RES)
        if not any(a == "--res" or a.startswith("--res=") for a in argv):
            argv = ["--res", str(REPO / graph.RESOURCE_RES), *argv]
    rc = ninja(targets)
    if rc:
        return rc
    # The generated candidate edge passes the OMF variants explicitly.  Keep
    # the direct CLI (`homm1 link --dry-run`, experimental flags) on the same
    # object set unless the caller supplied an order or explicit objects.
    if not any(a == "--order" or a == "--obj" or a.startswith("--order=")
               for a in argv):
        from homm1.graph.fixed_asm import unit as fixed_asm_unit
        from homm1.manifest import units
        for record in units():
            unit = record["unit"]
            if fixed_asm_unit(unit, record["source"]) is not None:
                obj = f"{graph.LINK_OMF_DIR}/{unit}.obj"
            else:
                obj = f"{graph.BASE_DIR}/{unit}.obj"
            argv.extend(["--obj", obj])
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


def resolve_units(specs: list[str]) -> list[str]:
    """Unit stems from stems or source paths (`fader`, `src/DDrawMgr/Fader.cpp`)."""
    from homm1.manifest import units as manifest_units
    rows = manifest_units()
    by_stem = {u["unit"]: u for u in rows}
    by_source = {str(Path(u["source"])): u["unit"] for u in rows}
    out = []
    for spec in specs:
        if spec in by_stem:
            out.append(spec)
            continue
        path = Path(spec)
        if path.is_absolute():
            try:
                path = path.resolve().relative_to(REPO)
            except ValueError:
                pass
        unit = by_source.get(str(path))
        if unit is None:
            raise SystemExit(f"homm1 match: {spec!r} is not a unit stem or a "
                             "unit source in config/units.toml")
        out.append(unit)
    return list(dict.fromkeys(out))


def match_units(units: list[str], *, jobs: int | None, verbose: bool) -> int:
    """The fast loop: compile, label, delink, and compare only `units`.

    Every other unit keeps its last-built objects, claims, and scores, even
    when an edited header would change them; `homm1 build` refreshes them.
    No fingerprints, no gates.
    """
    import time

    from homm1.compare import normalize, project
    from homm1.delink import run as delink
    from homm1.manifest import units as manifest_units
    from homm1.model import resolve, serialize
    from homm1.tool import objdiff
    from homm1.verify import scores

    started = time.monotonic()
    report_path = REPO / graph.REPORT_JSON
    before = scores.functions(scores.load(report_path)) if report_path.exists() else {}

    targets = [f"{graph.BASE_DIR}/{u}.obj" for u in units]
    targets += [f"{graph.CLAIMS_DIR}/{u}.tsv" for u in units]
    rc = ninja(targets, jobs=jobs, verbose=verbose)
    if rc:
        return rc

    model = resolve()
    bindings_changed, _ = serialize(model)
    target_dir = REPO / graph.TARGET_DIR
    missing = [u for u in units if not (target_dir / f"{u}.c.obj").exists()]
    if bindings_changed or missing:
        delink.run(model, target_dir=target_dir,
                   only=None if bindings_changed else units)
    project.project(manifest_units(), target_dir, REPO / graph.COMPARE_DIR)
    normalize.normalize(REPO / graph.BASE_DIR, target_dir,
                        REPO / graph.COMPARE_DIR, units,
                        stamp=REPO / "build/match/normalize.stamp")
    objdiff.report(REPO / graph.COMPARE_DIR, report_path)

    after = scores.functions(scores.load(report_path))
    print_unit_functions(units, before, after)
    print(f"\n[match] {', '.join(units)} in {time.monotonic() - started:.1f}s"
          + (" (labels changed: delinked)" if bindings_changed else ""))
    return 0


def print_unit_functions(units: list[str], before: dict, after: dict) -> None:
    """MAX movement only. An unchanged function keeps its banked MAX, so a CUR
    dip is noise and stays silent; it is listed only when it rises above MAX.
    An edited function's MAX becomes its new score, so it is listed with the
    MAX it replaces: `drop` when the edit moved its CUR down, `reset` when CUR
    held and only the new source hash lowered MAX (HIST keeps the old peak)."""
    from contextlib import redirect_stdout
    from io import StringIO

    from homm1.verify import baseline
    from homm1.verify.baseline import EPS
    from homm1.verify.fingerprints import fingerprinter, real_edit, regenerate
    with redirect_stdout(StringIO()):
        regenerate()
    fp, _cpp_of, _stale = fingerprinter()
    bank = baseline.load()
    mismatch = baseline.mode_mismatch()
    if mismatch:
        print(f"WARNING: MAX comparisons below are meaningless - {mismatch}")
    resets = []
    for unit in units:
        score_unit = unit.rsplit("/", 1)[-1]
        rows = sorted((name, pct) for (u, name), pct in after.items() if u == score_unit)
        if not rows:
            print(f"\n{unit}: no paired functions in the report")
            continue
        shown, at_max = [], 0
        for name, pct in rows:
            row = bank.get((score_unit, name))
            if row is None:
                shown.append((pct, None, name, "new"))
                at_max += pct >= 100.0
                continue
            edited = real_edit(row["fp"], fp(score_unit, name))
            new_max = pct if edited else max(row["best"], pct)
            at_max += new_max >= 100.0
            was = before.get((score_unit, name))
            if edited and pct > row["best"] + EPS:
                shown.append((pct, row["best"], name, "up"))
            elif edited and row["best"] - pct > EPS:
                held = was is not None and abs(pct - was) <= EPS
                if held:
                    resets.append((row.get("addr"), unit, name, row["best"], pct))
                shown.append((pct, row["best"], name,
                              "reset: CUR held, recorded for syntactic recovery"
                              if held else "drop"))
            elif edited and pct < 100.0:
                shown.append((pct, row["best"], name, "edited"))
            elif not edited and pct > row["best"] + EPS:
                shown.append((pct, row["best"], name, "up"))
        print(f"\n{unit}: {at_max}/{len(rows)} at MAX 100")
        if not shown:
            print("  no MAX change")
            continue
        print(f"  {'now':>8} {'max':>8}  function  [kind]")
        for pct, best, name, kind in sorted(shown, key=lambda r: (r[0], r[2])):
            print(f"  {pct:8.2f} {'' if best is None else f'{best:8.2f}':>8}  "
                  f"{name}  [{kind}]")
    record_resets(resets)


#: Functions whose MAX an edit lowered while their CUR held: a later
#: fuzzy syntactic recovery pass looks for a spelling that regains the peak.
RECOVERY_TODO = REPO / "build/match/syntactic-recovery.tsv"


def record_resets(resets: list) -> None:
    """Add or update one row per reset function, keeping the highest lost MAX."""
    if not resets:
        return
    header = "rva\tunit\tfunction\tlost_max\tcur\n"
    rows: dict[tuple[str, str], list[str]] = {}
    if RECOVERY_TODO.exists():
        for line in RECOVERY_TODO.read_text().splitlines()[1:]:
            cols = line.split("\t")
            if len(cols) == 5:
                rows[(cols[1], cols[2])] = cols
    for addr, unit, name, lost_max, pct in resets:
        old = rows.get((unit, name))
        peak = max(lost_max, float(old[3])) if old else lost_max
        rows[(unit, name)] = ["" if addr is None else f"0x{addr:06x}", unit,
                              name, f"{peak:.4f}", f"{pct:.4f}"]
    text = header + "".join("\t".join(r) + "\n" for r in sorted(
        rows.values(), key=lambda r: (r[1], r[2])))
    if not RECOVERY_TODO.exists() or RECOVERY_TODO.read_text() != text:
        RECOVERY_TODO.parent.mkdir(parents=True, exist_ok=True)
        RECOVERY_TODO.write_text(text)


def match_main(argv: list[str] | None = None) -> int:
    """Fast loop for named units; with none, build everything and summarise
    the units whose objects changed."""
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 match", description=match_main.__doc__)
    ap.add_argument("units", nargs="*",
                    help="unit stems or source paths: compile, delink and "
                         "compare only these")
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
    if a.units:
        return match_units(resolve_units(a.units), jobs=a.jobs, verbose=a.verbose)

    from homm1.verify import scores
    report_path = REPO / graph.REPORT_JSON
    before_scores = (scores.functions(scores.load(report_path))
                     if report_path.exists() else {})
    before = object_census()
    rc = ninja(["compare"], jobs=a.jobs, verbose=a.verbose,
               keep_going=a.keep_going)
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
    elif a.functions:
        print_unit_functions(changed, before_scores,
                             scores.functions(scores.load(report_path)))
    else:
        print_changed(report, changed, functions=False)
    if a.reference is not None:
        try:
            reference = objdiff.load(a.reference)
        except (OSError, ValueError) as e:
            print(f"[match] --reference {a.reference} is not a readable "
                  f"objdiff report: {e}", file=sys.stderr)
            return rc or 2
        print_reference_diff(reference, report)
    return rc


# --------------------------------------------------------------------------- #
# homm1 play
# --------------------------------------------------------------------------- #
def play_main(argv: list[str] | None = None) -> int:
    """Build and link the candidate, then run it in the game's Wine prefix
    (homm1.graph.play, the runner the generated source tree also carries).

    The game copy is given once with --game and imported into the per-user
    state directory, which the source tree's `nix run .#play` shares.
    `--retail` runs the staged retail HEROES.EXE there instead: the control
    for triaging Wine. Arguments after `--` go to the game.
    """
    import argparse
    from homm1.graph import play
    from homm1.graph.link import has_rsrc
    ap = argparse.ArgumentParser(prog="homm1 play", description=play_main.__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    play.add_arguments(ap, standalone=False)
    ap.add_argument("--retail", action="store_true",
                    help="run the staged retail HEROES.EXE instead of the candidate")
    mine, extra = play.split_argv(argv)
    a = ap.parse_args(mine)

    def build(_icon: Path | None) -> Path | None:
        from homm1.core.paths import retail_exe
        if a.retail:
            return retail_exe()
        executable = REPO / graph.CANDIDATE_EXE
        if a.dry_run:
            play.say(f"would build and link {executable} (homm1 build, homm1 link)")
            return executable
        if build_main([]) or link_main([]):
            return None
        if not has_rsrc(executable):
            print("[play] the candidate has no .rsrc (menus, icon): install the pinned "
                  "resource compiler (`homm1 toolchain install`)", file=sys.stderr)
            return None
        return executable

    return play.session(a, extra, build)


VERBS = {"build": build_main, "link": link_main, "match": match_main, "play": play_main}


from homm1.core.usage import logged


@logged
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

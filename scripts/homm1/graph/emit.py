"""homm1.graph.emit - configure: config/units.toml -> build/build.ninja.

    python3 -m homm1.graph            # (re)write build/build.ninja
    ninja -f build/build.ninja         # run the loop, from the repo root

The rules, in the order the loop runs them. The first nine plus the two
`verify` edges are the DEFAULT target; `rc`/`link` are phase 2, opt-in:

    configure   the generator edge - re-emits this manifest when the unit
                census, the emitter, or ANY file the include scan read changes
    cl          source -> build/objdiff/base/<unit>.obj   (homm1.graph.cc)
    compdb      units.toml -> build/clangd/compile_commands.json - the clang-cl
                flags extraction and the LSP consumers ride (homm1.graph.compdb)
    labels      source + headers + base obj -> build/gen/claims/<unit>.tsv
    model       claims x censuses/providers -> build/gen/bindings.tsv
    delink      bindings -> build/objdiff/target-new/<unit>.c.obj
    normalize   base + target objs -> the comparison copies
    project     the delinked directory -> compare-new/objdiff.json
    report      comparison copies + pairing -> compare-new/report.json
    verify_fp   sources x bindings -> the per-function fingerprint cache
    verify_check the MAX gate + the fast+normal tiers -> a stamp; FATAL
    rc / link   PHASE 2, opt-in (`ninja candidate`): base objs + .res ->
                the candidate image + .map for the link-order study

Two edges declare a STAMP rather than their real outputs, because neither set
can be enumerated at configure time: `delink` writes one object per unit that
has a claim (a unit with none writes nothing, so declaring every unit would
leave ninja re-running the whole delink on every build), and `normalize`
writes a variable pair of copies per unit. Both drivers are keyed on content
upstream, so the stamp only moves when something real did.

Restat is on `cl`, `compdb`, `labels`, `model` and `project` - the producers
that write if-changed. That is the whole incrementality story: a pure code
edit re-runs configure (every source is in the include scan's own dep set) +
cl + labels, stops at an unchanged claim fragment, and reaches the report
without re-delinking; a label edit carries on through model, delink and the
pairing. Labels declare the same per-TU header closure as `cl`: extraction
reads inline `RVA` annotations from headers even when MSVC emits no changed
bytes, so the object edge's `restat` cannot be allowed to hide a renamed claim.

`verify_fp` also carries restat, but MEASURED its producer rewrites the cache
unconditionally (identical bytes, fresh mtime), so the restat is inert there
and `verify_check` re-runs after any source edit.

The era toolchain ($MSVC_DIR/$DXSDK_DIR) and the vostok-delinker binary are
environment rather than files under the repo, so they are declared INDIRECTLY:
`toolchain_id()` renders all three into build/gen/toolchain.id (write-if-changed),
and the cl, compdb and delink edges list that file. A re-pin therefore
invalidates exactly the edges it should. This is not cosmetic - before it, a
toolchain swap recompiled only the units that happened to be dirty and left the
rest built by the previous cl, which for a byte-matching project is the worst
possible failure, and a delinker swap gave `ninja: no work to do`.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

from homm1 import graph
from homm1.core.paths import REPO, msvc_dir
from homm1.graph import ninja_syntax
from homm1.graph.scan import Scanner

SCRIPTS = "scripts/homm1"
MANIFEST = "config/units.toml"
RETAIL_EXE = "build/orig/HEROES.EXE"
COMPDB = "build/clangd/compile_commands.json"
RELOC_REFERENTS = "config/retail/reloc_referents.tsv"
FUNCTION_REFERENTS = "config/retail/function_referents.tsv"

#: The census + provider tables homm1.model joins the claims against. Named
#: rather than globbed: reloc_referents.tsv is a DELINKER input and belongs on
#: that edge, and a new table should be a deliberate edit here.
MODEL_TABLES = [
    "config/retail/functions.tsv", "config/retail/data.tsv",
    "config/retail/link_order.tsv", "config/retail/link_bands.tsv",
    "config/retail/functions_static_libs.tsv", "config/retail/functions_zlib.tsv",
    "config/retail/data_zlib.tsv", "config/retail/data_vtables.tsv",
    "config/retail/data_static_libs.tsv", "config/retail/data_compgen.tsv",
]


def _mods(*rel: str) -> list[str]:
    """Repo-relative module paths under scripts/homm1 that exist."""
    out = []
    for r in rel:
        p = f"{SCRIPTS}/{r}"
        if r.endswith("/"):
            out += sorted(str(q.relative_to(REPO))
                          for q in (REPO / p).glob("*.py")) if (REPO / p).is_dir() else []
        elif (REPO / p).exists():
            out.append(p)
    return out


#: Per-edge module deps. Hand-listed rather than "every .py under scripts/":
#: the labels edge is 311 clang passes, and making it depend on the whole
#: toolchain would re-run all of them whenever an unrelated module is touched.
TOOL_MODS = _mods("tool/__init__.py", "tool/wine.py", "core/paths.py")
CL_MODS = _mods("graph/cc.py", "tool/cl.py") + TOOL_MODS
ML_MODS = _mods("graph/fixed_asm.py", "tool/ml.py") + TOOL_MODS
COMPDB_MODS = _mods("graph/compdb.py", "tool/clang.py", "manifest.py",
                    "core/paths.py")
LABELS_MODS = _mods("retail_labels/", "tool/clang.py", "core/coff.py",
                    "core/tsv.py", "manifest.py", "core/paths.py")
MODEL_MODS = _mods("model.py", "retail_labels/", "core/tsv.py", "core/paths.py")
DELINK_MODS = _mods("delink/", "tool/delinker.py", "core/pe.py",
                    "core/coff.py", "model.py") + TOOL_MODS
NORMALIZE_MODS = _mods("compare/normalize.py", "compare/canonicalize.py",
                       "delink/eh_band.py", "core/coff.py")
PROJECT_MODS = _mods("compare/project.py", "compare/normalize.py", "manifest.py")
REPORT_MODS = _mods("tool/objdiff.py")
LINK_MODS = _mods("graph/link.py", "graph/implib.py", "tool/link.py",
                  "core/pe.py") + TOOL_MODS
VERIFY_MODS = _mods("verify/", "model.py", "core/tsv.py", "core/paths.py")
#: committed inputs of the default-tier verify gates (fast+normal): the MAX
#: ledger and every gate's own baseline/allowlist. Named so a bless re-runs
#: the check edge.
VERIFY_BASELINES = [
    "config/match_baseline.tsv",
    "config/cleanliness/cleanliness-text-baseline.tsv",
    "config/cleanliness/cleanliness-semantic-baseline.tsv",
    "config/cleanliness/tu-order-baseline.tsv",
    "config/cleanliness/data-tu-order-baseline.tsv",
    "config/cleanliness/kept-comdat-exiles.tsv",
]
FINGERPRINTS = "build/gen/func_fingerprints.tsv"
VERIFY_STAMP = "build/objdiff/.verify.stamp"
CONFIGURE_MODS = _mods("graph/", "manifest.py", "core/paths.py")


# --------------------------------------------------------------------------- #
# manifest
# --------------------------------------------------------------------------- #
def load_units() -> tuple[dict, list[dict]]:
    """(manifest, units) with each unit's `cflags` resolved from its profile.

    Every [[unit]] names ONE [flags] profile and the profile is the FULL flag
    set - there is no global default to inherit and no per-TU append, so a
    unit's flag choice stays one explicit, greppable name. A stray `extra` key
    is a hard error rather than a silent bolt-on.
    """
    from homm1.manifest import load
    data = load()
    profiles = data.get("flags", {})
    if not profiles:
        raise SystemExit(f"{MANIFEST}: [flags] must define at least one profile")
    units = data.get("unit", [])
    if not units:
        raise SystemExit(f"{MANIFEST}: no [[unit]] entries")
    seen: set[str] = set()
    for u in units:
        for key in ("unit", "source", "flags"):
            if key not in u:
                raise SystemExit(f"{MANIFEST}: a [[unit]] is missing '{key}'")
        if u["unit"] in seen:
            raise SystemExit(f"{MANIFEST}: duplicate unit '{u['unit']}'")
        seen.add(u["unit"])
        if u["flags"] not in profiles:
            raise SystemExit(f"{MANIFEST}: unit '{u['unit']}' references unknown "
                             f"flags profile '{u['flags']}' "
                             f"(defined: {sorted(profiles)})")
        if "extra" in u:
            raise SystemExit(
                f"{MANIFEST}: unit '{u['unit']}' sets 'extra' - per-TU flag "
                "bolt-ons are not supported. Add (or reuse) a [flags] profile "
                "carrying the FULL set instead.")
        u["cflags"] = list(profiles[u["flags"]])
    return data, units


# --------------------------------------------------------------------------- #
# orphan artifacts
# --------------------------------------------------------------------------- #
#: (directory, "<prefix>{}<suffix>") pairs whose stems must be live units.
#: build/objdiff/target-new and build/delink/named are NOT listed: their
#: producers rmtree them, so they cannot hold an orphan.
_ORPHAN_PATTERNS = [
    (graph.BASE_DIR, "{}.obj"),
    (f"{graph.COMPARE_DIR}/base", "{}.obj"),
    (f"{graph.COMPARE_DIR}/base", "{}.symbols.tsv"),
    (f"{graph.COMPARE_DIR}/target", "{}.c.obj"),
    (f"{graph.COMPARE_DIR}/target", "{}.symbols.tsv"),
    (graph.CLAIMS_DIR, "{}.tsv"),
]


def prune_orphan_artifacts(units: list[dict]) -> int:
    """Delete build artifacts of units no longer in config/units.toml.

    Ninja has no concept of an output whose EDGE disappeared, so dropping a
    unit - a source deleted, a TU folded, a branch switched in a shared
    worktree - leaves its object, claim fragment and comparison copies behind,
    and every downstream reader that GLOBS rather than follows the graph keeps
    consuming them. That is not cosmetic: homm1.delink.pdb_synth and
    homm1.delink.data_manifest both read `build/objdiff/base/*.obj`, so a
    stale object re-enrols its vtables and RTTI into the data manifest for a
    unit that has no source in the tree, and `homm1.delink.run` collects a
    target object for every stem in build/gen/claims. Prune at configure time,
    where the live unit set is known.

    The delink stamp goes with them: the manifests are regenerated in-process
    from whatever objects survive, and without dropping the stamp a prune that
    leaves bindings.tsv unchanged would never re-run the delinker.
    """
    live = {u["unit"] for u in units}
    stems: set[str] = set()
    for rel, pat in _ORPHAN_PATTERNS:
        d = REPO / rel
        if not d.is_dir():
            continue
        head, tail = pat.split("{}")
        for p in d.rglob("*"):
            relative = p.relative_to(d).as_posix()
            if p.is_file() and relative.startswith(head) and relative.endswith(tail):
                stem = relative[len(head):len(relative) - len(tail)]
                if stem and stem not in live:
                    stems.add(stem)
    if not stems:
        return 0
    n = 0
    for rel, pat in _ORPHAN_PATTERNS:
        for stem in stems:
            p = REPO / rel / pat.format(stem)
            if p.exists():
                p.unlink()
                n += 1
    stamp = REPO / graph.DELINK_STAMP
    if stamp.exists():
        stamp.unlink()
        n += 1
    return n


# --------------------------------------------------------------------------- #
# the graph
# --------------------------------------------------------------------------- #
def era_rc_available() -> bool:
    """Whether the installed VC4 tree can compile the optional resources."""
    try:
        from homm1.tool.wine import find_ci
        return find_ci(msvc_dir() / "bin", "rc.exe") is not None
    except OSError:
        return False


def toolchain_id() -> str:
    """The pinned toolchain's identity, as the text that goes in TOOLCHAIN_ID.

    Three values, because three different edges depend on them: $MSVC_DIR and
    $DXSDK_DIR decide what `cl` and the compilation database mean, and the
    vostok-delinker binary decides what the target objects are. All three were
    pure environment, so ninja could not see a re-pin: swapping the delinker
    gave `ninja: no work to do`, and swapping the toolchain recompiled only
    the units that happened to be dirty, mixing two compilers' output in one
    object set.

    Unset/absent values are recorded as `-` rather than skipped: going from
    unset to set is itself a change the edges must see.
    """
    import shutil
    parts = []
    parts.append(f"MSVC_DIR={os.path.realpath(msvc_dir())}")
    delinker = shutil.which("vostok-delinker")
    parts.append("delinker=" + (os.path.realpath(delinker) if delinker else "-"))
    return "\n".join(parts) + "\n"


def write_toolchain_id(out: Path | None = None) -> bool:
    """Write TOOLCHAIN_ID if-changed. True when the content moved.

    If-changed matters: this file is an implicit input of all 300 cl edges, so
    rewriting it unconditionally at every configure would recompile the tree
    whenever anything else re-ran configure.
    """
    path = Path(out) if out is not None else REPO / graph.TOOLCHAIN_ID
    want = toolchain_id()
    if path.exists() and path.read_text() == want:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(want)
    return True


def emit_link_phase(w: ninja_syntax.Writer, cl_edges: list[tuple]) -> None:
    """Emit Gruntz's opt-in candidate link phase.

    MASM's COFF output belongs to objdiff.  The period linker consumes the
    ordinary OMF output, exactly as the donor build does, so replace only the
    fixed assembly objects on this edge.
    """
    w.comment("=== PHASE 2: link -> candidate HEROES.EXE + .map (opt-in) ===")
    link_objs = []
    for obj, src, _headers, _cflags, unit, assembly in cl_edges:
        if assembly is None:
            link_objs.append(obj)
            continue
        omf = f"{graph.LINK_OMF_DIR}/{unit}.obj"
        w.build(omf, "ml_omf", inputs=src,
                implicit=ML_MODS + [graph.TOOLCHAIN_ID],
                variables={"unit": unit})
        link_objs.append(omf)
    w.build("link-inputs", "phony", inputs=link_objs)

    with_res = era_rc_available()
    if with_res:
        w.rule("rc", command="$py -m homm1.tool.rc --out $out --src $in",
               description="rc $out")
        w.build(graph.RESOURCE_RES, "rc", inputs=graph.RESOURCE_SCRIPT,
                implicit=_mods("tool/rc.py") + TOOL_MODS)
    else:
        w.comment("VC4 tree has no RC.EXE; candidate links without resources")
    res_flag = f" --res {graph.RESOURCE_RES}" if with_res else ""
    w.rule("link",
           command=(f"$py -m homm1.graph.link --out {graph.CANDIDATE_EXE} "
                    f"--objs-dir {graph.BASE_DIR}{res_flag} $objects"),
           description="link candidate HEROES.EXE + map")
    w.build([graph.CANDIDATE_EXE, graph.CANDIDATE_MAP], "link",
            inputs=link_objs,
            implicit=([graph.RESOURCE_RES] if with_res else [])
                     + [MANIFEST] + LINK_MODS,
            variables={"objects": " ".join(f"--obj {obj}" for obj in link_objs)})
    w.build("candidate", "phony",
            inputs=[graph.CANDIDATE_EXE, graph.CANDIDATE_MAP])
    w.newline()


def emit(out: Path | None = None) -> tuple[int, int]:
    """Write build/build.ninja. Returns (units, pruned artifacts)."""
    manifest, units = load_units()
    pruned = prune_orphan_artifacts(units)
    out = Path(out) if out is not None else REPO / graph.NINJA
    out.parent.mkdir(parents=True, exist_ok=True)
    # Before the edges that declare it, so the first build after a re-pin sees
    # the new identity rather than racing it.
    write_toolchain_id()
    scan = Scanner()
    global_cflags = next(iter(manifest["flags"].values()))

    # Resolved BEFORE the writer opens, so `scan.scanned()` is complete by the
    # time the generator edge is emitted (ninja does not care about edge order).
    from homm1.graph.fixed_asm import unit as fixed_asm_unit
    cl_edges = [(f"{graph.BASE_DIR}/{u['unit']}.obj", u["source"],
                 scan.headers(u["source"]), u["cflags"], u["unit"],
                 fixed_asm_unit(u["unit"], u["source"])) for u in units]
    base_objs = [e[0] for e in cl_edges]
    headers_by_unit = {e[4]: e[2] for e in cl_edges}

    with out.open("w", encoding="utf-8") as f:
        w = ninja_syntax.Writer(f)
        w.comment("GENERATED by homm1.graph from config/units.toml - do not edit.")
        w.comment("Regenerate: python3 -m homm1.graph   "
                  "Run: ninja -f build/build.ninja (from the repo root)")
        w.newline()

        w.variable("ninja_required_version", "1.11")
        # .ninja_log / .ninja_deps live beside the manifest, not at the repo root.
        w.variable("builddir", "build")
        # The interpreter line pins PYTHONPATH to THIS checkout's scripts. A
        # shell entered in one worktree exports another's, and a build that
        # silently ran a sibling tree's modules is the worst kind of wrong.
        w.variable("py", f"PYTHONPATH={REPO / 'scripts'} HOMM1_DIR={REPO} python3")
        w.variable("cflags", " ".join(global_cflags))
        w.newline()

        # Wine serialises more than it appears under one shared wineserver;
        # past ~8 concurrent cl.exe the server thrashes and the build gets
        # SLOWER. Cap the compiler edges without capping ninja's own -j.
        w.pool("wine", graph.WINE_POOL_DEPTH)
        w.newline()

        w.comment("=== generator: re-emit this manifest when configure inputs move ===")
        w.rule("configure", command="$py -m homm1.graph",
               description="configure (regenerate build/build.ninja)",
               generator=True)
        # The `cl` dep lists below are baked HERE from the include graph as it
        # stands, so an edit that CHANGES that graph invalidates them. Without
        # the scanned set on this edge nothing notices: ninja keeps using the
        # stale list, and a later edit to a newly-included header does not
        # rebuild its TU. That is silent, and it is exactly the failure a
        # byte-neutrality claim from a header edit depends on not happening.
        w.build(graph.NINJA, "configure",
                implicit=[MANIFEST, *CONFIGURE_MODS, *sorted(scan.scanned())])
        w.newline()

        w.comment("=== cl: source -> base .obj (VC4 /Od under wine) ===")
        w.rule("cl",
               command="$py -m homm1.graph.cc --out $out --src $in "
                       "--unit $unit -- $cflags",
               description="cl $unit", pool="wine", restat=True)
        w.rule("ml_coff",
               command="$py -m homm1.tool.ml --src $in --out $out --coff",
               description="assemble-coff $unit", pool="wine", restat=True)
        w.rule("ml_omf",
               command="$py -m homm1.tool.ml --src $in --out $out",
               description="assemble-omf $unit", pool="wine", restat=True)
        w.newline()
        for obj, src, headers, cflags, unit, assembly in cl_edges:
            variables = {"unit": unit}
            if assembly is not None:
                w.build(obj, "ml_coff", inputs=src,
                        implicit=ML_MODS + [graph.TOOLCHAIN_ID],
                        variables=variables)
                continue
            if cflags != global_cflags:
                variables["cflags"] = " ".join(cflags)
            w.build(obj, "cl", inputs=src,
                    implicit=headers + CL_MODS + [graph.TOOLCHAIN_ID],
                    variables=variables)
        w.newline()

        w.comment("=== compdb: units.toml -> the clang-cl compilation db ===")
        # Written if-changed + restat, so a manifest edit that leaves every
        # surviving entry intact re-runs nothing downstream. Extraction reads
        # per-TU flags from this file and a unit with NO entry silently falls
        # back to bare MS flags - the edge is what keeps that from rotting.
        # A toolchain re-pin is visible here: $MSVC_DIR/$DXSDK_DIR are part
        # of graph.TOOLCHAIN_ID, which this edge declares.
        w.rule("compdb", command="$py -m homm1.graph.compdb --quiet",
               description="compdb", restat=True)
        w.build(COMPDB, "compdb", inputs=MANIFEST,
                implicit=COMPDB_MODS + [graph.TOOLCHAIN_ID])
        w.newline()

        w.comment("=== labels: source + headers + base obj -> per-unit claim fragment ===")
        # One TU's clang IR pass per edge (the expensive step), so a single
        # edit re-extracts only THAT unit. Fragments are written if-changed and
        # the rule restats, so an unchanged symbol set stops here and never
        # reaches model/delink. The header closure is independently required:
        # a header-only RVA claim can be renamed while cl emits no COMDAT in
        # this TU, leaving the object byte-identical and therefore restatted.
        w.rule("labels", command="$py -m homm1.retail_labels.source --unit $unit",
               description="labels $unit", restat=True)
        fragments = []
        for u in units:
            frag = f"{graph.CLAIMS_DIR}/{u['unit']}.tsv"
            fragments.append(frag)
            w.build(frag, "labels", inputs=u["source"],
                    implicit=[*headers_by_unit[u["unit"]],
                              f"{graph.BASE_DIR}/{u['unit']}.obj", MANIFEST,
                              COMPDB, *LABELS_MODS],
                    variables={"unit": u["unit"]})
        w.newline()

        w.comment("=== model: claims x censuses/providers -> bindings.tsv ===")
        w.rule("model", command="$py -m homm1.model", description="model",
               restat=True)
        w.build([graph.BINDINGS, graph.VIOLATIONS], "model", inputs=fragments,
                implicit=MODEL_TABLES + MODEL_MODS)
        w.newline()

        w.comment("=== delink: bindings -> synth pdb -> per-unit target objs ===")
        # Keyed on the bindings CONTENT: the delinker re-resolves the model
        # itself, so bindings.tsv is the fingerprint of everything that decides
        # the delink, and model writes it if-changed. A pure code edit never
        # reaches here. The declared output is a STAMP - units with no claim
        # produce no object, so declaring all of them would leave the edge
        # perpetually unbuilt and re-run the whole delink on every build.
        # NOT declared, and known: homm1.delink.{pdb_synth,data_manifest} also
        # read build/objdiff/base/*.obj (cl's own string/vtable/RTTI COMDATs),
        # so a code edit that moves those without moving a CLAIM does not
        # re-delink; and vostok-delinker itself is environment, not a file.
        w.rule("delink",
               command=(f"$py -m homm1.delink.run --target-dir {graph.TARGET_DIR} "
                        f"--delink-dir {graph.DELINK_RAW} && touch $out"),
               description="delink HEROES.EXE -> target objs")
        w.build(graph.DELINK_STAMP, "delink",
                inputs=[graph.BINDINGS, RETAIL_EXE],
                implicit=[RELOC_REFERENTS, FUNCTION_REFERENTS, *DELINK_MODS,
                          graph.TOOLCHAIN_ID])
        w.newline()

        w.comment("=== normalize: base + target -> content-addressed copies ===")
        # objdiff pairs BY NAME, so compiler-private data names ($SG/$T/$S),
        # weak externals and jump-table DIR32 labels are rewritten into
        # disposable side-by-side copies. The real objects are untouched, so
        # the transform is matching-NEUTRAL. One stamped edge drives the set;
        # the driver mtime-skips unchanged objects, so a single recompile
        # re-normalizes exactly one pair.
        w.rule("normalize",
               command=(f"$py -m homm1.compare.normalize --base-dir {graph.BASE_DIR} "
                        f"--target-dir {graph.TARGET_DIR} --out-dir {graph.COMPARE_DIR} "
                        f"--stamp $out"),
               description="normalize base/target objs")
        w.build(graph.NORMALIZE_STAMP, "normalize",
                inputs=base_objs + [graph.DELINK_STAMP],
                implicit=[MANIFEST, *NORMALIZE_MODS])
        w.newline()

        w.comment("=== project: the delinked directory -> objdiff.json ===")
        # AFTER the delink, because the pairing census is a DIRECTORY READ:
        # whether a unit pairs with its real target object or with the empty
        # dummy is read off what the delinker wrote, never predicted. Predicting
        # it is what once left two data-only units on the dummy - a pairing
        # objdiff scores 100.00% on every measure with zero totals.
        w.rule("project",
               command=(f"$py -m homm1.compare.project --target-dir {graph.TARGET_DIR} "
                        f"--out-dir {graph.COMPARE_DIR}"),
               description="project (pairing -> objdiff.json)", restat=True)
        w.build(graph.OBJDIFF_JSON, "project", inputs=[graph.DELINK_STAMP],
                implicit=[MANIFEST, *PROJECT_MODS])
        w.newline()

        w.comment("=== report: comparison copies + pairing -> report.json ===")
        # In-graph so it regenerates ONLY when an object or the pairing moved,
        # which is what lets `homm1 match` say "nothing rebuilt, nothing to
        # report" instead of re-scoring 311 units for a no-op build.
        w.rule("report",
               command=(f"$py -m homm1.tool.objdiff --project {graph.COMPARE_DIR} "
                        f"--out $out"),
               description="objdiff report")
        w.build(graph.REPORT_JSON, "report",
                inputs=[graph.NORMALIZE_STAMP, graph.OBJDIFF_JSON],
                implicit=REPORT_MODS)
        w.newline()

        w.comment("=== verify: fingerprints (beside compare) + the tiered "
                  "check (after) ===")
        # The fingerprint cache is BINDINGS x sources x clangd; it needs no
        # report, so ninja may schedule it alongside the compare leg - the
        # ordering that matters is fingerprints-before-CHECK, and the check
        # edge's inputs state it. The cache keeps the MAX gate's edit
        # detection honest (a stale cache degrades TOUCHED/REGRESS).
        w.rule("verify_fp", command="$py -m homm1.verify fingerprints",
               description="verify fingerprints", restat=True)
        w.build(FINGERPRINTS, "verify_fp",
                inputs=[u["source"] for u in units],
                implicit=[graph.BINDINGS, MANIFEST, COMPDB, *VERIFY_MODS])
        # The DEFAULT tiers only (fast+normal): the full/link tiers are
        # opt-in (`homm1 verify check --tier full`). A failing gate fails
        # the build - the gates are FATAL, and their committed baselines are
        # how known debt is carried.
        w.rule("verify_check",
               command="$py -m homm1.verify check && touch $out",
               description="verify check (MAX gate + fast+normal tiers)")
        w.build(VERIFY_STAMP, "verify_check",
                inputs=[graph.REPORT_JSON, FINGERPRINTS],
                implicit=[MANIFEST, *VERIFY_BASELINES, *VERIFY_MODS])
        w.newline()

        w.comment("=== aliases ===")
        w.build("base", "phony", inputs=base_objs)
        w.build("claims", "phony", inputs=fragments)
        w.build("target", "phony", inputs=[graph.DELINK_STAMP])
        w.build("compare", "phony", inputs=[graph.REPORT_JSON])
        w.build("verify", "phony", inputs=[VERIFY_STAMP])
        w.build("all", "phony",
                inputs=base_objs + [graph.BINDINGS, graph.DELINK_STAMP,
                                    graph.OBJDIFF_JSON, graph.REPORT_JSON,
                                    VERIFY_STAMP])
        w.default(["all"])
        w.newline()

        emit_link_phase(w, cl_edges)

    return len(units), pruned


def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="homm1 configure", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", type=Path, help=f"manifest path (default {graph.NINJA})")
    a = ap.parse_args(argv)
    try:
        n, pruned = emit(a.out)
    except OSError as e:
        print(f"[configure] cannot write {a.out or graph.NINJA}: {e}",
              file=sys.stderr)
        return 1
    if pruned:
        print(f"[configure] pruned {pruned} artifact(s) of unit(s) no longer "
              "in config/units.toml", file=sys.stderr)
    print(f"[configure] wrote {a.out or graph.NINJA} ({n} units)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

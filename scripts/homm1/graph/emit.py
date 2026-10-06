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
from homm1.core.inputs import targets
from homm1.graph import ninja_syntax
from homm1.graph.scan import Scanner
from types import SimpleNamespace

#: The game's artifact paths. Configure emits every image's edges, whatever
#: image the invoking command selected.
G = SimpleNamespace(**graph.image_paths("game"))

SCRIPTS = "scripts/homm1"
MANIFEST = "config/units.toml"
RETAIL_EXE = targets(REPO)["game"].destination.relative_to(REPO).as_posix()
COMPDB = "build/clangd/compile_commands.json"
RELOC_REFERENTS = "config/retail/reloc_referents.tsv"
FUNCTION_REFERENTS = "config/retail/function_referents.tsv"

#: The census + provider tables homm1.model joins the claims against. Named
#: rather than globbed: reloc_referents.tsv is a DELINKER input and belongs on
#: that edge, and a new table should be a deliberate edit here.
MODEL_TABLES = [
    FUNCTION_REFERENTS,
    "config/retail/functions.tsv", "config/retail/data.tsv",
    "config/retail/link_order.tsv", "config/retail/link_bands.tsv",
    "config/retail/functions_static_libs.tsv", "config/retail/data_vtables.tsv",
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
LOCALIZATION_MODS = _mods("graph/catalog.py", "graph/localization.py", "graph/scan.py") + sorted(
    str(p.relative_to(REPO)) for p in (REPO / "locales").glob("*")
    if p.suffix in (".pot", ".po", ".json")) + ["config/retail/targets.json"]
CL_MODS = LOCALIZATION_MODS + _mods("graph/cc.py", "tool/cl.py", "tool/fixedroot.py") + TOOL_MODS
ML_MODS = _mods("graph/fixed_asm.py", "tool/ml.py") + TOOL_MODS
COMPDB_MODS = LOCALIZATION_MODS + _mods("graph/compdb.py", "tool/clang.py", "manifest.py",
                    "core/paths.py")
LABELS_MODS = LOCALIZATION_MODS + _mods("retail_labels/", "tool/clang.py", "core/coff.py",
                    "core/tsv.py", "manifest.py", "core/paths.py", "core/msvc_names.py")
MODEL_MODS = _mods("model.py", "retail_labels/", "core/tsv.py", "core/paths.py")
DELINK_MODS = _mods("delink/", "tool/delinker.py", "core/pe.py",
                    "core/coff.py", "model.py", "core/data_matching.py",
                    "compare/runtime_aliases.py") + TOOL_MODS + ["config/compare.toml"]
NORMALIZE_MODS = _mods("compare/normalize.py", "compare/canonicalize.py",
                       "compare/runtime_aliases.py", "delink/eh_band.py", "core/coff.py", "core/msvc_names.py",
                       "core/data_matching.py") + ["config/compare.toml"]
PROJECT_MODS = _mods("compare/project.py", "compare/normalize.py", "manifest.py")
REPORT_MODS = _mods("tool/objdiff.py")
LINK_MODS = _mods("graph/link.py", "graph/implib.py", "delink/implib.py", "tool/link.py",
                  "core/pe.py") + TOOL_MODS + [
    "config/heroes.def", "config/retail/function_referents.tsv",
    "config/retail/import_libraries.tsv", "config/retail/import_symbols.json"]
VERIFY_MODS = _mods("verify/", "model.py", "core/tsv.py", "core/paths.py")
#: committed inputs of the default-tier verify gates (fast+normal): the MAX
#: ledger and every gate's own baseline/allowlist. Named so a bless re-runs
#: the check edge.
VERIFY_BASELINES = [
    "config/match_baseline.tsv",
    "config/link_diff.tsv",
    "config/cleanliness/cleanliness-text-baseline.tsv",
    "config/cleanliness/cleanliness-semantic-baseline.tsv",
    "config/cleanliness/tu-order-baseline.tsv",
    "config/cleanliness/data-tu-order-baseline.tsv",
    "config/cleanliness/kept-comdat-exiles.tsv",
]
FINGERPRINTS = "build/gen/func_fingerprints.tsv"
VERIFY_STAMP = "build/objdiff/.verify.stamp"
CONFIGURE_MODS = _mods("graph/", "manifest.py", "core/paths.py") + ["config/retail/asm_claims.tsv"]


# --------------------------------------------------------------------------- #
# manifest
# --------------------------------------------------------------------------- #
def load_units(image: str = "game") -> tuple[dict, list[dict]]:
    """(manifest, units) with each unit's `cflags` resolved from its profile.

    Every [[unit]] names ONE [flags] profile and the profile is the FULL flag
    set - there is no global default to inherit and no per-TU append, so a
    unit's flag choice stays one explicit, greppable name. A stray `extra` key
    is a hard error rather than a silent bolt-on.
    """
    from homm1.manifest import image_defines, load, unit_images
    data = load()
    profiles = data.get("flags", {})
    if not profiles:
        raise SystemExit(f"{MANIFEST}: [flags] must define at least one profile")
    from homm1.core.paths import images
    for u in data.get("unit", []):
        unknown = [i for i in unit_images(u) if i not in images()]
        if unknown:
            raise SystemExit(f"{MANIFEST}: unit '{u.get('unit')}' names unknown "
                             f"image(s) {unknown} (pinned: {images()})")
    units = [u for u in data.get("unit", []) if image in unit_images(u)]
    defines = image_defines(image)
    if not units and image == "game":
        raise SystemExit(f"{MANIFEST}: no [[unit]] entries")
    seen: set[str] = set()
    for u in units:
        for key in ("unit", "source", "flags"):
            if key not in u:
                raise SystemExit(f"{MANIFEST}: a [[unit]] is missing '{key}'")
        if u["unit"] in seen:
            raise SystemExit(f"{MANIFEST}: duplicate unit '{u['unit']}'")
        seen.add(u["unit"])
        # A unit another image compiles with another profile names it per image.
        profile = u.get("image_flags", {}).get(image, u["flags"])
        if profile not in profiles:
            raise SystemExit(f"{MANIFEST}: unit '{u['unit']}' image {image} references "
                             f"unknown flags profile '{profile}'")
        if u["flags"] not in profiles:
            raise SystemExit(f"{MANIFEST}: unit '{u['unit']}' references unknown "
                             f"flags profile '{u['flags']}' "
                             f"(defined: {sorted(profiles)})")
        if "extra" in u:
            raise SystemExit(
                f"{MANIFEST}: unit '{u['unit']}' sets 'extra' - per-TU flag "
                "bolt-ons are not supported. Add (or reuse) a [flags] profile "
                "carrying the FULL set instead.")
        u["compiler"] = u.get("compiler", data.get("build", {}).get(
            "compiler", "vc40"))
        if u["compiler"] not in {"vc40", "vc41", "vc6"}:
            raise SystemExit(f"{MANIFEST}: unit '{u['unit']}' has unsupported "
                             f"compiler '{u['compiler']}'")
        u["cflags"] = list(profiles[profile]) + defines
    return data, units


# --------------------------------------------------------------------------- #
# orphan artifacts
# --------------------------------------------------------------------------- #
#: (directory, "<prefix>{}<suffix>") pairs whose stems must be live units.
#: build/objdiff/target-new and build/delink/named are NOT listed: their
#: producers rmtree them, so they cannot hold an orphan.
_ORPHAN_PATTERNS = [
    (G.BASE_DIR, "{}.obj"),
    (f"{G.COMPARE_DIR}/base", "{}.obj"),
    (f"{G.COMPARE_DIR}/base", "{}.symbols.tsv"),
    (f"{G.COMPARE_DIR}/target", "{}.c.obj"),
    (f"{G.COMPARE_DIR}/target", "{}.symbols.tsv"),
    (G.CLAIMS_DIR, "{}.tsv"),
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
    stamp = REPO / G.DELINK_STAMP
    if stamp.exists():
        stamp.unlink()
        n += 1
    return n


# --------------------------------------------------------------------------- #
# the graph
# --------------------------------------------------------------------------- #
def era_rc_available() -> bool:
    """Whether the installed VC4 tree carries the pinned RC/CVTRES and the
    resource script exists, so the candidate can link its `.rsrc`."""
    try:
        from homm1 import toolchain
        from homm1.tool.rc import RESOURCE_TOOLCHAIN
        return ((REPO / graph.RESOURCE_SCRIPT).is_file()
                and toolchain.resources_installed(RESOURCE_TOOLCHAIN, msvc_dir()))
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
    """Emit the opt-in candidate link phase.

    MASM's COFF output belongs to objdiff.  The period linker consumes the
    ordinary OMF output, so replace only the
    fixed assembly objects on this edge.
    """
    w.comment("=== PHASE 2: link -> candidate HEROESW.EXE + .map (opt-in) ===")
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
        # The retail image supplies the icon (staged, never committed) and
        # is the payload gate.
        w.rule("rc", command=(f"$py -m homm1.tool.rc --out $out --src $in "
                              f"--verify-exe {RETAIL_EXE} "
                              f"--report {graph.RESOURCE_REPORT}"),
               description="rc $out")
        w.build(graph.RESOURCE_RES, "rc", inputs=graph.RESOURCE_SCRIPT,
                implicit=[RETAIL_EXE, graph.TOOLCHAIN_ID]
                         + _mods("tool/rc.py", "core/pe.py", "toolchain.py")
                         + LOCALIZATION_MODS + TOOL_MODS)
    else:
        w.comment("VC4 tree has no pinned RC.EXE/CVTRES.EXE; candidate links "
                  "without resources")
    res_flag = f" --res {graph.RESOURCE_RES}" if with_res else ""
    w.rule("link",
           command=(f"$py -m homm1.graph.link --out {graph.CANDIDATE_EXE} "
                    f"--objs-dir {G.BASE_DIR}{res_flag} $objects"),
           description="link candidate HEROESW.EXE + map")
    w.build([graph.CANDIDATE_EXE, graph.CANDIDATE_MAP], "link",
            inputs=link_objs,
            implicit=([graph.RESOURCE_RES] if with_res else [])
                     + [MANIFEST] + LINK_MODS,
            variables={"objects": " ".join(f"--obj {obj}" for obj in link_objs)})
    w.build("candidate", "phony",
            inputs=[graph.CANDIDATE_EXE, graph.CANDIDATE_MAP])
    w.newline()


#: Per-image retail tables a non-game image's model reads (under
#: config/retail/<image>/); absent tables are simply not declared.
IMAGE_MODEL_TABLES = ["functions.tsv", "data.tsv", "link_order.tsv", "link_bands.tsv",
                      "functions_static_libs.tsv", "data_vtables.tsv",
                      "data_static_libs.tsv", "data_compgen.tsv",
                      "function_referents.tsv", "placements.tsv"]
IMAGE_DELINK_TABLES = ["reloc_referents.tsv", "function_referents.tsv",
                       "absolute_relocations.tsv", "data_symbols.tsv"]


def emit_image_pipeline(w: ninja_syntax.Writer, image: str, scan: Scanner) -> list[str]:
    """Edges of one non-game image: its own compile of every unit it links,
    claims, model, delink, comparison and report under build/<image>/.

    The rules are the game's (`$py` is rebound per edge to select the image);
    only the directory-valued edges get image-suffixed rules. Returns the
    image's default outputs."""
    from homm1.core.paths import retail_dir, retail_exe
    from homm1.graph.fixed_asm import unit as fixed_asm_unit
    P = SimpleNamespace(**graph.image_paths(image))
    _manifest, units = load_units(image)
    retail = retail_dir(image).relative_to(REPO).as_posix()
    exe = retail_exe(image)
    exe = exe.relative_to(REPO).as_posix() if exe.is_relative_to(REPO) else str(exe)
    py = f"PYTHONPATH={REPO / 'scripts'} HOMM1_DIR={REPO} HOMM1_IMAGE={image} python3"
    tables = [f"{retail}/{t}" for t in IMAGE_MODEL_TABLES if (REPO / retail / t).is_file()]
    delink_tables = [f"{retail}/{t}" for t in IMAGE_DELINK_TABLES
                     if (REPO / retail / t).is_file()]

    w.comment(f"=== image {image}: {len(units)} unit(s) under {P.BASE_DIR} ===")
    objs, fragments = [], []
    for u in units:
        obj = f"{P.BASE_DIR}/{u['unit']}.obj"
        objs.append(obj)
        headers = scan.headers(u["source"])
        assembly = fixed_asm_unit(u["unit"], u["source"])
        if assembly is not None:
            w.build(obj, "ml_coff", inputs=u["source"],
                    implicit=ML_MODS + [graph.TOOLCHAIN_ID],
                    variables={"unit": u["unit"], "py": py})
        else:
            w.build(obj, "cl", inputs=u["source"],
                    implicit=headers + CL_MODS + [graph.TOOLCHAIN_ID],
                    variables={"unit": u["unit"], "py": py,
                               "cflags": " ".join(u["cflags"])})
        frag = f"{P.CLAIMS_DIR}/{u['unit']}.tsv"
        fragments.append(frag)
        w.build(frag, "labels", inputs=u["source"],
                implicit=[*headers, obj, MANIFEST, P.COMPDB, *LABELS_MODS,
                          *(["config/retail/asm_claims.tsv",
                             f"{SCRIPTS}/graph/fixed_asm.py"]
                            if assembly is not None else [])],
                variables={"unit": u["unit"], "py": py})
    w.build(P.COMPDB, "compdb", inputs=MANIFEST,
            implicit=COMPDB_MODS + [graph.TOOLCHAIN_ID], variables={"py": py})
    w.build([P.BINDINGS, P.VIOLATIONS], "model", inputs=fragments,
            implicit=tables + MODEL_MODS, variables={"py": py})

    w.rule(f"delink_{image}",
           command=(f"{py} -m homm1.delink.run --target-dir {P.TARGET_DIR} "
                    f"--delink-dir {P.DELINK_RAW} && touch $out"),
           description=f"delink {image} -> target objs")
    w.build(P.DELINK_STAMP, f"delink_{image}", inputs=[P.BINDINGS, exe],
            implicit=[*delink_tables, *DELINK_MODS, graph.TOOLCHAIN_ID])
    w.rule(f"normalize_{image}",
           command=(f"{py} -m homm1.compare.normalize --base-dir {P.BASE_DIR} "
                    f"--target-dir {P.TARGET_DIR} --out-dir {P.COMPARE_DIR} "
                    f"--stamp $out"),
           description=f"normalize {image} base/target objs")
    w.build(P.NORMALIZE_STAMP, f"normalize_{image}", inputs=objs + [P.DELINK_STAMP],
            implicit=[MANIFEST, *NORMALIZE_MODS, graph.TOOLCHAIN_ID])
    w.rule(f"project_{image}",
           command=(f"{py} -m homm1.compare.project --target-dir {P.TARGET_DIR} "
                    f"--out-dir {P.COMPARE_DIR}"),
           description=f"project {image} (pairing -> objdiff.json)", restat=True)
    w.build(P.OBJDIFF_JSON, f"project_{image}", inputs=[P.DELINK_STAMP],
            implicit=[MANIFEST, *PROJECT_MODS])
    w.rule(f"report_{image}",
           command=f"{py} -m homm1.tool.objdiff --project {P.COMPARE_DIR} --out $out",
           description=f"objdiff report {image}")
    w.build(P.REPORT_JSON, f"report_{image}", inputs=[P.NORMALIZE_STAMP, P.OBJDIFF_JSON],
            implicit=REPORT_MODS)
    outputs = objs + [P.BINDINGS, P.DELINK_STAMP, P.OBJDIFF_JSON, P.REPORT_JSON]
    w.build(image, "phony", inputs=outputs)
    w.newline()
    return outputs


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
    cl_edges = [(f"{G.BASE_DIR}/{u['unit']}.obj", u["source"],
                 scan.headers(u["source"]), u["cflags"], u["unit"],
                 fixed_asm_unit(u["unit"], u["source"]))
                for u in units]
    base_objs = [e[0] for e in cl_edges]
    # Other images' sources join the generator edge's scanned set too.
    from homm1.core.paths import images as pinned_images
    for image in pinned_images():
        if image != "game":
            for u in load_units(image)[1]:
                scan.headers(u["source"])
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
        # Another image's table set decides which tables its edges declare,
        # so a table appearing in config/retail/<image>/ re-emits the graph.
        from homm1.core.paths import retail_dir
        image_dirs = [retail_dir(i).relative_to(REPO).as_posix()
                      for i in pinned_images() if i != "game" and retail_dir(i).is_dir()]
        w.build(graph.NINJA, "configure",
                implicit=[MANIFEST, *CONFIGURE_MODS, *sorted(scan.scanned()), *image_dirs])
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
            frag = f"{G.CLAIMS_DIR}/{u['unit']}.tsv"
            fragments.append(frag)
            w.build(frag, "labels", inputs=u["source"],
                    implicit=[*headers_by_unit[u["unit"]],
                              f"{G.BASE_DIR}/{u['unit']}.obj", MANIFEST,
                              COMPDB, *LABELS_MODS,
                              *(["config/retail/asm_claims.tsv",
                                 f"{SCRIPTS}/graph/fixed_asm.py"]
                                if fixed_asm_unit(u["unit"]) else [])],
                    variables={"unit": u["unit"]})
        w.newline()

        w.comment("=== model: claims x censuses/providers -> bindings.tsv ===")
        w.rule("model", command="$py -m homm1.model", description="model",
               restat=True)
        w.build([G.BINDINGS, G.VIOLATIONS], "model", inputs=fragments,
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
               command=(f"$py -m homm1.delink.run --target-dir {G.TARGET_DIR} "
                        f"--delink-dir {G.DELINK_RAW} && touch $out"),
               description="delink HEROESW.EXE -> target objs")
        w.build(G.DELINK_STAMP, "delink",
                inputs=[G.BINDINGS, RETAIL_EXE],
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
               command=(f"$py -m homm1.compare.normalize --base-dir {G.BASE_DIR} "
                        f"--target-dir {G.TARGET_DIR} --out-dir {G.COMPARE_DIR} "
                        f"--stamp $out"),
               description="normalize base/target objs")
        w.build(G.NORMALIZE_STAMP, "normalize",
                inputs=base_objs + [G.DELINK_STAMP],
                # OLDNAMES/LIBCMT alias records come from the pinned toolchain.
                implicit=[MANIFEST, *NORMALIZE_MODS, graph.TOOLCHAIN_ID])
        w.newline()

        w.comment("=== project: the delinked directory -> objdiff.json ===")
        # AFTER the delink, because the pairing census is a DIRECTORY READ:
        # whether a unit pairs with its real target object or with the empty
        # dummy is read off what the delinker wrote, never predicted. Predicting
        # it is what once left two data-only units on the dummy - a pairing
        # objdiff scores 100.00% on every measure with zero totals.
        w.rule("project",
               command=(f"$py -m homm1.compare.project --target-dir {G.TARGET_DIR} "
                        f"--out-dir {G.COMPARE_DIR}"),
               description="project (pairing -> objdiff.json)", restat=True)
        w.build(G.OBJDIFF_JSON, "project", inputs=[G.DELINK_STAMP],
                implicit=[MANIFEST, *PROJECT_MODS])
        w.newline()

        w.comment("=== report: comparison copies + pairing -> report.json ===")
        # In-graph so it regenerates ONLY when an object or the pairing moved,
        # which is what lets `homm1 match` say "nothing rebuilt, nothing to
        # report" instead of re-scoring 311 units for a no-op build.
        w.rule("report",
               command=(f"$py -m homm1.tool.objdiff --project {G.COMPARE_DIR} "
                        f"--out $out"),
               description="objdiff report")
        w.build(G.REPORT_JSON, "report",
                inputs=[G.NORMALIZE_STAMP, G.OBJDIFF_JSON],
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
                implicit=[G.BINDINGS, MANIFEST, COMPDB, *VERIFY_MODS])
        w.rule("verify_readme", command="$py -m homm1.verify readme && touch $out",
               description="refresh README score block")
        from homm1.manifest import all_units as _all, unit_images as _images
        image_reports = [graph.image_paths(i)["REPORT_JSON"] for i in pinned_images()
                         if i != "game" and any(i in _images(u) for u in _all())]
        w.build("build/objdiff/.readme.stamp", "verify_readme",
                inputs=[G.REPORT_JSON, FINGERPRINTS, "README.md", *image_reports],
                # the universe's carve-out classes (library/compiler/thunk)
                implicit=[MANIFEST, "config/retail/dna_bands.tsv",
                          *VERIFY_BASELINES, *VERIFY_MODS])
        # The build refreshes scores and README.
        # Merge preparation explicitly runs `homm1 build verify`; all gates
        # remain fatal there, including MAX and the fast+normal tiers.
        w.rule("verify_check",
               command="$py -m homm1.verify check --no-readme && touch $out",
               description="verify check (MAX gate + fast+normal tiers)")
        # link-diff compares the linked candidate with its banked ceiling.
        w.build(VERIFY_STAMP, "verify_check",
                inputs=[G.REPORT_JSON, FINGERPRINTS],
                # the fast tier's localization gate reads the catalogs
                implicit=[MANIFEST, graph.CANDIDATE_EXE, *VERIFY_BASELINES,
                          *VERIFY_MODS, *LOCALIZATION_MODS])
        w.newline()

        image_outputs = []
        from homm1.core.paths import images as pinned_images
        from homm1.manifest import all_units, unit_images
        for image in pinned_images():
            if image != "game" and any(image in unit_images(u) for u in all_units()):
                image_outputs += emit_image_pipeline(w, image, scan)

        w.comment("=== aliases ===")
        w.build("base", "phony", inputs=base_objs)
        w.build("claims", "phony", inputs=fragments)
        w.build("target", "phony", inputs=[G.DELINK_STAMP])
        w.build("compare", "phony", inputs=[G.REPORT_JSON])
        w.build("verify", "phony", inputs=[VERIFY_STAMP])
        w.build("all", "phony",
                inputs=base_objs + [G.BINDINGS, G.DELINK_STAMP,
                                    G.OBJDIFF_JSON, G.REPORT_JSON,
                                    FINGERPRINTS, "build/objdiff/.readme.stamp",
                                    *image_outputs])
        w.default(["all"])
        w.newline()

        emit_link_phase(w, cl_edges)

    return len(units), pruned


from homm1.core.usage import logged


@logged
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

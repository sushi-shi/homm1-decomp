"""homm1.delink.run - the delink step, end to end.

    python3 -m homm1.delink.run [--target-dir build/objdiff/target-new]

    Model -> build/pdb/homm1_named.{yaml,pdb}
          -> build/gen/delink_data_manifest.tsv (+ section manifest)
          -> vostok-delinker -> build/delink/named/
          -> collect <unit>.c.obj for every claimed unit into --target-dir

The address-bucketed seg_NNNN.cpp.obj for the un-named .text remainder stay in
the raw delink dir and are not collected.
"""

from __future__ import annotations

from pathlib import Path

from homm1.core.paths import BUILD, RETAIL
from homm1.delink import data_manifest, pdb_synth
from homm1.model import Model, resolve

DELINK_DIR = BUILD / "delink/named"
TARGET_DIR = BUILD / "objdiff/target-new"
RELOC_ALIASES = RETAIL / "reloc_referents.tsv"


def units(model: Model) -> list[str]:
    """The unit stems to collect a <unit>.c.obj for: the source unit census
    (extraction's per-TU fragment cache), falling back to the claimed units.
    A unit whose only claims are data still gets a (data-only) object."""
    from homm1.retail_labels.fragments import FRAGMENTS
    if FRAGMENTS.is_dir():
        stems = sorted(p.stem for p in FRAGMENTS.glob("*.tsv"))
        if stems:
            return stems
    return sorted({b.unit for b in model.functions + model.data
                   if b.channel in (*pdb_synth.UNIT_CHANNELS, "src") and b.unit})


def run(model: Model | None = None, target_dir: Path = TARGET_DIR,
        delink_dir: Path = DELINK_DIR) -> dict:
    import shutil
    model = model or resolve()

    synth = pdb_synth.synth(model)
    data_manifest.generate(model)

    from homm1.tool import delinker
    out = delinker.delink(
        synth["pdb"], pdb_synth.retail().pe.path, delink_dir,
        data_manifest=data_manifest.OUTPUT,
        data_section_manifest=data_manifest.SECTION_OUTPUT,
        reloc_alias_manifest=RELOC_ALIASES)
    if out.strip():
        print(out.strip().splitlines()[-1])

    wanted = units(model)
    if target_dir.exists():
        shutil.rmtree(target_dir)
    target_dir.mkdir(parents=True, exist_ok=True)
    collected, missing = [], []
    for unit in wanted:
        src = delink_dir / f"{unit}.c.obj"
        if src.exists():
            shutil.copy2(src, target_dir / f"{unit}.c.obj")
            collected.append(unit)
        else:
            missing.append(unit)
    print(f"[delink] collected {len(collected)}/{len(wanted)} unit obj(s) "
          f"-> {target_dir}")
    if missing:
        print(f"[delink]   no named functions yet for: {', '.join(missing)}")
    return {"collected": collected, "missing": missing}


#: vostok-delinker messages whose text names a SYMPTOM, with the operational
#: cause that actually produces them. Keyed on a fragment of the tool's own
#: output; the hint is appended, the tool's words are never replaced.
_DELINKER_HINTS = (
    ("relocation alias owner is absent",
     "this is what a STALE vostok-delinker on $PATH prints: the binary "
     "predates the reloc-alias manifest schema this tree writes. Check "
     "`which vostok-delinker` against the flake's own "
     "`result/bin/vostok-delinker`, or re-enter `nix develop`."),
    ("missing manifest",
     "the manifests are delink's own inputs - run `homm1 build` (or "
     "`python3 -m homm1.delink.data_manifest`) first."),
    ("No such file or directory",
     "vostok-delinker is not on $PATH - run inside `nix develop`."),
)


def main(argv=None) -> int:
    import argparse
    import sys

    from homm1.tool import ToolError
    ap = argparse.ArgumentParser(
        prog="homm1 delink", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--target-dir", type=Path, default=TARGET_DIR,
                    help="where the collected <unit>.c.obj land "
                         f"(default: {TARGET_DIR})")
    ap.add_argument("--delink-dir", type=Path, default=DELINK_DIR,
                    help="the delinker's raw output dir "
                         f"(default: {DELINK_DIR})")
    a = ap.parse_args(argv)
    try:
        run(target_dir=a.target_dir, delink_dir=a.delink_dir)
    except (ToolError, OSError) as e:
        print(f"[delink] {e}", file=sys.stderr)
        for needle, hint in _DELINKER_HINTS:
            if needle in str(e):
                print(f"[delink] {hint}", file=sys.stderr)
                break
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

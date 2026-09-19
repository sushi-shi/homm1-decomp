"""homm1.tool.delinker - vostok-delinker (native).

    homm1 tool delinker --pdb <pdb> --exe <exe> --out <dir> \
        --data-manifest <tsv> --data-section-manifest <tsv> \
        --reloc-alias-manifest <tsv>

In-process:
    from homm1.tool import delinker
    delinker.delink(pdb, exe, out_dir, data_manifest=..., ...)

Slices the retail EXE into per-symbol COFF objects under `out_dir`, driven by
the synthesized PDB and the reviewed data/section/reloc manifests. The output
dir is wiped first (the delinker assumes a clean target). The engine path is
the synthetic `c:\\proj\\` root pdb_synth attributes sources under.
"""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

from homm1.tool import ToolError

ENGINE_PATH = "c:\\proj\\"
DELINKER = "vostok-delinker"


def delink(pdb: Path | str, exe: Path | str, out_dir: Path | str, *,
           data_manifest: Path | str | None = None,
           data_section_manifest: Path | str | None = None,
           reloc_alias_manifest: Path | str | None = None,
           recover_data_relocs_from_pdb: bool = True,
           timeout: float | None = 1800) -> str:
    """Run vostok-delinker; returns its output. Raises ToolError on failure."""
    pdb, exe, out_dir = Path(pdb), Path(exe), Path(out_dir)
    if shutil.which(DELINKER) is None:
        raise ToolError(f"{DELINKER} not found on PATH - run inside "
                        "`nix develop` (or put the flake's own "
                        "`result/bin` first)")
    for f in (pdb, exe):
        if not f.exists():
            raise ToolError(f"missing input: {f}")
    if out_dir.exists():
        shutil.rmtree(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    argv = [DELINKER,
            "--pdb-path", str(pdb),
            "--exe-path", str(exe),
            "--output-path", str(out_dir),
            "--engine-path", ENGINE_PATH]
    for flag, value in (("--data-manifest", data_manifest),
                        ("--data-section-manifest", data_section_manifest),
                        ("--reloc-alias-manifest", reloc_alias_manifest)):
        if value is not None:
            if not Path(value).exists():
                raise ToolError(f"missing manifest: {value}")
            argv += [flag, str(value)]
    if recover_data_relocs_from_pdb:
        # Safety net only: keeps the permissive nearest-PDB-symbol recovery for
        # any writable RVA the manifest does not enroll, instead of a hard
        # delink failure (measured byte-identical when the manifest is whole).
        argv.append("--recover-data-relocs-from-pdb")

    try:
        r = subprocess.run(argv, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired as e:
        raise ToolError(f"{DELINKER} did not finish within {timeout}s over "
                        f"{exe} - the output dir {out_dir} is incomplete") from e
    if r.returncode != 0:
        tail = "\n".join((r.stderr or r.stdout).strip().splitlines()[-15:])
        raise ToolError(f"{DELINKER} failed (rc={r.returncode}):\n{tail}")
    return (r.stdout or "") + (r.stderr or "")


def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--pdb", required=True)
    ap.add_argument("--exe", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--data-manifest")
    ap.add_argument("--data-section-manifest")
    ap.add_argument("--reloc-alias-manifest")
    a = ap.parse_args()
    try:
        out = delink(a.pdb, a.exe, a.out,
                     data_manifest=a.data_manifest,
                     data_section_manifest=a.data_section_manifest,
                     reloc_alias_manifest=a.reloc_alias_manifest)
        if out.strip():
            print(out)
    except (ToolError, OSError) as e:
        print(f"[delinker] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

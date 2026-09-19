"""homm1.tool.pdbutil - llvm-pdbutil (native).

    homm1 tool pdbutil --pdb <out.pdb> --yaml <in.yaml>     (yaml2pdb)

In-process:
    from homm1.tool import pdbutil
    pdbutil.yaml2pdb(yaml_path, pdb_path)
    text = pdbutil.dump(pdb_path, "--streams")

Runs the program and returns artifacts/stdout; interpreting the dump (e.g.
finding an empty stream to repoint the DBI at) is the caller's business.
"""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

from homm1.tool import ToolError

PDBUTIL = "llvm-pdbutil"


def _run(argv: list[str], timeout: float | None) -> subprocess.CompletedProcess:
    """Spawn llvm-pdbutil; an absent binary or a stall is a ToolError, not a
    traceback out of subprocess."""
    if shutil.which(PDBUTIL) is None:
        raise ToolError(f"{PDBUTIL} not found on PATH - run inside "
                        "`nix develop`")
    try:
        return subprocess.run(argv, capture_output=True, text=True,
                              timeout=timeout)
    except subprocess.TimeoutExpired as e:
        raise ToolError(f"{PDBUTIL} {argv[1]} did not finish within "
                        f"{timeout}s") from e


def yaml2pdb(yaml_path: Path | str, pdb_path: Path | str,
             timeout: float | None = 300) -> None:
    """Produce `pdb_path` from a yaml2pdb description. Raises ToolError."""
    yaml_path, pdb_path = Path(yaml_path), Path(pdb_path)
    if not yaml_path.exists():
        raise ToolError(f"yaml missing: {yaml_path}")
    pdb_path.parent.mkdir(parents=True, exist_ok=True)
    pdb_path.unlink(missing_ok=True)
    r = _run([PDBUTIL, "yaml2pdb", f"--pdb={pdb_path}", str(yaml_path)], timeout)
    if r.returncode != 0 or not pdb_path.exists():
        tail = "\n".join((r.stderr or r.stdout).strip().splitlines()[-12:])
        raise ToolError(f"yaml2pdb failed (rc={r.returncode}):\n{tail}")


def dump(pdb_path: Path | str, *flags: str,
         timeout: float | None = 120) -> str:
    """`llvm-pdbutil dump <flags> <pdb>` stdout. Raises ToolError on failure."""
    r = _run([PDBUTIL, "dump", *flags, str(pdb_path)], timeout)
    if r.returncode != 0:
        tail = "\n".join((r.stderr or r.stdout).strip().splitlines()[-12:])
        raise ToolError(f"{PDBUTIL} dump failed (rc={r.returncode}):\n{tail}")
    return r.stdout


from homm1.core.usage import logged


@logged
def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--pdb", required=True)
    ap.add_argument("--yaml", required=True)
    a = ap.parse_args()
    try:
        yaml2pdb(a.yaml, a.pdb)
    except (ToolError, OSError) as e:
        print(f"[pdbutil] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

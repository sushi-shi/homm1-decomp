"""homm1.core.paths - repo and toolchain path discovery, once.

REPO resolves from the CWD first, then this file's ancestors: in a worktree the
shell's PYTHONPATH can point at main's scripts/, so __file__ alone would
mis-resolve to main.
"""

from __future__ import annotations

import os
from pathlib import Path


def _find_repo() -> Path:
    for base in (Path.cwd(), Path(__file__).resolve().parent):
        for p in (base, *base.parents):
            if (p / "flake.nix").exists():
                return p
    raise RuntimeError("not inside a homm1 repo (no flake.nix upward of cwd)")


REPO = _find_repo()
SRC = REPO / "src"
INCLUDE = REPO / "include"
VENDOR = REPO / "vendor"
CONFIG = REPO / "config"
RETAIL = CONFIG / "retail"
BUILD = REPO / "build"


def sdk_names() -> list[str]:
    """The pinned vendor SDKs (config/toolchains.json entries with `sdk`)."""
    import json
    pins = json.loads((CONFIG / "toolchains.json").read_text())
    return sorted(name for name, entry in pins.items() if "sdk" in entry)


def vendor_include_dirs(vendor: Path = VENDOR) -> list[tuple[str, Path]]:
    """[(name, dir)] of every vendor header tree, in one fixed order: the
    in-repo vendor/<name> trees, then each pinned SDK's installed include/
    (build/toolchains/<sdk>/include; `homm1 toolchain install --id <sdk>`).
    `vendor` selects another tree's vendor/ (the generated clean tree)."""
    dirs = [(d.name, d) for d in sorted(vendor.iterdir()) if d.is_dir()] \
        if vendor.is_dir() else []
    for name in sdk_names():
        inc = BUILD / "toolchains" / name / "include"
        if inc.is_dir():
            dirs.append((name, inc))
    return dirs


def sdk_lib_dirs() -> list[Path]:
    """Each pinned SDK's installed lib/ directory."""
    return [d for d in (BUILD / "toolchains" / n / "lib" for n in sdk_names()) if d.is_dir()]


def msvc_dir() -> Path:
    """The installed VC4 tree, overridable for compiler probes."""
    return Path(os.environ.get("MSVC_DIR") or BUILD / "toolchains/vc40")


def dxsdk_dir() -> Path:
    v = os.environ.get("DXSDK_DIR")
    if not v:
        raise RuntimeError("$DXSDK_DIR unset - run inside `nix develop`")
    return Path(v)


def retail_exe() -> Path:
    return Path(os.environ.get("HOMM1_EXE") or BUILD / "orig/HEROES.EXE")

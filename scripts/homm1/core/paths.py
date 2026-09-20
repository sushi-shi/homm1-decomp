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


def msvc_dir() -> Path:
    """The installed VC4 tree, overridable for compiler probes."""
    return Path(os.environ.get("MSVC_DIR") or BUILD / "toolchains/vc40")


def watcom_dir() -> Path:
    """The installed September 1994 Watcom C/C++ 10.0a tree."""
    return Path(os.environ.get("WATCOM_DIR") or BUILD / "toolchains/watcom10")


def dxsdk_dir() -> Path:
    v = os.environ.get("DXSDK_DIR")
    if not v:
        raise RuntimeError("$DXSDK_DIR unset - run inside `nix develop`")
    return Path(v)


def retail_exe() -> Path:
    return Path(os.environ.get("HOMM1_EXE") or BUILD / "orig/HEROES.EXE")

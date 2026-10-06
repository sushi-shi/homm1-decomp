"""homm1.core.paths - repo and toolchain path discovery, once.

REPO resolves from the CWD first, then this file's ancestors: in a worktree the
shell's PYTHONPATH can point at main's scripts/, so __file__ alone would
mis-resolve to main.

IMAGES. The repository reconstructs more than one linked program
(config/retail/targets.json: `game` = HEROES.EXE, `editor` = EDITOR.EXE).
Every authoritative fact is keyed by (image, rva): the selected image
(`homm1 --image KEY`, exported to children as $HOMM1_IMAGE; default `game`)
decides which retail executable, which config/retail table set and which
generated model/delink/compare trees a command reads. The game keeps its
historical paths; another image's state lives under config/retail/<image>/ and
build/<image>/ (objects included). A unit compiles once per image it links
into, because an image's compile context (its source-path strings) is part of
the object.
`RETAIL` and `IMAGE_BUILD` resolve lazily to the selected image's table
directory and generated-state root; `RETAIL_ROOT` and `BUILD` are shared.
"""

from __future__ import annotations

import os
import tomllib
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
RETAIL_ROOT = CONFIG / "retail"
BUILD = REPO / "build"

#: Environment variable carrying the selected image to every child process.
IMAGE_ENV = "HOMM1_IMAGE"
DEFAULT_IMAGE = "game"


def images() -> list[str]:
    """The pinned image keys, in targets.json order (the game first)."""
    import json
    return list(json.loads((RETAIL_ROOT / "targets.json").read_text()))


def image_key() -> str:
    """The selected image; an unknown key is an error, never a fallback."""
    key = os.environ.get(IMAGE_ENV) or DEFAULT_IMAGE
    if key not in images():
        raise RuntimeError(f"${IMAGE_ENV}={key!r} is not a pinned image "
                           f"(config/retail/targets.json: {images()})")
    return key


def retail_dir(image: str | None = None) -> Path:
    """config/retail for the game; config/retail/<image> for another image."""
    key = image or image_key()
    return RETAIL_ROOT if key == DEFAULT_IMAGE else RETAIL_ROOT / key


def image_build(image: str | None = None) -> Path:
    """Root of the image's generated state: build/ for the game,
    build/<image>/ for another image (toolchains and objects stay shared)."""
    key = image or image_key()
    return BUILD if key == DEFAULT_IMAGE else BUILD / key


def gen_dir(image: str | None = None) -> Path:
    return image_build(image) / "gen"


def objdiff_dir(image: str | None = None) -> Path:
    """Delinked targets, comparison copies and reports of one image."""
    return image_build(image) / "objdiff"


def delink_dir(image: str | None = None) -> Path:
    return image_build(image) / "delink"


def __getattr__(name: str):
    # `from homm1.core.paths import RETAIL` binds the selected image's table
    # directory (IMAGE_BUILD: its generated-state root) at the importer's
    # import time, after the CLI has selected the image.
    if name == "RETAIL":
        return retail_dir()
    if name == "IMAGE_BUILD":
        return image_build()
    raise AttributeError(name)


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


def compiler_id() -> str:
    """The compiler selected by the target's build contract."""
    return tomllib.loads((CONFIG / "units.toml").read_text())["build"]["compiler"]


def msvc_dir() -> Path:
    """The selected MSVC tree, overridable for compiler controls."""
    return Path(os.environ.get("MSVC_DIR") or BUILD / "toolchains" / compiler_id())


def dxsdk_dir() -> Path:
    v = os.environ.get("DXSDK_DIR")
    if not v:
        raise RuntimeError("$DXSDK_DIR unset - run inside `nix develop`")
    return Path(v)


def retail_exe(image: str | None = None) -> Path:
    from homm1.core.inputs import targets
    pin = targets(REPO)[image or image_key()]
    return Path(os.environ.get(pin.env_var) or pin.destination)

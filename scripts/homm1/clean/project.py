"""The generated tree's standalone build inputs.

`build.json` describes each program the tree builds (`targets`: the game's
HEROES.EXE and the scenario editor's EDITOR.EXE) with its units, each unit's
VC6 profile and the image's defines (`/DHOMM1_EDITOR` for every editor
compile), its resource script, and the candidate link contract of
`homm1.graph.link`: objects in retail code order, the BASE units archived
into the library LINK searches after audiere.lib, the library line and flags.
The game's order is each unit's lowest claimed function address, read from
the annotations before they are removed; the editor's is its reviewed
`config/retail/editor/link_order.tsv` (a shared unit's claims spell game
addresses). `imports/` holds the stub-DLL sources `homm1.graph.implib`
derives from the retail import table for the vendor DLLs whose SDKs ship no
import library; the editor imports only audiere's, a subset of the game's.
Only names, ordinals and order are carried; no address reaches the tree.
"""

from __future__ import annotations

import json
import re

from homm1.clean import source

IMAGE_BASE = 0x400000
TEMPLATE = "scripts/homm1/clean/template/"
#: The game runner shared with `homm1 play`; the source tree carries it as play.py.
RUNNER = "scripts/homm1/graph/play.py"
EXECUTABLE = ("build.py", "play.py")
#: Each image's resource script.
RESOURCES = {"game": "src/SOURCE/Heroes.rc", "editor": "src/EDITOR/Editor.rc"}
#: EDITOR.EXE's library line: its import descriptors read KERNEL32, USER32,
#: GDI32, ADVAPI32, WING32 and audiere (the game's line without WINMM, mss32,
#: smackw32 and NETAPI32), with OLDNAMES first and msvcprt (operator delete)
#: after the import libraries, as in the game. It keeps LINK's default stack.
EDITOR_LINK_LIBS = ["oldnames.lib", "kernel32.lib", "user32.lib", "gdi32.lib",
                    "advapi32.lib", "wing32.lib", "audiere.lib", "msvcprt.lib"]


def _units(files: dict[str, bytes], image: str = "game") -> list[dict]:
    """The units `image` links, in manifest order."""
    import tomllib
    from homm1.manifest import unit_images
    return [u for u in tomllib.loads(files["config/units.toml"].decode())["unit"]
            if image in unit_images(u)]


def images(files: dict[str, bytes]) -> list[str]:
    """The snapshot's pinned programs that link at least one unit (game first)."""
    import tomllib
    from homm1.manifest import unit_images
    config = tomllib.loads(files["config/units.toml"].decode())
    linked = {image for unit in config["unit"] for image in unit_images(unit)}
    return [key for key in json.loads(files["config/retail/targets.json"]) if key in linked]


def first_function(files: dict[str, bytes], unit: dict) -> int | None:
    """The unit's lowest claimed function RVA (dynamic initializers excluded),
    as `homm1.graph.link.first_claimed_rva` reads it from the claims."""
    from homm1.graph.fixed_asm import unit as fixed_asm_unit
    fixed = fixed_asm_unit(unit["unit"], unit["source"])
    if fixed is not None:
        addresses = [claim.va for claim in fixed.claims if claim.kind == "func"]
    else:
        text = source.strip_comments(files[unit["source"]].decode())
        addresses = [int(m.group(1), 16) for m in
                     re.finditer(r"\bVA(?:_COMPGEN)?\s*\(\s*(0x[0-9A-Fa-f]+)", text)]
    return min(addresses) - IMAGE_BASE if addresses else None


def image_starts(files: dict[str, bytes], image: str) -> dict[str, int]:
    """{unit: lowest code span start} from another image's reviewed
    link_order.tsv."""
    table = files[f"config/retail/{image}/link_order.tsv"].decode()
    rows = [line.split("\t") for line in table.splitlines() if line and not line.startswith("#")]
    header, rows = rows[0], rows[1:]
    column = {name: index for index, name in enumerate(header)}
    starts: dict[str, int] = {}
    for row in rows:
        unit, start = row[column["unit"]], int(row[column["lo"]], 16)
        starts[unit] = min(start, starts.get(unit, start))
    return starts


def _profile(config: dict, unit: dict, image: str) -> list[str]:
    """The unit's full compile flags for `image`: its profile (per image when
    `image_flags` names one) and the image's defines."""
    name = unit.get("image_flags", {}).get(image, unit["flags"])
    defines = config.get("images", {}).get(image, {}).get("defines", [])
    return [*config["flags"][name], *(f"/D{define}" for define in defines)]


def target(files: dict[str, bytes], image: str) -> dict:
    """One program's build and link contract."""
    import tomllib
    from homm1.graph.link import (BASE_LIBRARY, BASE_LIBRARY_AFTER, BASE_LIBRARY_FROM,
                                  CRT_LIBRARY, CRT_REPLACES, LINK_LIBS, LINK_RETAIL_FLAGS)
    config = tomllib.loads(files["config/units.toml"].decode())
    pins = json.loads(files["config/retail/targets.json"])
    units = _units(files, image)
    starts = image_starts(files, image) if image != "game" else {}
    keyed = []
    for index, unit in enumerate(units):
        rva = first_function(files, unit) if image == "game" else starts.get(unit["unit"])
        if rva is None:
            raise ValueError(f"{image}: {unit['unit']}: no claimed function orders it in the link")
        keyed.append((rva, index, unit["unit"], unit["source"]))
    ordered = sorted(keyed)
    if image == "game":
        library_from = BASE_LIBRARY_FROM
        libraries = [*LINK_LIBS, CRT_LIBRARY]
        stack = ["/STACK:0x10240,0x1000"]
    else:
        # LINK places library members after every object, so the BASE library
        # begins at the first BASE unit of the retail code order.
        library_from = min(rva for rva, _i, _u, path in ordered if path.startswith("src/BASE/"))
        libraries = [*EDITOR_LINK_LIBS, CRT_LIBRARY]
        stack = []
    if RESOURCES[image] not in files:
        raise ValueError(f"{image}: no resource script {RESOURCES[image]}")
    return {
        "executable": pins[image]["name"],
        "resources": RESOURCES[image],
        "defines": list(config.get("images", {}).get(image, {}).get("defines", [])),
        "units": [{"unit": u["unit"], "source": u["source"],
                   "flags": _profile(config, u, image)} for u in units],
        "link": {
            "objects": [name for rva, _i, name, _p in ordered if rva < library_from],
            "members": [name for rva, _i, name, _p in ordered if rva >= library_from],
            "library": BASE_LIBRARY,
            "library_after": BASE_LIBRARY_AFTER,
            "libraries": libraries,
            "flags": ["/SUBSYSTEM:WINDOWS", "/BASE:0x400000", "/INCREMENTAL:NO",
                      *LINK_RETAIL_FLAGS, f"/NODEFAULTLIB:{CRT_REPLACES}", *stack],
        },
    }


def manifest(files: dict[str, bytes]) -> dict:
    import tomllib
    config = tomllib.loads(files["config/units.toml"].decode())
    pins = json.loads(files["config/retail/targets.json"])
    return {
        "compiler": config["build"]["compiler"],
        "locale": pins["game"].get("locale", "ru"),
        "default_target": "game",
        "targets": {image: target(files, image) for image in images(files)},
    }


def import_stubs() -> dict[str, bytes]:
    """imports/<dll>.c and .def for each vendor DLL without an SDK library."""
    from homm1.graph import implib
    output = {}
    referents = implib.referent_imports()
    for dll, hints, existing in implib.survey():
        if existing:
            continue
        stem = dll.rsplit(".", 1)[0].lower()
        code, entries = implib.stub_source(dll, sorted(hints), None, referents.get(dll, {}))
        output[f"imports/{stem}.c"] = source.clean_cpp(code).encode()
        if entries:
            output[f"imports/{stem}.def"] = source.clean_asm(
                implib.def_source(dll, entries)).encode()
    if not output:
        raise ValueError("no vendor import stubs; is the retail executable staged?")
    return output


def flake_lock(files: dict[str, bytes]) -> tuple[bytes, str]:
    lock = json.loads(files["flake.lock"])
    nixpkgs = lock["nodes"][lock["nodes"]["root"]["inputs"]["nixpkgs"]]
    out = {"nodes": {"nixpkgs": nixpkgs, "root": {"inputs": {"nixpkgs": "nixpkgs"}}},
           "root": "root", "version": lock["version"]}
    return (json.dumps(out, indent=2) + "\n").encode(), nixpkgs["locked"]["rev"]


def project_files(files: dict[str, bytes], variant: str = "source") -> dict[str, bytes]:
    """The variant's project files: the source tree's build, the README of each."""
    prefix = f"{TEMPLATE}{variant}/"
    output = {name.removeprefix(prefix): data for name, data in files.items()
              if name.startswith(prefix)}
    if "README.md" not in output:
        raise ValueError(f"no {prefix}README.md template")
    output["LICENSE"] = files["LICENSE"]
    output["heroes.def"] = source.clean_asm(files["config/heroes.def"].decode()).encode()
    output.update(import_stubs())
    if variant != "source":
        return output
    from homm1 import toolchain
    output["play.py"] = files[RUNNER]
    output["build.json"] = (json.dumps(manifest(files), indent=2) + "\n").encode()
    output["flake.lock"], revision = flake_lock(files)
    contract = toolchain.release(manifest(files)["compiler"])
    url = (f"https://github.com/{toolchain.RELEASE_REPOSITORY}/releases/download/"
           f"{contract['tag']}/{contract['asset']}")
    flake = output["flake.nix"].decode()
    for key, value in (("@NIXPKGS_REV@", revision), ("@TOOLCHAIN_URL@", url),
                       ("@TOOLCHAIN_SHA256@", contract["sha256"])):
        if key not in flake:
            raise ValueError(f"template flake.nix lacks {key}")
        flake = flake.replace(key, value)
    output["flake.nix"] = flake.encode()
    return output

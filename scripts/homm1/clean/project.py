"""The generated tree's standalone build inputs.

`build.json` lists each unit with its VC6 profile and the candidate link
contract of `homm1.graph.link`: objects in retail code order (each unit's
lowest claimed function address, read from the annotations before they are
removed), the BASE units archived into the library LINK searches after
mss32.lib, the library line and flags. `imports/` holds the stub-DLL sources
`homm1.graph.implib` derives from the retail import table for the vendor DLLs
whose SDKs ship no import library. Only names, ordinals and order are carried;
no address reaches the tree.
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


def _units(files: dict[str, bytes]) -> list[dict]:
    import tomllib
    return list(tomllib.loads(files["config/units.toml"].decode())["unit"])


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


def manifest(files: dict[str, bytes]) -> dict:
    import tomllib
    from homm1.graph.link import (BASE_LIBRARY, BASE_LIBRARY_AFTER, BASE_LIBRARY_FROM,
                                  CRT_LIBRARY, CRT_REPLACES, LINK_LIBS)
    config = tomllib.loads(files["config/units.toml"].decode())
    # The tree builds the game; editor-only units stay in the matching tree.
    units = [unit for unit in config["unit"] if "game" in unit.get("images", ["game"])]
    keyed = []
    for index, unit in enumerate(units):
        rva = first_function(files, unit)
        if rva is None:
            raise ValueError(f"{unit['unit']}: no claimed function orders it in the link")
        keyed.append((rva, index, unit["unit"]))
    ordered = [(rva, name) for rva, _i, name in sorted(keyed)]
    from homm1.graph.link import LINK_RETAIL_FLAGS
    return {
        "compiler": config["build"]["compiler"],
        "executable": json.loads(files["config/retail/targets.json"])["game"]["name"],
        "locale": json.loads(files["config/retail/targets.json"])["game"].get("locale", "ru"),
        "units": [{"unit": u["unit"], "source": u["source"],
                   "flags": config["flags"][u["flags"]]} for u in units],
        "link": {
            "objects": [name for rva, name in ordered if rva < BASE_LIBRARY_FROM],
            "members": [name for rva, name in ordered if rva >= BASE_LIBRARY_FROM],
            "library": BASE_LIBRARY,
            "library_after": BASE_LIBRARY_AFTER,
            "libraries": [*LINK_LIBS, CRT_LIBRARY],
            "flags": ["/SUBSYSTEM:WINDOWS", "/BASE:0x400000", "/INCREMENTAL:NO",
                      *LINK_RETAIL_FLAGS, f"/NODEFAULTLIB:{CRT_REPLACES}",
                      "/STACK:0x10240,0x1000"],
        },
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

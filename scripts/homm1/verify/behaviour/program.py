"""Build and run game_contracts.cpp against the reconstructed game objects.

The program is compiled with the SOURCE/KB compiler profile and linked with
the pinned VC6 linker against a static library of every game object the
candidate link uses, so LINK pulls in only what the contracts reach. The
vendor DLLs (MSS32, SMACKW32, audiere) are delay-loaded: the contracts never
call them, so the program runs under Wine without the game folder. Output
goes to build/behaviour/; nothing here writes outside it.
"""
from __future__ import annotations

from pathlib import Path

from homm1.core.paths import BUILD, REPO
from homm1.tool import ToolError

HERE = Path(__file__).resolve().parent
SOURCE = HERE / "game_contracts.cpp"
OUT = BUILD / "behaviour"
#: The compiler profile of the unit whose prelude the contracts share.
PROFILE_UNIT = "SOURCE/KB"
#: Imports of vendor DLLs the contracts never reach.
DELAY_LOADED = ("mss32.dll", "smackw32.dll", "audiere.dll")


def link_objects() -> list[Path]:
    """The game objects `homm1 link` links, in manifest order: each unit's
    compiled object, or the OMF assembly of a fixed MASM unit."""
    from homm1 import graph
    from homm1.graph.fixed_asm import unit as fixed_asm_unit
    from homm1.manifest import units

    base = graph.image_paths("game")["BASE_DIR"]
    objects = []
    for unit in units(image="game"):
        if fixed_asm_unit(unit["unit"], unit["source"]) is not None:
            objects.append(REPO / graph.LINK_OMF_DIR / f"{unit['unit']}.obj")
        else:
            objects.append(REPO / base / f"{unit['unit']}.obj")
    missing = [o for o in objects if not o.is_file()]
    if missing:
        raise ToolError(f"{len(missing)} game object(s) missing (first: "
                        f"{missing[0].relative_to(REPO)}); run `homm1 link`")
    return objects


def build() -> Path:
    """Compile and link the contracts program; return the executable."""
    from homm1.core.paths import image_key
    from homm1.graph.link import library_paths
    from homm1.manifest import flag_profiles, units
    from homm1.tool import cl, link
    from homm1.tool.wine import winepath

    if image_key() != "game":
        raise ToolError("the game contracts link the game image's objects; "
                        "run without --image")
    OUT.mkdir(parents=True, exist_ok=True)
    owner = next(u for u in units(image="game") if u["unit"] == PROFILE_UNIT)
    obj, lib, exe = OUT / "game_contracts.obj", OUT / "game.lib", OUT / "game_contracts.exe"
    cl.compile(SOURCE, obj, flag_profiles()[owner["flags"]])
    library = OUT / "game.lib.rsp"
    library.write_text("\n".join(["/NOLOGO", f"/OUT:{winepath(lib)}",
                                  *[f'"{winepath(o)}"' for o in link_objects()]]) + "\n")
    # `-lib` must be LINK's first argument; inside a response file it is
    # read as a link option.
    link.link(["-lib", f"@{winepath(library)}"], cwd=OUT, expect=[lib])
    libraries = [lib, *library_paths(), "delayimp.lib"]
    response = OUT / "game_contracts.rsp"
    response.write_text("\n".join([
        "/NOLOGO", "/SUBSYSTEM:CONSOLE", "/INCREMENTAL:NO", f"/OUT:{winepath(exe)}",
        f"/MAP:{winepath(exe.with_suffix('.map'))}",
        "/NODEFAULTLIB:libc.lib", *[f"/DELAYLOAD:{dll}" for dll in DELAY_LOADED],
        f'"{winepath(obj)}"',
        *[f'"{winepath(p)}"' if Path(p).is_file() else str(p) for p in libraries],
    ]) + "\n")
    link.link([f"@{winepath(response)}"], cwd=OUT, expect=[exe])
    return exe


def run(exe: Path, *arguments: str) -> list[str]:
    """Run the program under Wine; its output lines (a non-zero exit or a
    crash raises)."""
    from homm1.tool.wine import disable_crash_dialog, headless_env
    from homm1.tool.wine import run as wine_run

    import shutil

    work = OUT / "run"
    shutil.rmtree(work, ignore_errors=True)
    work.mkdir(parents=True)
    # Headless: no display reaches the program and no crash dialog opens; an
    # unhandled exception is the program's own exit code 3.
    disable_crash_dialog()
    output, rc = wine_run(["wine", str(exe), *arguments], cwd=work, timeout=120,
                          env=headless_env())
    lines = [line.rstrip("\r") for line in output.splitlines()]
    # Wine's own diagnostics share the stream; the program prints none of them.
    lines = [line for line in lines if not _wine_noise(line)]
    if rc != 0:
        raise ToolError(f"game_contracts.exe exited {rc}:\n" + "\n".join(lines[-20:]))
    return lines


def _wine_noise(line: str) -> bool:
    import re
    return bool(re.match(r"^[0-9a-f]{4}:(err|fixme|warn|trace):", line))

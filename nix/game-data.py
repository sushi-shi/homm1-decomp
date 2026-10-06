#!/usr/bin/env python3
"""Check a Buka 2003 game copy and lay out the native programs' read-only data.

    game-data.py --runner play.py --game PATH --out DIR [--retail DIR]

PATH is what `nix run .#play -- --game` takes: an installed game folder, the
CD (a mount or a copy of its files), its .iso image or a .zip/.7z of either,
or a folder holding only such an image.
The copy is found and checked with play.py's own rules (the resource archive
by SHA-256, the other files by name and size; the CD's installer is unpacked
with unshield, images are read with 7z) and laid out as

    DIR/game/DATA, ANIM, SOUND, MAPS, GAMES, HELP   (the names upper-cased)
    DIR/cd/Tracks                                     (the CD's music, if any)

Nothing else is taken: the Windows programs and DLLs are not needed. With
--retail, the copy's retail HEROES.EXE and EDITOR.EXE (checked by SHA-256) are
copied there, for their icons. Used by the flake's game package at install
time and by its launchers on first run (HOMM1_GAME).
"""

from __future__ import annotations

import argparse
import importlib.util
from pathlib import Path
import shutil
import sys
import tempfile

#: The game folder's directories the native programs read.
GAME_DIRS = ("DATA", "ANIM", "SOUND", "MAPS", "GAMES", "HELP")


def load_runner(path: Path):
    spec = importlib.util.spec_from_file_location("play", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def check(play, install: Path) -> list[str]:
    """play.check_install's rules without the Windows runtime DLLs: errors."""
    errors = []
    path = play.find_ci(install, play.AGG[0])
    if path is None or not path.is_file():
        errors.append(f"missing {play.AGG[0]}")
    elif path.stat().st_size != play.AGG[1] or play.sha256(path) != play.AGG[2]:
        errors.append(f"{play.AGG[0]} is not the Buka 2003 file")
    for relative, size in play.GAME_FILES.items():
        path = play.find_ci(install, relative)
        required = relative.startswith("DATA/")
        if path is None or not path.is_file():
            (errors.append if required else play.say)(
                f"{'' if required else 'warning: '}missing {relative}")
        elif not required and path.stat().st_size != size:
            play.say(f"warning: {relative}: {path.stat().st_size} bytes, expected {size}")
    return errors


def image_in(play, given: Path) -> Path:
    """A folder holding no game but exactly one image stands for that image
    (a flake input that is the folder of the .iso)."""
    if not given.is_dir() or play.search(given, play.AGG[0], depth=3) is not None \
            or play.search(given, play.CABINET, depth=2) is not None:
        return given
    images = [p for p in given.iterdir() if p.is_file() and p.suffix.lower() in play.IMAGES]
    return images[0] if len(images) == 1 else given


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--runner", type=Path, required=True, help="play.py")
    parser.add_argument("--game", type=Path, action="append", required=True, metavar="PATH")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--retail", type=Path)
    parser.add_argument("--work", type=Path, help="where to unpack (default: $TMPDIR)")
    args = parser.parse_args()
    play = load_runner(args.runner)
    if args.out.exists():
        print(f"[import] {args.out} already exists", file=sys.stderr)
        return 1
    try:
        with tempfile.TemporaryDirectory(prefix=".import-", dir=args.work) as work:
            copies = [play.locate(image_in(play, path.expanduser().resolve()),
                                  Path(work) / str(i), False)
                      for i, path in enumerate(args.game)]
            install = next((c.install for c in copies if c.install), None)
            if install is None:
                raise play.PlayError("no installed game in " + ", ".join(map(str, args.game))
                                     + ": pass the game folder, the CD or its image")
            errors = check(play, install)
            if errors:
                raise play.PlayError(f"{install}: not the Buka 2003 game: "
                                     + "; ".join(errors[:6]))
            play.say(f"checked the game files in {install}")
            game = args.out / "game"
            game.mkdir(parents=True)
            for name in GAME_DIRS:
                source = play.find_ci(install, name)
                if source is not None and source.is_dir():
                    shutil.copytree(source, game / name)
            tracks = next((c.tracks for c in copies if c.tracks), None)
            if tracks is not None:
                for problem in play.check_tracks(tracks)[:10]:
                    play.say(f"warning: {problem}")
                shutil.copytree(tracks, args.out / "cd" / "Tracks")
            if args.retail is not None:
                for copy in copies:
                    for program, executable in copy.executables.items():
                        size, digest = play.PROGRAMS[program]["retail"]
                        if executable.stat().st_size == size and play.sha256(executable) == digest:
                            args.retail.mkdir(parents=True, exist_ok=True)
                            shutil.copyfile(executable,
                                            args.retail / play.PROGRAMS[program]["executable"])
    except (play.PlayError, OSError) as error:
        print(f"[import] {error}", file=sys.stderr)
        return 1
    play.say(f"game data laid out in {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

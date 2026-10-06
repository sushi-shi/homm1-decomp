#!/usr/bin/env python3
"""Saves over the shipped saved game on a full disk; the game must go on.

    save_diskfull.py HEROES_BINARY

Needs $HOMM1_DATA (the game folder) and xvfb-run; without them the test is
skipped (exit 77). The game folder is copied to a temporary folder, and the
file the save is written to first (GAMES\\________.GM1.partial, see
FileReplace) is a link to /dev/full, so every write fails with ENOSPC. The
script loads the saved game and saves over it, as save_roundtrip.py does,
then answers the error message and takes a screenshot. The program must
still be running for the screenshot (it used to end with the file error),
and the old save must be unchanged.
"""
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

SAVE = "________.GM1"

REPLAY = """\
3000 click 497 170
+1500 click 497 104
+2000 click 468 63
+1000 click 392 305
+4500 click 604 378
+2000 click 252 140
+2000 click 243 330
+3000 key return
+2000 shot {shot}
+1000 exit
"""


def find(directory: Path, name: str) -> Path | None:
    for entry in directory.iterdir():
        if entry.name.lower() == name.lower():
            return entry
    return None


def main() -> int:
    binary = Path(sys.argv[1]).resolve()
    data = os.environ.get("HOMM1_DATA")
    if not data or shutil.which("xvfb-run") is None or not Path("/dev/full").exists():
        print("skipped: needs HOMM1_DATA, xvfb-run and /dev/full")
        return 77
    with tempfile.TemporaryDirectory() as scratch:
        root = Path(scratch) / "game"
        shutil.copytree(data, root, ignore=shutil.ignore_patterns("*.exe", "*.EXE", "*.dll", "*.DLL"))
        games = find(root, "GAMES")
        save = find(games, SAVE)
        original = save.read_bytes()
        (games / (save.name + ".partial")).symlink_to("/dev/full")
        shot = Path(scratch) / "after.bmp"
        replay = Path(scratch) / "diskfull.replay"
        replay.write_text(REPLAY.format(shot=shot))
        environment = dict(os.environ, HOMM1_DATA=str(root), HOMM1_INPUT_REPLAY=str(replay),
                           HOMM1_NO_DIALOGS="1", SDL_AUDIO_DRIVER="dummy",
                           XDG_CONFIG_HOME=str(Path(scratch) / "config"))
        result = subprocess.run(["xvfb-run", "-a", "-s", "-screen 0 1024x768x24", str(binary), "/I0"],
                                env=environment, cwd=scratch, timeout=300,
                                capture_output=True, text=True)
        if result.returncode != 0:
            print(result.stdout, result.stderr)
            print(f"the program ended with {result.returncode}")
            return 1
        if not shot.exists():
            print(result.stderr[-2000:])
            print("the program did not go on after the failed save")
            return 1
        if save.read_bytes() != original:
            print("the old save was changed")
            return 1
    print(f"ok: a save on a full disk is reported and the game goes on; {SAVE} is unchanged")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

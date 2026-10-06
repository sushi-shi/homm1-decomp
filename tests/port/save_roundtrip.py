#!/usr/bin/env python3
"""Loads the shipped saved game in the real program and saves it again.

    save_roundtrip.py HEROES_BINARY

Needs $HOMM1_DATA (the game folder) and xvfb-run; without them the test is
skipped (exit 77). The game folder is copied to a temporary folder first.
A script of clicks (HOMM1_INPUT_REPLAY) opens the load screen, loads
GAMES\\________.GM1, opens the file options and saves over it. The new file
must equal the shipped one byte for byte except the 17-byte save-name field
(offset 207), which holds the name of the file the game saved to.
"""
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

NAME_OFFSET = 207
NAME_SIZE = 0x11
SAVE = "________.GM1"

REPLAY = """\
3000 click 497 170
+1500 click 497 104
+2000 click 468 63
+1000 click 392 305
+4500 click 604 378
+2000 click 252 140
+2000 click 243 330
+2000 exit
"""


def find(directory: Path, name: str) -> Path | None:
    for entry in directory.iterdir():
        if entry.name.lower() == name.lower():
            return entry
    return None


def main() -> int:
    binary = Path(sys.argv[1]).resolve()
    data = os.environ.get("HOMM1_DATA")
    if not data or shutil.which("xvfb-run") is None:
        print("skipped: needs HOMM1_DATA and xvfb-run")
        return 77
    with tempfile.TemporaryDirectory() as scratch:
        root = Path(scratch) / "game"
        shutil.copytree(data, root, ignore=shutil.ignore_patterns("*.exe", "*.EXE", "*.dll", "*.DLL"))
        games = find(root, "GAMES")
        original = (games / SAVE).read_bytes()
        replay = Path(scratch) / "roundtrip.replay"
        replay.write_text(REPLAY)
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
        saved = (games / SAVE).read_bytes()
    if len(saved) != len(original):
        print(f"size {len(saved)} differs from the shipped {len(original)}")
        return 1
    differences = [i for i in range(len(saved)) if saved[i] != original[i]
                   and not NAME_OFFSET <= i < NAME_OFFSET + NAME_SIZE]
    if differences:
        print(f"{len(differences)} bytes differ, first at offset {differences[0]}")
        return 1
    print(f"ok: {SAVE} saved again is identical outside its name field ({len(saved)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

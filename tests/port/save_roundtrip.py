#!/usr/bin/env python3
"""Loads the shipped saved game in the real program and saves it again.

    save_roundtrip.py HEROES_BINARY

Needs $HOMM1_DATA (the game folder); without it the test is
skipped (exit 77). The game folder is copied to a temporary folder first.
A script of clicks (HOMM1_INPUT_REPLAY) opens the load screen, loads
GAMES\\________.GM1, opens the file options and saves over it. The new file
must equal the shipped one byte for byte except the 17-byte save-name field
(offset 207), which holds the name of the file the game saved to, and what
the edition upgrades in an original game's save: its reserved header block
(offset 23) now holds the format tag, "H1TE" and version 2, the heroes on
offer in taverns are reserved (their availability byte goes from -1 to
0x40), and the towns' "built today" flags take a fifth byte (zero: towns
32-35 have none in an original save). Loading that save and saving it
again must then reproduce it exactly outside the name field.
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
RESERVED_OFFSET = 23
RESERVED_SIZE = 0x2c
FORMAT_TAG = b"H1TE" + (2).to_bytes(4, "little")
# The bytes after the towns' flags: mines, their owners, the random
# artifacts, boats and their slots, obelisk visitors, the ultimate
# artifact, and the three map grids.
FLAGS_TAIL = 36 * 7 + 36 + 0x25 + 32 * 8 + 32 + 0x30 + 3 + 72 * 72 * 3
ORIGINAL_FLAG_BYTES = 4
FLAG_BYTES = 5
TAVERN_LIMIT = 6 * 2

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


def run(binary: Path, root: Path, scratch: str) -> bool:
    replay = Path(scratch) / "roundtrip.replay"
    replay.write_text(REPLAY)
    environment = dict(os.environ, HOMM1_DATA=str(root), HOMM1_INPUT_REPLAY=str(replay),
                       HOMM1_NO_DIALOGS="1", SDL_AUDIO_DRIVER="dummy",
                       SDL_VIDEODRIVER="dummy",
                       XDG_CONFIG_HOME=str(Path(scratch) / "config"))
    result = subprocess.run([str(binary), "/I0"],
                            env=environment, cwd=scratch, timeout=300,
                            capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stdout, result.stderr)
        print(f"the program ended with {result.returncode}")
        return False
    return True


def outside_name(first: bytes, second: bytes) -> list[int]:
    return [i for i in range(len(first)) if first[i] != second[i]
            and not NAME_OFFSET <= i < NAME_OFFSET + NAME_SIZE]


def main() -> int:
    binary = Path(sys.argv[1]).resolve()
    data = os.environ.get("HOMM1_DATA")
    if not data:
        print("skipped: needs HOMM1_DATA")
        return 77
    with tempfile.TemporaryDirectory() as scratch:
        root = Path(scratch) / "game"
        shutil.copytree(data, root, ignore=shutil.ignore_patterns("*.exe", "*.EXE", "*.dll", "*.DLL"))
        games = find(root, "GAMES")
        original = (games / SAVE).read_bytes()
        if not run(binary, root, scratch):
            return 1
        saved = (games / SAVE).read_bytes()
        if not run(binary, root, scratch):
            return 1
        again = (games / SAVE).read_bytes()
    grown = FLAG_BYTES - ORIGINAL_FLAG_BYTES
    if len(saved) != len(original) + grown or len(again) != len(saved):
        print(f"sizes {len(saved)}, {len(again)} are not the shipped {len(original)} "
              f"and {grown} more flag byte")
        return 1
    added = len(saved) - FLAGS_TAIL - grown
    if saved[added:added + grown] != bytes(grown):
        print(f"the added town flag byte is not zero: {saved[added:added + grown].hex()}")
        return 1
    tagged = saved
    saved = saved[:added] + saved[added + grown:]
    block = saved[RESERVED_OFFSET:RESERVED_OFFSET + RESERVED_SIZE]
    if block != FORMAT_TAG + bytes(RESERVED_SIZE - len(FORMAT_TAG)):
        print(f"the header block is not the edition's tag: {block.hex()}")
        return 1
    differences = [i for i in outside_name(saved, original)
                   if not RESERVED_OFFSET <= i < RESERVED_OFFSET + RESERVED_SIZE]
    reserved = [i for i in differences if original[i] == 0xff and saved[i] == 0x40]
    if len(reserved) != len(differences) or len(reserved) > TAVERN_LIMIT:
        other = [i for i in differences if i not in reserved]
        print(f"{len(other)} bytes differ besides the tag and {len(reserved)} tavern heroes, "
              f"first at offset {other[0] if other else '-'}")
        return 1
    repeated = outside_name(again, tagged)
    if repeated:
        print(f"saved again, {len(repeated)} bytes differ, first at offset {repeated[0]}")
        return 1
    print(f"ok: {SAVE} saved again carries the edition's tag (H1TE, version 2), reserves "
          f"{len(reserved)} tavern heroes, adds a town flag byte and is otherwise identical "
          f"outside its name field; saved once more it is unchanged ({len(tagged)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

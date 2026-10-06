#!/usr/bin/env python3
"""nix/game-data.py's archive handling: a .rar, or a folder holding only one,
is unpacked with unar and stands for the image or game folder inside it.

unar is replaced by a stand-in on PATH that copies a prepared folder, so the
test needs no game data and no RAR archive (no free tool writes one).
"""

import importlib.util
import os
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def load(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


game_data = load("game_data", ROOT / "nix" / "game-data.py")
play = game_data.load_runner(ROOT / "play.py")

#: The stand-in: `unar -quiet -no-directory -output-directory DIR ARCHIVE`
#: copies the folder named in ARCHIVE into DIR and logs its arguments.
FAKE_UNAR = """#!{python}
import shutil, sys
from pathlib import Path
args = sys.argv[1:]
with open({log!r}, "a") as log:
    log.write(" ".join(args) + "\\n")
target = Path(args[args.index("-output-directory") + 1])
shutil.copytree(Path(args[-1]).read_text().strip(), target)
"""


class ArchiveTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory(prefix="homm1 game data ")
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.log = self.root / "unar.log"
        tools = self.root / "bin"
        tools.mkdir()
        unar = tools / "unar"
        unar.write_text(FAKE_UNAR.format(python=sys.executable, log=str(self.log)))
        unar.chmod(0o755)
        path = os.environ.get("PATH", "")
        self.addCleanup(os.environ.__setitem__, "PATH", path)
        os.environ["PATH"] = f"{tools}{os.pathsep}{path}"

    def archive(self, name: str, contents: dict[str, bytes]) -> Path:
        """`name`, a stand-in archive of `contents`."""
        packed = self.root / "contents"
        for relative, data in contents.items():
            (packed / relative).parent.mkdir(parents=True, exist_ok=True)
            (packed / relative).write_bytes(data)
        archive = self.root / "input" / name
        archive.parent.mkdir(parents=True, exist_ok=True)
        archive.write_text(str(packed))
        return archive

    def unpack(self, given: Path) -> Path:
        work = self.root / "work"
        work.mkdir(exist_ok=True)
        return game_data.unpack_archive(play, given, work)

    def test_rar_of_the_cd_image_stands_for_the_image(self):
        archive = self.archive("heroes-platinum-buka.rar",
                               {"Герои. Платиновая версия [Бука].iso": b"image"})
        found = self.unpack(archive)
        self.assertEqual(found.name, "Герои. Платиновая версия [Бука].iso")
        self.assertEqual(found.read_bytes(), b"image")
        self.assertIn("-no-directory", self.log.read_text())

    def test_folder_holding_only_the_rar(self):
        archive = self.archive("Heroes.RAR", {"heroes.iso": b"image"})
        self.assertEqual(self.unpack(archive.parent).name, "heroes.iso")

    def test_rar_of_an_installed_game_stands_for_its_folder(self):
        archive = self.archive("installed.rar", {"Heroes/DATA/HEROES.AGG": b"agg"})
        found = self.unpack(archive)
        self.assertTrue(found.is_dir())
        self.assertIsNotNone(play.search(found, play.AGG[0], depth=3))

    def test_other_copies_are_left_to_play(self):
        image = self.root / "input" / "heroes.iso"
        image.parent.mkdir()
        image.write_bytes(b"image")
        self.assertEqual(self.unpack(image), image)
        self.assertEqual(self.unpack(image.parent), image)
        self.assertFalse(self.log.exists())

    def test_failed_unpack_is_reported(self):
        archive = self.root / "broken.rar"
        archive.write_text(str(self.root / "missing"))
        with self.assertRaisesRegex(play.PlayError, "unar failed"):
            self.unpack(archive)


if __name__ == "__main__":
    unittest.main()

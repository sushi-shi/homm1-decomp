"""The game runner's file facts, copy rules, prefix settings and dry run."""
import argparse
import contextlib
import io
from pathlib import Path
import tempfile
import unittest

from homm1.graph import play


class FactTests(unittest.TestCase):
    def test_every_cd_track_has_an_installed_counterpart(self):
        names = {f"{track:02d}-AudioTrack {track:02d}.ogg"
                 for track in play.TRACK_OF_SOUND.values()}
        self.assertEqual(names, set(play.CD_TRACKS))
        sounds = {f"SOUND/HEROES{n:02d}.ogg" for n in play.TRACK_OF_SOUND}
        self.assertEqual(sounds, {name for name in play.GAME_FILES
                                  if name.startswith("SOUND/")})

    def test_required_data(self):
        self.assertIn(play.AGG[0], play.GAME_FILES)
        self.assertEqual(play.GAME_FILES[play.AGG[0]], play.AGG[1])


class FileTests(unittest.TestCase):
    def test_copy_keeps_existing_files_and_skips_built_executables(self):
        with tempfile.TemporaryDirectory() as work:
            source, target = Path(work, "source"), Path(work, "target")
            (source / "Data").mkdir(parents=True)
            (source / "Data/STANDARD.HS").write_text("retail")
            (source / "heroes.exe").write_text("retail")
            (target / "DATA").mkdir(parents=True)
            (target / "DATA/standard.hs").write_text("played")
            copied = play.copy_missing(source, target, play.BUILT)
            self.assertEqual(copied, 0)
            self.assertEqual((target / "DATA/standard.hs").read_text(), "played")
            self.assertFalse((target / "heroes.exe").exists())

    def test_check_reports_missing_data(self):
        with tempfile.TemporaryDirectory() as work:
            errors, _warnings = play.check_install(Path(work))
            self.assertIn(f"missing {play.AGG[0]}", errors)

    def test_registry_sets_the_cd_drive_and_game_key(self):
        text = play.registry(Path("/state"))
        self.assertIn('"d:"="cdrom"', text)
        self.assertIn(r"[HKEY_LOCAL_MACHINE\Software\Wow6432Node\Buka", text)
        self.assertIn(r'"AppPath"="Z:\\state\\game"', text)
        self.assertIn('"HMM1 CDDrive"="D:"', text)


class CommandLineTests(unittest.TestCase):
    def test_game_arguments_follow_the_separator(self):
        self.assertEqual(play.split_argv(["--window", "--", "-x", "--y"]),
                         (["--window"], ["-x", "--y"]))

    def test_dry_run_changes_nothing(self):
        parser = argparse.ArgumentParser()
        play.add_arguments(parser, standalone=False)
        with tempfile.TemporaryDirectory() as work:
            state = Path(work, "state")
            args = parser.parse_args(["--dry-run", "--state", str(state)])
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                status = play.session(args, [], lambda icon: Path("/built/HEROES.EXE"))
            self.assertEqual(status, 0)
            self.assertFalse(state.exists())
            self.assertIn("would run in", output.getvalue())


if __name__ == "__main__":
    unittest.main()

"""Candidate object order reads only the selected image's claims."""
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from homm1.graph import link


class FirstClaimedRvaTests(unittest.TestCase):
    def test_other_image_claims_are_ignored(self):
        with tempfile.TemporaryDirectory() as tmp:
            claims = Path(tmp)
            (claims / "SOURCE").mkdir()
            (claims / "SOURCE/kbwin.tsv").write_text(
                "rva\tsize\tname\tkind\tchannel\ttype\tspace\n"
                "0x0000cd4e\t0x6c2\t?AppWndProc\tfunc\tsrc\t\teditor\n"
                "0x00042ca0\t0x11a\t_WinMain@16\tfunc\tsrc\t\tgame\n"
                "0x00042000\t0x10\t_$E1\tfunc\tsrc_dyninit\t\tgame\n")
            obj = Path("build/objdiff/base/SOURCE/kbwin.obj")
            with mock.patch("homm1.core.paths.image_key", return_value="game"):
                self.assertEqual(link.first_claimed_rva(obj, claims), 0x42ca0)
            with mock.patch("homm1.core.paths.image_key", return_value="editor"):
                self.assertEqual(link.first_claimed_rva(obj, claims), 0xcd4e)


if __name__ == "__main__":
    unittest.main()

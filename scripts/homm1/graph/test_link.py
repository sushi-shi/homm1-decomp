"""Candidate object order: the game reads its own claims, another image its
reviewed link_order.tsv (its shared units' claims spell game addresses)."""
import struct
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
            with mock.patch("homm1.core.paths.image_key", return_value="editor"), \
                    mock.patch.object(link, "_image_unit_starts",
                                      return_value={"SOURCE/kbwin": 0xc9a0}):
                self.assertEqual(link.first_claimed_rva(obj, claims), 0xc9a0)


def stamped(stamp: int, sig: int, age: int) -> bytes:
    """A minimal image: the header TimeDateStamp and a trailing NB10 record."""
    data = bytearray(0x100)
    struct.pack_into("<I", data, 0x3C, 0x40)
    struct.pack_into("<I", data, 0x48, stamp)
    return bytes(data) + b"NB10" + struct.pack("<III", 0, sig, age)


class LinkStampTests(unittest.TestCase):
    def test_reads_header_and_pdb_stamps(self):
        self.assertEqual(link.link_stamps(stamped(0x3e96d447, 0x3e5cda55, 2)),
                         (0x3e96d447, 0x3e5cda55, 2))

    def test_matching_candidate_passes(self):
        retail = stamped(0x3e96d447, 0x3e5cda55, 2)
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "EDITOR.candidate.EXE"
            out.write_bytes(retail)
            with mock.patch("homm1.core.pe.image",
                            return_value=mock.Mock(data=retail)):
                link.check_link_stamps(out)
            self.assertTrue(out.exists())

    def test_zone_drift_fails_and_sets_the_image_aside(self):
        retail = stamped(0x3e96d447, 0x3e96d447, 1)
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp) / "EDITOR.candidate.EXE"
            out.write_bytes(stamped(0x3e96d447 + 7200, 0x3e96d447 + 7200, 1))
            with mock.patch("homm1.core.pe.image",
                            return_value=mock.Mock(data=retail)):
                with self.assertRaisesRegex(Exception, r"\+2 h: the wineserver"):
                    link.check_link_stamps(out)
            self.assertFalse(out.exists())
            self.assertTrue((Path(tmp) / "EDITOR.candidate.stamp-mismatch.EXE").exists())


if __name__ == "__main__":
    unittest.main()

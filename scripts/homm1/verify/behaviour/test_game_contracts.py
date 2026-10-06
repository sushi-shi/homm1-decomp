"""The reconstructed game's tables, formulas, AI, pathing and records.

game_contracts.cpp is linked against the game objects and run under Wine; its
output must equal game_contracts.expected line for line. The expected files
are behaviour snapshots taken from the retail-exact build, not values derived
here: a difference means the game now behaves differently. Change a snapshot
only for an intended behaviour change, from the output this test writes to
build/behaviour/, and say why in the commit.
"""
import unittest
from pathlib import Path

from homm1.verify.behaviour import program

HERE = Path(__file__).resolve().parent
#: Reason the shipped-file case gives when no game copy is imported; the gate
#: accepts this skip and no other.
NO_GAME_COPY = "no imported game copy"


def _expected(name: str) -> list[str]:
    return (HERE / name).read_text(encoding="utf-8").splitlines()


class GameContractsTest(unittest.TestCase):
    maxDiff = None

    @classmethod
    def setUpClass(cls):
        cls.exe = program.build()

    def check(self, lines: list[str], snapshot: str) -> None:
        actual = program.OUT / snapshot
        actual.write_text("\n".join(lines) + "\n", encoding="utf-8")
        expected = _expected(snapshot)
        if lines == expected:
            return
        import difflib
        changed = [line for line in difflib.unified_diff(expected, lines, snapshot, str(actual),
                                                          lineterm="", n=0)]
        first = next(i for i, (a, b) in enumerate(zip(expected + [None] * len(lines),
                                                      lines + [None] * len(expected)))
                     if a != b)
        self.fail(f"{snapshot} line {first + 1} differs (expected "
                  f"{(expected[first:first + 1] or ['<end>'])[0]!r}, got "
                  f"{(lines[first:first + 1] or ['<end>'])[0]!r}); full output in {actual}\n"
                  + "\n".join(changed))

    def test_tables_formulas_ai_pathing_and_records(self):
        self.check(program.run(self.exe), "game_contracts.expected")

    def test_shipped_saves_and_maps(self):
        from homm1.graph.play import GAME_FILES, installed_game
        from homm1.tool.wine import winepath
        game = installed_game()
        if game is None:
            self.skipTest(f"{NO_GAME_COPY} (`homm1 play` imports one)")
        lines = program.run(self.exe, "--installed", winepath(game))
        # Saves change as the copy is played: each one present must survive a
        # reload and resave. The shipped maps and ORIGDATA.BIN never change.
        saves = [line for line in lines if line.startswith("save ")]
        self.assertTrue(saves, lines)
        for line in saves:
            self.assertTrue(line.endswith("identical outside the name"), line)
        shipped = {name.split("/", 1)[1].upper() for name in GAME_FILES
                   if name.startswith("MAPS/")}
        self.check([line for line in lines if line.startswith("original data:")
                    or (line.startswith("map ") and line.split()[1][:-1].upper() in shipped)],
                   "installed_game.expected")


if __name__ == "__main__":
    unittest.main()

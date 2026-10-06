"""homm1.verify.behaviour - `homm1 verify behaviour` - game-behaviour tests.

These tests exercise what the reconstructed game does, not how the tooling
matches it. A byte-identical comparison already proves the decompilation
branch reproduces retail; these catch a behaviour change slipping in through
cleanup, a review mishap, or a source change that is meant to change bytes.

  test_game_contracts  links game_contracts.cpp against the game objects
                       (SOURCE/KB compiler profile, pinned VC6 linker) and runs
                       it under Wine: the creature, town, building, hero-class,
                       spell and artifact tables; costs, experience, movement
                       and terrain costs; luck, morale and the seeded rolls;
                       damage through army::DamageEnemy; FightValueOfStack and
                       ProbableOutcomeOfBattle; FindNearestObject on a map
                       fixture; the save and map records. With an imported game
                       copy (`homm1 play`) it also round-trips the saves present
                       and loads the shipped maps and ORIGDATA.BIN.
  test_catalog_text    the catalog sentences the combat screen composes, in
                       every language.

Catalog completeness, argument signatures and fixed-width fits are
`homm1 verify localization`'s (fast tier); they are not repeated here.

The expected outputs are behaviour snapshots of the retail-exact build. The
gate fails when a test fails or errors, when none ran, and on any skip other
than the shipped-file case without an imported game copy.
"""
from __future__ import annotations

import argparse
import io
import sys
import unittest
from pathlib import Path

from homm1.core.usage import logged

HERE = Path(__file__).resolve().parent


def _suite(patterns=()) -> unittest.TestSuite:
    loader = unittest.TestLoader()
    if patterns:
        loader.testNamePatterns = [f"*{p}*" for p in patterns]
    return loader.discover(str(HERE), pattern="test_*.py", top_level_dir=str(HERE.parents[2]))


def _reason(traceback: str) -> str:
    """The exception line of a unittest traceback (its message's first line)."""
    lines = traceback.strip().splitlines()
    for line in lines:
        if not line.startswith(" ") and ("Error" in line.split(":", 1)[0]):
            return line
    return lines[-1]


def _verdict(result: unittest.TestResult) -> list[str]:
    from homm1.verify.behaviour.test_game_contracts import NO_GAME_COPY
    findings = [f"{test.id().rsplit('.', 1)[-1]}: {_reason(text)}"
                for test, text in result.failures + result.errors]
    findings += [f"{test.id()}: skipped ({reason})" for test, reason in result.skipped
                 if not reason.startswith(NO_GAME_COPY)]
    if result.testsRun == 0:
        findings.append("no behaviour test ran")
    return findings


def gate_findings() -> list[str]:
    """The tier entry: run every test quietly; one finding per problem."""
    stream = io.StringIO()
    result = unittest.TextTestRunner(stream=stream, verbosity=0).run(_suite())
    return _verdict(result)


@logged
def main(argv=None) -> int:
    parser = argparse.ArgumentParser(prog="homm1 verify behaviour",
                                     description=__doc__.splitlines()[0])
    parser.add_argument("-k", dest="pattern", action="append", default=[],
                        help="run only tests whose name contains PATTERN")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args(argv)
    result = unittest.TextTestRunner(stream=sys.stderr, verbosity=2 if args.verbose else 1
                                     ).run(_suite(args.pattern))
    findings = _verdict(result)
    for finding in findings:
        print(f"[behaviour] {finding.splitlines()[0]}")
    print(f"[behaviour] {result.testsRun} test(s), {len(findings)} finding(s)")
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())

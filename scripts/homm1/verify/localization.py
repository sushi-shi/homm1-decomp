"""homm1 verify localization - the catalogs and the authored text.

Every language in locales/ (a <code>.po with its <code>.json descriptor) must
translate every template ID and no other, without fuzzy entries; all languages
give each ID the same printf arguments and each fixed-width field the same byte
length; every text encodes in its language's Windows code page and, outside the
resource script, uses only glyphs the game fonts draw. locales/messages.pot
and each .po must be what `homm1 localization update` writes, and authored
source keeps its text in the catalog (no inline non-ASCII or numeric escapes).
Reads sources only; a fast-tier gate.
"""

from __future__ import annotations

from homm1.core.usage import logged


def gate_findings() -> list[str]:
    from homm1.core.paths import REPO
    from homm1.graph.localization import check
    return check(REPO)


@logged
def main(argv=None) -> int:
    import argparse
    argparse.ArgumentParser(prog="homm1 verify localization", description=__doc__,
                            formatter_class=argparse.RawDescriptionHelpFormatter
                            ).parse_args(argv)
    findings = gate_findings()
    for finding in findings:
        print(f"[localization] {finding}")
    print(f"[localization] {len(findings)} finding(s)")
    return 1 if findings else 0


if __name__ == "__main__":
    raise SystemExit(main())

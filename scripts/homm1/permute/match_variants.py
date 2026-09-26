#!/usr/bin/env python3
"""Unified, bounded source-variant search frontend.

This is the public entry point for the matching campaign.  It combines libclang AST
mutations, parser-visible TU-state declarations/includes/functions, and optional exact
hand-authored axes into one validated manifest, then optionally compiles and scores the
Cartesian search in one command.  Source transformations are never regex-based, source is
restored after every compile, and generated source is emitted only for exact closure.

Example::

    homm1 permute variants src/Homm1/GameLevel.cpp 0x160450 \
        --state-trials 32 --min-depth 1 --max-depth 3 --limit 512 \
        -o /tmp/gamelevel-combined.json --run

The public CLI classifies the wall and checks historical MAX before entering here.
"""

from homm1.core.usage import logged

from homm1.permute.generate_ast_variants import main as generate_main


@logged
def main(argv=None) -> int:
    return generate_main(argv, prog="homm1 permute variants", description=__doc__)


if __name__ == "__main__":
    raise SystemExit(main())

"""homm1.graph - the one build graph, and the verbs that drive it.

`python3 -m homm1.graph` (configure) writes **build/build.ninja** from
config/units.toml; `ninja -f build/build.ninja` from the repo root then runs
the whole loop:

    src/ --cl--> base objs --labels--> claims --model--> bindings
                                                      --delink--> target objs
    base objs + target objs --normalize--> comparison copies
                            --project--> objdiff.json --report--> report.json

Every edge invokes an existing module's own CLI (`python3 -m homm1.tool.cl`,
`homm1.retail_labels.source`, `homm1.model`, `homm1.delink.run`,
`homm1.compare.{normalize,project}`, `homm1.tool.objdiff`); this package
owns only the WIRING, plus the two things a graph needs that no module
provides: the `cl` edge driver (homm1.graph.cc) and the candidate-link
policy (homm1.graph.link).

The manifest lives under build/ rather than at the repo root because it is
generated state; ninja's build root stays the repo root, so every path in it
is repo-relative and `ninja -f build/build.ninja` is run from the top.

Incrementality rests on one rule: EVERY producer writes if-changed, and its
rule carries `restat`. An edit that does not change an artifact's content
therefore stops the cascade exactly there - a code-only edit reaches
normalize/report without re-delinking, and a no-op build does nothing.
"""

from __future__ import annotations

#: The emitted manifest, and ninja's build root (the repo root).
NINJA = "build/build.ninja"

#: Compile outputs and the two object trees the comparison pairs.
BASE_DIR = "build/objdiff/base"
TARGET_DIR = "build/objdiff/target-new"
COMPARE_DIR = "build/objdiff/compare-new"
DELINK_RAW = "build/delink/named"

#: Generated model state.
CLAIMS_DIR = "build/gen/claims"
BINDINGS = "build/gen/bindings.tsv"
VIOLATIONS = "build/gen/violations.tsv"

#: The pinned toolchain's identity, as a DECLARED input. $MSVC_DIR, $DXSDK_DIR
#: and the vostok-delinker binary used to be pure environment, which ninja
#: cannot see: re-pinning left 300 objects compiled by the OLD cl and
#: recompiled only newly-edited units with the NEW one - a silently mixed
#: object set, the worst failure mode a byte-matching project has. Writing the
#: identity to a file and hanging the cl/compdb/delink edges off it makes a
#: re-pin invalidate exactly what it should.
TOOLCHAIN_ID = "build/gen/toolchain.id"

#: Stamps for the two edges whose real outputs are a directory the graph
#: cannot enumerate at configure time.
DELINK_STAMP = "build/objdiff/.delink.stamp"
NORMALIZE_STAMP = "build/objdiff/.normalize.stamp"

OBJDIFF_JSON = f"{COMPARE_DIR}/objdiff.json"
REPORT_JSON = f"{COMPARE_DIR}/report.json"

#: Phase 2 (opt-in): candidate image and link map.
CANDIDATE_EXE = "build/exe/HEROES.candidate.EXE"
CANDIDATE_MAP = "build/exe/HEROES.candidate.map"
RESOURCE_SCRIPT = "src/Heroes/Heroes.rc"
RESOURCE_RES = "build/gen/heroes.res"

#: `wine cl` parallelism. Wine serialises far more than it looks under a
#: shared wineserver, and past ~8 concurrent cl.exe the server thrashes and
#: the build slows down; the pool caps the cl edges without capping ninja.
WINE_POOL_DEPTH = 8

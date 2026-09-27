# Reconstruction evidence

Retail bytes and RVAs from [the pinned target](../config/retail/targets.json)
are authoritative. Notes here preserve reviewed layouts, referents, donor
correspondence and source hypotheses. Data evidence admits only identities and
layouts needed by code. Current scores belong in the generated report/ledger.

| Area | Starting points |
| --- | --- |
| Target and compiler | [Target survey](target-survey.md), [release census](executable-function-census.md), [inline EH](vc4-inline-eh.md), [assembly](handwritten-assembly-survey.md), [LZHUF provenance](lzhuf-provenance.md) |
| Graphics | [WinG/DirectDraw](wingraph.md), [palette helpers](palette-graphics.md), [screen blit](screen-blit.md), [bitmap copy](bitmap-copy.md), [icon fill](icon-fill.md) |
| Sound | [Manager lifecycle](digital-driver-startup.md), [volume/inlining](sound-volume-control.md), [sample layout](sample-playback-layout.md), [stream startup](sample-stream-start.md), [CD playback](cd-playback.md), [ambient music](ambient-music.md), [polling](poll-sound.md) |
| UI and resources | [Resource manager](resource-manager-layout.md), [window layout](window-layout.md), [lifecycle](window-lifecycle.md), [Add](window-add.md), [widgets](widget-constructors.md), [readers](widget-readers.md) |
| Input and dispatch | [Mouse coordinates](mouse-coordinates.md), [cursor lifecycle](mouse-pointer-lifecycle.md), [input messages](input-message-handler.md), [executive manager](executive-manager.md) |
| Game models | [High scores](high-score-constructor.md), [owner/Close](high-score-owner-close.md), [player](player-layout-findings.md), [hero deallocation](hero-deallocation-layout.md), [AI construction](philAI-construction.md), [town purchase](philAI-town-purchase.md), [adventure reseed](advmanager-reseed.md) |
| Pathfinding | [Clear/layout](findpath-clear.md), [distance](findpath-quick-distance.md), [terrain cost](findpath-terrain-cost.md) |
| Logging | [String](log-string.md), [two integers](log-two-integers.md) |

## Reviewed tables and generated reports

Keep `homm1-dna-bands.tsv`, `homm1-code-link-bands.tsv` and
`homm2-tu-segments.tsv`: the live census/universe tools consume these reviewed
inputs, and retail referents cite them. `homm1 audit dna-bands` writes a new
review report to `build/gen/`; `--write-config` is an explicit admission step.

Generated donor inventories and candidate/graph/unit snapshots are retired
from this directory. [Tooling audits](../docs/tooling-inheritance.md#repeat-the-review)
and `homm1 init` regenerate reports under ignored `build/`. For new donor
correspondence research, run `python3 -m homm1.labels.donor_align --help`
inside the Nix shell. Supply the target executable, `config/retail/functions.tsv`
and the donor executable/symbol model; direct `--output`, `--graph-output` and
`--segments-output` to `build/`. Candidate similarity does not admit code.
The completed source-stub materializer is retired; reconstruct ordinary C++
through the normal source claims. Historical snapshots remain in Git history.

New evidence should identify the target, addresses, reproduction commands and
what remains uncertain. Keep temporary probes, compiler-state artifacts,
worker handoffs and score histories under ignored `build/`.

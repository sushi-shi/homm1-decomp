# Project documentation

- [Build and comparison](build-system.md), [command map](tooling-map.md),
  [workflow and skills](workflow.md).
- [Compiler evidence/setup](compiler.md), [profiles](compiler-flags.md),
  [candidate linking](linker-flags.md), [relocations](relocations.md).
- [Score tracking](match-status.md), [permutation](permuter.md),
  [donor compiler patterns](patterns/INDEX.md), [clangd](clangd.md).
- [Data attribution](data-attribution.md), [candidate-image checks](image-diff.md),
  [cleanliness](cleanliness-metrics.md), [markers](comment-markers.md),
  [constants work list](constants.md), [enum reuse review](enum-reuse.md).
- [Inheritance and validation](tooling-inheritance.md),
  [script maintenance](../scripts/README.md).

Retail facts are under `config/retail`; build contracts under `config`;
generated state under `build`; reusable compiler mechanisms under
`docs/patterns`. Documentation is
not a runtime input or a substitute for current reports.

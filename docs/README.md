# Project documentation

- [Matching workflow](tooling.md), [build system](build-system.md),
  [command map](tooling-map.md), [repository workflow](workflow.md).
- [Compiler and toolchain](compiler.md), [other builds](builds.md),
  [candidate linking](linker-flags.md),
  [candidate-image checks](image-diff.md), [playing the build](play.md),
  [clean source branch](clean-source.md).
- [Score tracking](match-status.md), [permutation](permuter.md),
  [compiler patterns](patterns/INDEX.md).
- [Cleanliness](cleanliness-metrics.md), [source markers](comment-markers.md),
  [constants](constants.md), [enum reuse](enum-reuse.md), [clangd](clangd.md).
- [Tooling inheritance](tooling-inheritance.md),
  [script maintenance](../scripts/README.md).

Retail facts live in `config/retail`, build contracts in `config`, generated
state in `build`, and reusable compiler mechanisms in `docs/patterns`.

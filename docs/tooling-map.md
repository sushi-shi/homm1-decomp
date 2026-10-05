# Command map

All Python commands use `scripts/homm1` and the shared usage logger.

| Command | Implementation | Purpose |
| --- | --- | --- |
| `configure`, `build`, `match`, `link` | `graph` | Graph, selected-unit loop, full verification, candidate link |
| `play` | `graph.verbs`, `graph.play` | Build, link with resources, install beside local game data and run ([playing](play.md)) |
| `labels`, `model`, `delink`, `compare` | `retail_labels`, `model`, `delink`, `compare` | Claim-to-object comparison pipeline |
| `verify status`, `check`, `bank`, `readme` | `verify.verbs` | Reporting, gates, explicit ledger update, generated README |
| `verify fingerprints`, `verify <gate>` | `verify` | Source hashes and individual gates |
| `sema` | `sema` | Retail addresses, disassembly, xrefs, classes, strings, maps |
| `walls` | `walls` | Inventory, priors, diagnosis, semantic differences and residual heuristics |
| `permute` | `permute` | Classified candidates, campaigns, state trials and source variants |
| `lsp` | `lsp` | clangd references, hover, index and rename |
| `ghidra` | `ghidra` | Viewer export and verification |
| `tool` | `tool` | Individual external-tool drivers and manifest merge |
| `workflow` | `workflow` | Repository hooks and safe staged formatting |
| `clean` | `clean` | Clean source tree, VC4 verification and snapshot branch ([clean source](clean-source.md)) |
| `audit tooling --whole-tree`, `audit usage`, `audit dna-bands` | `audit` | Pinned donor inventory, usage-logging coverage, DNA census |
| `verify lzhuf-oracle` | `verify.lzhuf_oracle` | Runs the retail LZHUF codec under Wine against the Rust port in [`tools/`](../tools/README.md) |

Run each command's help for its current options. `homm1 sema -` accepts batch
queries. [The build guide](build-system.md) defines artifact paths and modes;
[tooling](tooling.md#usage-history) defines invocation logging. No VC5
inline-budget command is advertised as supported when its implementation is
absent.

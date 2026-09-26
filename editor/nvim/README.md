# homm1.nvim

Adapted from the pinned Giten editor integration. The build shell wraps an available
Neovim to load it automatically. Explicit loading also works:

```sh
nvim --cmd 'set rtp^=./editor/nvim' src/SOURCE/KB.cpp
```

For persistent setup, add this checkout's `editor/nvim` to Neovim's runtime path.
No user-wide editor configuration is changed by this repository.

`:Homm1 target`, `base`, `diff`, and `status` show the function under the cursor.
`:Homm1Build` runs the full build; `:Homm1Log` shows editor subprocess launches.
The donor keybindings remain: `vt`, `vb`, `vd`, `vs`, `vx` (xrefs), `vi`
(class), `vg` (definition), `vB` (build), `vq` (close), and `V` (peek).

The address join reads the current `build/gen/bindings.tsv`; source `VA(...)`
addresses are converted to RVAs. Reports and normalized pairs come from
`build/objdiff/compare-new`, including recursive BASE/SOURCE unit names.
The editor uses the same project comparison settings as the command-line report.
A code-mode 100% result does not claim data-reference identity.

`:Homm1 autobuild` enables selected-unit `homm1 match` on save, including claim
refresh and normalization. `:Homm1 autoformat` enables clang-format on save.
Both default off. Settings are local generated state under `build/`.
Clangd uses the generated compilation database and root `.clangd` configuration.

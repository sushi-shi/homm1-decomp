# Repository workflow

The four project skills live under `.agents/skills`: `matcher`,
`wall-identifier`, `holista`, and `permute`. They follow one evidence loop
with HoMM1's VC4 profiles, VA annotations and strict score.
Their reference catalog and `docs/patterns` keep compiler observations that
were not measured on HoMM1 as hypotheses to test, not established behavior.
`CLAUDE.md` and `.claude/skills` link to the canonical instructions and skills;
`.claude/agents/matcher.md` supplies the bounded worker profile.

```sh
nix develop .#build
homm1 workflow setup
homm1 match BASE/MOUSEMGR
homm1 verify readme
homm1 build
```

`workflow setup` installs repository-local Git settings for `.githooks` and
its unit-manifest merge driver. It refuses to replace an unrelated hooks path.
The pre-commit hook formats fully staged C/C++ files under `src` and `include`.
It refuses partially staged files before changing anything, so unstaged edits
cannot enter a commit accidentally. Vendor files are excluded. The configuration does not insert braces or
trailing commas.
Formatting still requires the normal comparison validation before committing.

The unit merge driver preserves independent additions/deletions and refuses
conflicting changes to one unit. It does not decide compiler profiles or
reconstruction ownership. Git history and review still decide those facts.

Root `.clangd` reads the generated compilation database; no global editor
settings are changed.

README status is a build product. `homm1 verify readme` updates only the marked
block, does not bank scores, and refuses comparison-mode ledger mismatches.
`verify check` also refreshes it when run directly. Counts outside that block
must not duplicate generated status. Generated data lives in ignored `build/`.

`homm1 build verify` also links the candidate (`homm1 link`) and runs
`link-diff`: the bytes in which the candidate differs from the retail image,
per region (headers, each section, the trailing overlay, the file size), may
not exceed the ceiling in `config/link_diff.tsv`. Lower counts pass and are
blessed with `homm1 verify link-diff --update`. The editor's candidate
(`homm1 --image editor link`) is checked the same way against
`config/retail/editor/link_diff.tsv`.

# Repository workflow

The four project skills live under `.agents/skills`: `matcher`,
`wall-identifier`, `holista`, and `permute`. They retain the donor's evidence
loop while using HoMM1's VC4 profiles, VA annotations and code-first score.
Their reference catalog and `docs/patterns` preserve donor compiler observations
as hypotheses to test, not established HoMM1 behavior.
`CLAUDE.md` and `.claude/skills` link to the canonical instructions and skills;
`.claude/agents/matcher.md` supplies the bounded worker profile.

```sh
nix develop .#build
homm1 workflow setup
homm1 match BASE/MOUSEMGR
homm1 verify readme
homm1 build
homm1 test
```

`workflow setup` installs repository-local Git settings for `.githooks` and
its unit-manifest merge driver. It refuses to replace an unrelated hooks path.
The pre-commit hook formats fully staged C/C++ files under `src` and `include`.
It refuses partially staged files before changing anything, so unstaged edits
cannot enter a commit accidentally. Vendor files are excluded. Unlike the
donor formatter, this configuration does not insert braces or trailing commas.
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

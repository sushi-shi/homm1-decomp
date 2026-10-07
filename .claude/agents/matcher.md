---
name: matcher
tools: Bash, Read, Edit, Write, Grep, Glob, LSP
description: Reconstruct HoMM1 functions against the pinned Buka 2003 HEROES.EXE and EDITOR.EXE with VC6 SP5. Give a bounded target and an isolated worktree when using multiple workers. Follows AGENTS.md and the shared matcher skill.
---

Read `AGENTS.md` and `.agents/skills/matcher/SKILL.md`. Work only on the assigned
target and in the assigned worktree, setting `HOMM1_DIR` to that root.
Use `wall-identifier` to diagnose a plateau and `permute` for qualified residue.
Perform the work directly; this worker does not spawn additional workers.

Iterate with `homm1 match <unit>` inside `nix develop .#build`. Follow the root
validation requirements: build after source/tooling changes and test tooling.
The comparison is strict (`data_matching = true`): data-reference identities
and addends count.
Do not commit unless the caller requests it; preserve concurrent changes.

Report per target: MAX before/after, structural correction, retail evidence,
compiler controls, code-referent verdict, and any remaining wall.

---
name: holista
description: Recover the inline helpers, accessors, and macros hidden inside HoMM1 functions. Read one function at a time, mark where its statements drop below the abstraction level of the code around them (raw member arrays, container internals, repeated expressions, scoped blocks), restore the helper the original developers called, and apply it across every site. Use for helper-recovery passes over random functions, for hard walls that may be an open-coded helper, and whenever a function mixes domain operations with the internals of another class.
---

Use HoMM1's pinned VC4 profile and absolute `VA(...)` source annotations.
The active score is strict (`data_matching=true`): data-reference identities
and addends count. `AGENTS.md` and the user's instructions take precedence.


# holista — recover the helpers behind a function

Era developers wrote accessors, small inline helpers, and macros; the compiler expanded
them. A reconstruction that transcribes the expanded body reads fine but can miss
the helper's own statement boundaries, local scopes, temporaries, and evaluation
order, which is often exactly what a stubborn wall is. Restoring the helper is
correct source whether or not a score moves; when enough helpers are restored,
hard functions start to close.

```diff
-    if (target != 0 && target->m_skillLevel[m_skill] <= m_level) {
-        if (target->m_skillLevel[m_skill] == 0)
+    if (target != 0
+        && target->getSecondarySkill(TSecondarySkill(m_skill)) <= m_level) {
+        if (target->getSecondarySkill(TSecondarySkill(m_skill)) == 0)
             target->giveSS(m_skill, m_level);
```

The caller works with skills; it should not index another class's array. The
helper is called twice, not cached: that is the original shape.

`AGENTS.md` governs (source rules, CUR/MAX/HIST, the fast loop). This skill is
the method.

## 1. Pick a function

Sample, do not hand-pick: a random function reveals habits the worklist misses.

```sh
grep -v '^#' config/match_baseline.tsv | shuf -n 1
```

Any MAX is fair game: an exact function with an open-coded helper is still
wrong source, and its sites teach you the helper for the hard ones. Read the
whole function, its class header, and its callers and callees.

## 2. Read the abstraction levels

Annotate each statement with the level it works at, and look for statements
below the level of their neighbours:

- **Another class's internals:** indexing a member array, walking a list's
  nodes, testing flag bits or enum ranges of an object the function does not
  own. The owner almost certainly had an accessor or predicate
  (`getSecondarySkill`, `IsAlive`, `GetAt`, `HasFlag`).
- **Repeated expressions:** the same field path or computation written twice
  instead of held in a local. An original helper call per use looks exactly
  like this; a transcriber would have cached it.
- **Scoped blocks:** a `{ ... }` block with its own locals in the middle of a
  function is the footprint of an inlined helper body (or a macro). Its
  locals' lifetimes, EH states, and stack homes come from that boundary.
- **Casts and conversions at a call site:** `TSecondarySkill(m_skill)`-style
  conversions show the helper's typed parameter.
- **Idioms:** clamp/min/max, range checks, bit packing, coordinate math,
  lookup-with-default. These were helpers or macros far more often than not.

`homm1 walls abstractions --unit <unit>` classifies sub-100 rows by level;
its `call` and `textual` rows are leads for this pass, not proof.

## 3. Find the helper's evidence

A helper needs evidence, not taste:

- **Repetition:** `rg` the raw pattern across `src/` and `include/`. The same
  expression at several sites, especially in different classes, is the
  strongest signal. Count the sites.
- **Existing helpers:** check the owning class and its headers first; the
  helper may already exist unused, or exist for a sibling field.
- **Sibling code:** similar code elsewhere in the tree is a hypothesis until
  HoMM1 retail confirms it.
- **Retail shape:** a repeated load instead of a reused register, a reload
  after a call, or scope-shaped stack homes corroborate a call per use.

Name the helper for what it means in the owner's domain, put it in the
owning class's shared header (inline member, or a macro where the family uses
macros), and give it the typed signature the call sites imply.

## 4. Apply everywhere and measure

1. Replace every site of the pattern, not just the sampled function.
2. `homm1 build` (the change spans units), then read the MAX report.
3. A function that was MAX = 100 must stay 100 after the rewrite. If it drops,
   the helper's shape is wrong somewhere (by value versus reference, const,
   parameter type, statement split, member versus free function); fix the
   shape rather than abandoning the helper, or leave that site unconverted
   and note why.
4. Sub-100 functions may dip on the first shape; compose further levers from
   the helper base before concluding (`AGENTS.md`, matching rules).
5. Keep each helper and its justified sites together for review; commit when requested.

A helper that keeps every exact function exact is kept even if no score rises:
it is the original source layer. Do not invent helpers without repetition or
retail evidence, and never add one only to move a score.

## 5. Report

For each sampled function: what it mixed, which helper(s) were recovered (name,
owner, site count), MAX changes (new exacts, drops you fixed or left), and any
helper you suspected but could not justify.

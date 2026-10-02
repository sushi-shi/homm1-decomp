# VC4 float expression shape

Measured with the HoMM1 VC4 `/Od /G5 /Ob1` profiles (PHILAI, 2026-10).

- **Store-then-compare.** For `x = <float expr>; if (x > y)` VC4 keeps the
  value on the x87 stack and emits either `fcom y; fstp x` or
  `fst x; fcomp y`. Which one is decided by the declaration order of `x` and
  `y`, not by the statement text: `philAI::GetBestHero` became exact by
  declaring `adjusted` before `bestScore` (different hash buckets, so the
  stack slots did not move). Splitting `if ((x = e) < y)` into two
  statements did the same for `QuickCombat`.
- **Product chains are reassociated.** An unparenthesized chain such as
  `n * d * d * gfBonus` is emitted with the global factor first
  (`fild n; fmul gfBonus; fmul d; fmul d`). Parenthesizing the local part,
  `(n * d * d) * gfBonus`, keeps the written order
  (`ProbableOutcomeOfBattle`, human-bonus arm).
- **Cast temporaries.** `diff = (float)(c ? a - b : b - a);` stores each arm
  to a float temporary and copies it with an integer `mov`; the same
  expression with `static_cast<float>` or no cast stores straight into
  `diff` (`QuickCombat`).

Operand order of commutative integer compares/adds and 2-D index order still
move with unrelated TU state; use `homm1 permute state` for those once the
shapes above are settled.

Not established: a general declaration-order rule. In `FightValueOfStack`
declaring `spellScore` first made it the loaded operand of an integer compare,
while reordering `RVOfPosition` flipped unrelated multiplies the other way.
Treat declaration order as a lever to A/B, not as a predictor.

# SPELLAI equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/SPELLAI.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?EffectSpellDamage@combatManager@@QAEXPAHHHH@Z` | 99.982 | IDENTICAL | mirrored compare | 0x309: `cmp b16, w11+r; jg` vs `cmp w11+r, b16; jl`. |

# HISCORE equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/HISCORE.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?Update@highScoreManager@@QAEXXZ` | 96.335 | IDENTICAL | commutative operand order; register renaming | 0x379-0x3a0: `w30/3 + w44*7` with the two terms computed in the opposite order; edx difference is dead. |

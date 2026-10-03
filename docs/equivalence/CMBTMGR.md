# CMBTMGR equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/CMBTMGR.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?CatAttack@combatManager@@QAEXC@Z` | 99.990 | IDENTICAL | commutative operand order | 0x4d8/0x6de/0x87e add operands swapped; 0x88c imul operands swapped. |
| `?KeepAttack@combatManager@@QAEXXZ` | 99.997 | IDENTICAL | commutative max | 0x40a: max of [ebp-0x50]/[ebp-0x48] with operands swapped. |

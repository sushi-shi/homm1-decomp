# COMMAND equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/COMMAND.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?ShowDeadArmies@combatManager@@QAEXPAVheroWindow@@@Z` | 99.992 | IDENTICAL | commutative operand order | 0x5ac/0x6a2: `imul` operands swapped. |

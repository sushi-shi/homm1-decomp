# EVENTS equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/EVENTS.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?DoEvent@advManager@@QAEXPAVmapCell@@HH@Z` | 99.693 | IDENTICAL | commutative operand order | 0x126b/0x134c: cell index `720*x+10*y` operands swapped; 0x12a3/0x1384: `abs(dx)+abs(dy)` order swapped. |

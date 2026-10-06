# FINDPATH equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/FINDPATH.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?FindCombatPath@searchArray@@QAEFFFPAVarmy@@C@Z` | 99.425 | IDENTICAL | register renaming | esi/edi swap army pointer and start hex throughout; 0x288-0x2b6 node pointer/counter in eax/ecx swapped (`and cl` vs `and al` gives the 2-byte size difference); i16 arg high halves unused. |
| `?PushPoint@searchArray@@QAEXFFGGGCCCCCCC@Z` | 99.588 | IDENTICAL | commutative operand order; mirrored compare | 0xc7: `(high+low)>>1` operands swapped; 0xed: `cmp high,low; jle` vs `cmp low,high; jge`. |
| `?TestPossibleDirections@searchArray@@QAEXFFQACQAEFH@Z` | 99.945 | IDENTICAL | commutative operand order | 0xee/0xf7: `gSearchNeighborY` and `gpGame` loaded into ecx/edx swapped, then summed into one address. |

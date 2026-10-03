# ADVMGR equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/ADVMGR.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?ComboDraw@advManager@@QAECFFC@Z` | 99.424 | IDENTICAL | mirrored compare | 0x9cc/0x9f9/0xab8: extent clamps `cmp v,Max; jle` vs `cmp Max,v; jge` etc. |
| `?DrawCell@advManager@@QAEXFFFFCCC@Z` | 99.879 | IDENTICAL | commutative operand order; register renaming | 0x57a: `s_drawGroundTile |= byte` with operands in swapped registers; the other register dies at the following cdecl call. 88 other hunks are displacements. |
| `?HeroQuickView@advManager@@QAEXCCFF@Z` | 99.995 | IDENTICAL | commutative operand order | 0x742/0x833: `imul` operands swapped. |
| `?Main@advManager@@UAEFAAUtag_message@@@Z` | 99.996 | IDENTICAL | commutative operand order (equality) | 0x87: giHostGamePos/giThisGamePos compare swapped before `jne`. |
| `?TownGate@advManager@@QAEXXZ` | 99.979 | IDENTICAL | commutative operand order | 0xb5/0xf0: `abs(dx)+abs(dy)` with the two pure `abs` calls swapped. |
| `?TownQuickView@advManager@@QAEXCCFF@Z` | 99.994 | IDENTICAL | commutative operand order | 0x563/0x6c7: `imul` operands swapped. |
| `?ViewPuzzle@advManager@@QAEXXZ` | 99.172 | IDENTICAL | commutative operand order | 0x18e: `5*b57 + 2*b56` built from swapped registers. |

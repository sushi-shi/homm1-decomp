# REQUEST equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/REQUEST.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?DoKnob@fileRequester@@QAEXXZ` | 98.562 | IDENTICAL | mirrored compare | 0x100: `cmp y+0x38, base+0xd4; jle` vs `cmp base+0xd4, y+0x38; jge`. |
| `?Open@fileRequester@@UAEFF@Z` | 99.206 | IDENTICAL | commutative operand order | 0x24d: `[eax+ecx+3]` address operands swapped. |

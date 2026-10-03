# wingraph equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/wingraph.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?WGAppPaint@@YAHPAX0@Z` | 98.310 | IDENTICAL | commutative operand order | 0x11f/0x128: `add` operands swapped. |

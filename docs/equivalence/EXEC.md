# EXEC equivalence survey

Manual survey of the non-exact functions of `src/BASE/EXEC.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?ShutDownSystem@executive@@QAEXXZ` | 99.792 | IDENTICAL | commutative operand order (equality) | 0x25: `cmp eax,ecx` vs `cmp ecx,eax` before `je`. |

# KB equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/KB.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?HandleRemoteDeadPlayerExit@@YAXH@Z` | 97.273 | IDENTICAL | commutative operand order (equality) | 0x6: `cmp [giThisGamePos],arg` vs swapped, `jne`. |
| `?HandleRemoteSuddenExit@@YAXXZ` | 99.841 | IDENTICAL | commutative operand order (equality) | 0x52: game-position compare swapped, `jne`. |
| `?InterpretCommandLine@@YAHXZ` | 99.112 | IDENTICAL | mirrored compare | 0x226: `cmp i,n; jge` vs `cmp n,i; jle`. |
| `?UpdateAppSpecificMenus@@YAXPAX@Z` | 92.000 | IDENTICAL | commutative operand order (equality) | 0x6: `cmp arg,[gAdventureMenu]` vs swapped, `jne`. |

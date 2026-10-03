# SEARCH equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/SEARCH.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?FindNearestObject@searchArray@@QAEFFFFFE@Z` | 92.444 | IDENTICAL | register renaming; independent reorder; dead upper bits | 0x55-0x77 node copy with ecx/edx swapped; 0x97-0xb1 x/y in cx/ax; 0x12a-0x14b: `x+dir.x`, `y+dir.y` and the [esp+0x1c] argument computed around `add esp,0x10` with matching esp-relative offsets; i16 PushPoint args differ only in unused high halves. |
| `?SeedPosition@searchArray@@QAEXFFFFHHHHHHHH@Z` | 98.746 | IDENTICAL | register renaming; mirrored compare; dead spill | 0x412-0x452: x/y in di/ax vs ax/di; the scratch slot [esp+0x10] holds x vs y but is only read inside this block; compares identical. 0x613: `cmp cell,cur+12; jge` vs `cmp cur+12,cell; jle`. |

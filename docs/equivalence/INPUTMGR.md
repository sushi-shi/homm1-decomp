# INPUTMGR equivalence survey

Manual survey of the non-exact functions of `src/BASE/INPUTMGR.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?Open@inputManager@@UAEFF@Z` | 99.750 | IDENTICAL | register renaming; dead upper bits | 0x69: strcpy length kept in edx vs eax; i16 return uses ax only. |

# ICONWDGT equivalence survey

Manual survey of the non-exact functions of `src/BASE/ICONWDGT.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?Main@iconWidget@@UAEFAAUtag_message@@@Z` | 99.562 | IDENTICAL | mirrored compare | 0x87: `cmp bx,ax; jl` vs `cmp ax,bx; jg`. |

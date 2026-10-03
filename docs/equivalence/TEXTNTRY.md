# TEXTNTRY equivalence survey

Manual survey of the non-exact functions of `src/BASE/TEXTNTRY.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?Main@textEntryWidget@@UAEFAAUtag_message@@@Z` | 99.485 | IDENTICAL | mirrored compare | 0x83/0x8c/0xe2/0xef: `jl`↔`jg`; 0x4d8: `jle`↔`jge`. |

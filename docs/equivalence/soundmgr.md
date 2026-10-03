# soundmgr equivalence survey

Manual survey of the non-exact functions of `src/BASE/soundmgr.cpp` against retail
`HEROES.EXE` (master 44a4447, strict comparison). "ours" is the
recompiled object, "retail" the delinked target; offsets are
function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `?Open@soundManager@@UAEFF@Z` | 99.685 | IDENTICAL | register renaming | 0x128-0x14f: sample-handle loop pointer/counter in ebx/edi swapped. |
| `?PlayAmbientMusic@soundManager@@QAEXHHH@Z` | 95.583 | IDENTICAL | register renaming; independent reorder | 0x57: CDPlay args pushed in the same order from swapped registers; 0x136/0x15b: AIL_serve pointer and counter in ebp/edi swapped; 0x20f-0x24b: volume in ecx vs eax, StartSample args identical (byte flag goes to an i16 parameter). |
| `?SetMusicQuality@soundManager@@QAEXH@Z` | 99.759 | IDENTICAL | register renaming | 0x82: AIL_serve pointer/counter in ebx/edi swapped. |
| `?StartSample@soundManager@@QAEPAU_SAMPLE@@PADPAPADFFHHH@Z` | 99.930 | IDENTICAL | register renaming | 0x8e: AIL_serve pointer/counter in ebx/edi swapped; both registers rewritten before reuse. |
| `?StopAllSamples@soundManager@@QAEXXZ` | 99.722 | IDENTICAL | register renaming | 0xb7: AIL_serve pointer/counter in edi/esi swapped. |
| `?StopSample@soundManager@@QAEXPAU_SAMPLE@@@Z` | 99.355 | IDENTICAL | register renaming | 0x24: AIL_serve pointer/counter in esi/edi swapped. |
| `?SwitchAmbientMusic@soundManager@@QAEXH@Z` | 97.439 | IDENTICAL | register renaming | Inlined playing-state test returns in eax vs ecx on every path (0x40, 0x5e, 0xf6, 0x109, 0x126, 0x137) and is only tested. |

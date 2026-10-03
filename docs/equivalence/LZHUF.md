# LZHUF equivalence survey

Manual survey of the non-exact functions of `vendor/lzhuf/encoder.cpp` against retail
`HEROES.EXE` (master d1fdb30, strict comparison). Every differing
instruction pair was classified; "ours" is the recompiled object, "retail"
the delinked target. Offsets are function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `EncodeData` | 97.817 | IDENTICAL | mirrored compare; register renaming; independent reorder (distinct arrays); dead upper bits | 0x19c: `cmp len,match_length; jge` vs `cmp match_length,len; jle`. 0x6ad: `cmp di,bp; jl` vs `cmp bp,di; jg`. 0x3d0-0x4a7 (inlined tree update): ours reads `son[c]` before storing `freq[l]=k`, retail after; `freq`, `son` and `prnt` are distinct arrays, so the load cannot observe the store. Every store has the same address and value in both (`freq[c]=freq[l]`, `freq[l]=k`, `prnt[i]=l`, `prnt[i+1]=l` if i<T, `son[l]=i`, `prnt[j]=c`, `prnt[j+1]=c` if j<T, `son[c]=j`), in the same order. Retail `lea ecx,[ebp+1]` vs ours `inc cx`: only cx is ever read. The post-loop code writes every register before a full-width read. |
| `InsertNode` | 99.245 | IDENTICAL | mirrored compare; commutative operand order (equality) | 0xa4: `cmp bp,[match_length]; jle` vs `cmp [match_length],bp; jge`. 0xd0: same operand swap followed by `jne` in both. |
| `UpdateEncoderTree` | 99.676 | IDENTICAL | register renaming | si and di swap roles for the two loop counters (0x16, 0x34, 0x5e). |

3 functions: all IDENTICAL.

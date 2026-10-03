# REMOTE equivalence survey

Manual survey of the non-exact functions of `src/SOURCE/REMOTE.cpp` against retail
`HEROES.EXE` (master d1fdb30, strict comparison). Every differing
instruction pair was classified; "ours" is the recompiled object, "retail"
the delinked target. Offsets are function-relative.

| function | CUR | verdict | residue classes | evidence |
|---|---|---|---|---|
| `Connect` | 99.948 | IDENTICAL | commutative operand order (equality compare) | 0x231/0x240: ours divides `stime` then `oldsec` by 1000, retail `oldsec` then `stime`; the quotients are only compared with `cmp ecx,eax; je`. The divisor is the constant 1000, so neither division can trap. |
| `TransmitRemoteData` | 99.922 | IDENTICAL | commutative operand order (equality compare) | 0x174: `mov eax,[gLastConfirm]; cmp [gIDCtr],eax` vs operands swapped; followed by `jne` in both. |
| `WaitForDirectConnect` | 99.953 | IDENTICAL | commutative operand order (equality compare) | 0x24d/0x25c: the same `stime/1000` vs `oldsec/1000` order swap as `Connect`, compared with `cmp ecx,eax; je`. |

3 functions: all IDENTICAL.

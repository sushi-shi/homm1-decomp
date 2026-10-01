# VC4.0 /Od local stack-slot order

Local names decide the `/Od` frame layout of HoMM1 functions. The pinned VC4.0
compiler does not follow the HoMM2-derived model in
`scripts/homm1/core/od_slots.py` (`(v >> 4) + v * 4 + c`, folded to 16 bits,
low four bits). Observed HoMM1 candidate objects instead fit:

```python
def bucket(name):
    v = 0
    for c in name:
        v = ((v >> 7) + v * 4 + ord(c)) & 0xFFFFFFFF
    return v % 16
# shallowest slot first: ascending bucket, newest declaration first in a bucket
```

## Measurements

Controlled A/B compiles of `SOURCE/kbwin` (`homm1 sema frame`) with fixed
bodies and changed local names or declaration order:

- `FindToken` with `len` plus `i`, `index`, `k`, `n` or `j` always put `len`
  at `ebp-4`; renaming the index to `pos` moved it to `ebp-4`. Swapping the
  declaration order of `len` and `i` did not change the frame.
- `SetMenus` with the PoL names (`count`, `commandId`, `scanPosition`,
  `commandPosition`, `disableFlag`, `index`) produced
  `count, index, scanPosition, commandPosition, disableFlag, commandId` in
  three declaration orders; only the equal-bucket pair moved, newest first.
- Renaming single locals (`count`→`itemCount`, `disableFlag`→`flag`,
  `commandId`→`id`) moved exactly the renamed slot as the formula predicts.

A brute-force search over shift/multiply hash families and moduli found this
formula as the only fit for all fifteen observations. Applied to every
multi-local frame of PHILAI, KB, GAME, wingraph, HISCORE, REQUEST, TOWNMGR
and Misc it predicts 19 of 24 frames; the existing model predicts 8. The five
misses involve `this`, an aggregate local, a nested block, or two functions
sharing one name, so those cases are not established by this note.

Choosing names with this formula made `SetMenus`, `FindToken` and
`FindLastToken` exact against retail. The names themselves remain
reconstruction choices; retail carries no local symbols.

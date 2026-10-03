# VC4 sortnode is a deterministic, replayable function of C1 handles

HoMM1 VC4 (`C2.EXE` of the pinned `vc40` toolchain), measured with an in-process tracer.
Extends [operand sort keys](vc4-operand-sort-key-is-the-symbol-handle.md),
[/O2 register allocation](vc4-global-register-allocation-is-chaitin-briggs.md) and
[control flow consumes handles](vc4-control-flow-consumes-c1-handles.md).

Signature: an operand-order residue (2-D index halves, `cmp`/`add`/`imul` operands, `fld`/`fadd`) that
flips with unrelated TU edits. Use this entry when you want to *compute* which handle state gives the
retail order instead of searching for it.

## Mechanism (read from C2.EXE, every step confirmed by trace)

- Driver: `/Od` sorts each statement tree once (`0x475755`); `/O2` repeats the walk until no rewrite
  fires (`0x408b40`, at most 100 passes).
- Walker `0x408d62(node, parent)`: if the op has table flag `0x2000` (binary), walk the **right** child
  first, then the left one (unless the op is a leaf), then compute the node weight (`0x40846e`).
  Op flags are the words at `0x484828[op]`: bits 0..1 class (0 binary, 1 unary, 2 leaf, 3 binary
  with rank +0x70), `8` commutative, `0x10` comparison, `4` associative chain (add 0x2, mul 0x4,
  and/or/xor 0xb..0xd).
- Weight, 16 bits, `rank & 0xfff0 | hash & 0xf`:
  - symbol leaf (IL `0x26`): `0x10 | k4(handle)`, where `handle` is the C1 handle stored at
    symbol `+0x24`; `0x10` when the leaf has no symbol or the symbol kind byte is `0x18`.
  - other leaves: constants hash their value (handle-free); anything else `0x10 | (op>>4)-op`.
  - unary: `rank = child + 0x10` for load `0x30`, child passes through for `0x31` and for the
    conversion `0x34` when `0x4138c3` strips it, else `child + 0x20`; `hash = child + (op>>4) - op`
    with `op` a signed byte.
  - binary: `rank = (L + R + 0x20) & 0xfff0` (`+0x70` for class 3), `hash = L + R + (op>>4) - op`;
    op `0x44` takes R.
- The old "k4(handle + 0x3000)" leaf key is this load hash: `k4(h + 0x3000) == (k4(h) + 3) % 16`
  for every 16-bit h, and `(0x30>>4) - 0x30 = -45 = 3 (mod 16)` is the load node's term. The
  2-byte / 1-byte "rotations" 15 / 14 are the widening conversion `0x34` (`+15` each) applied once
  (short to int) or twice (char to short to int); each conversion also adds `0x20` to the rank, which is
  why a 4-byte leaf against a narrow one is decided by rank.
- Exchange `0x413ded`: swap the children when `R > L` (unsigned); a comparison then takes the mirrored
  relation from table `0x481c0c` (`==`/`!=` unchanged, `0x21 <-> 0x23`, `0x22 <-> 0x24`).
- Reassociation `0x413e27` (associative ops, integer types): for `(a op b) op c`, if
  `weight(b) < weight(c)` the tree becomes `(a op c) op b` (builder `0x40ed62` pattern `PPLYPLY`) and the
  left subtree is re-walked; float/pointer-typed chains take extra checks.

Consequences:

- The sorted tree is a pure function of the leaf handles plus handle-free constant weights.
- The IL order that reaches sortnode can itself change with handles (C1 or earlier C2 passes).
  The exchange is a comparison, so the result does not depend on that order except on exact ties.
  Every measured state agreed.

## Measurements

- In-process tracer (ptrace is unavailable: yama `ptrace_scope=2` blocks winedbg launch and attach
  and gdb attach). Tracepoints
  at `0x408daf` (node weight), `0x408588` (leaf handle), `0x413ded` (exchange), `0x413e5c` (rotation),
  `0x47576f`/`0x408b61`/`0x408bc4`/`0x408bec` (tree roots, with the function's C1 handle
  `[[[0x48173c]+4]+0x24]`).
- A replay of the traced sort reproduces every traced weight, exchange and rotation:
  - `SOURCE/ARMY`: 1338 trees.
  - `BASE/WINMGR` (`/O2`): 1007 walks.
- Prediction runs the replay from one trace with a pure handle map `+N` above the
  first handle the `.cpp` allocates, and compares the predicted sorted trees with a real traced
  compile of the shifted TU. All of them matched:

  | TU | shifts | predictions | changed trees |
  |---|---|---|---|
  | ARMY | 1, 2, 3, 5, 8, 13 | 8028 | 193 |
  | ADVMGR | 1..16 | 69136 | 326 |
  | WINMGR | 1, 3, 7, 12 | 4028 | 79 |
- Retail residues:
  - `army::PowEffect` (78.6 CUR, 2-D index pairs `m_armies[side][stackIndex]`): 5040 declaration orders
    keep the /Od slots, and the replay sorts them into 12 outcome classes. Compiling one
    representative per class (12 compiles) found the class that matches retail. It moves
    `stackIndex` last (`frames armyNum step longest cellHex curArmy side stackIndex`). The function
    is then exact apart from a relaxed data addend and padding.
  - Over 64 top-of-TU typedef shifts, the predicted class of PowEffect and DoHydraAttack determined
    the compiled score exactly: equal class always gave equal score.
  - `advManager::GetCell` (`m_mapData[x][y]`, parameters against a member): retail needs the
    parameter handles shifted by +1..+14 relative to now; +15/+16 revert. The prediction matched
    16 real compiles.
  - `army::DoAttack` involves member pairs (`m_side/m_index`, `m_occupantSide/m_occupantIndex`) from
    two headers. Under whole-TU shifts it has 42 outcome classes in 64 states, each predicted
    correctly; none reaches retail (best 16 diff lines). The residue needs relative member-handle
    changes, which only header order or content supplies.
- `/O2` register colouring, measured on a micro probe of four `int` locals live across calls:
  ties follow C2 ids `0x4020+`, which are assigned in **C1 handle & 31** order. The register
  assignment rotates exactly when a local's handle crosses `0xc0`: pads 15/16/17 rotate, 18 restores.
  This confirms the Chaitin-Briggs entry's tie key.
- `/Od` slots: C1 writes the `sy` records of locals in `od_slots.slot_order` (hash bucket ascending,
  newest first). C2 assigns frame slots in that record order (checked on PowEffect's eight locals).

## Using it

The tracer and replay scripts were research tooling and are not kept in the
tree; the mechanism above is what carries over.

1. For a local operand residue, the outcome depends only on the locals' handle
   order: enumerate declaration orders that keep the `/Od` slots, group them by
   predicted sorted tree, compile one representative per class and keep the class
   that matches retail. Pick an authentic declaration order inside it.
2. For a parameter, member or global residue, find the handle window that
   produces retail's outcome and look for authentic earlier-TU content
   (declaration order, or a control-flow spelling per the control-flow entry)
   that supplies a shift inside it. Do not add declarations only to shift
   handles.
3. Several functions share one TU prefix, so evaluate all of a TU's residues
   against the same shift before choosing.

## Not established

- The constant-weight formula: replayed from the trace, not re-derived.
- The rotation's float/pointer branch (type word `& 0xc00`): flagged, unmodelled.
- Whether C1's own pre-sort order can matter on exact ties: no counterexample yet.
- A full `/O2` colouring replay from traced interference graphs. The select phase is at
  `0x40cbe1` (node `+0x18` id, `+0x16` weight, register).

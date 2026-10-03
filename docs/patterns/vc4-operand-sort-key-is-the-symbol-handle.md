# VC4 sorts commutative operands by a hash of their C1 symbol handles

HoMM1 VC4 (C2.EXE of the pinned `vc40` toolchain), measured. Signature: a row
whose whole residue is two operand loads in the opposite order
(`mov eax,[a]; cmp [b],eax` vs `mov eax,[b]; cmp [a],eax`,
`movsx eax,[a]; movsx ecx,[b]` swapped, `fld [a]; fadd [b]` swapped, or the two
halves of a 2-D index), unaffected by writing the expression the other way
round, and flipping when unrelated declarations are added earlier in the TU.

## Mechanism

C2's `sortnode` pass gives every expression node a 16-bit weight and, for a
commutative operator or a comparison, exchanges the children when
`weight(right) > weight(left)` (unsigned; comparisons are reversed).  Equal
weights keep the source order.  The heavier operand becomes the left one and is
evaluated first.  Addresses in `C2.EXE`: weight of one node `0x40846e`,
recursive walker `0x408d62`, exchange `0x413ded` (assert string `sortnode.c`).

`weight = rank | hash`, `rank` in bits 4..15, `hash` in bits 0..3:

| node | rank | hash |
|---|---|---|
| symbol (IL `0x26`) | `0x10` | `k4(handle)` |
| constant (IL `0x33`) | 0 | function of the value |
| unary (load, convert) | child + `0x20` (`0x10` for IL load `0x30`) | child + `(op>>4) - op` |
| binary | `(L + R + 0x20) & 0xfff0` | `L + R + (op>>4) - op` |

```python
def k4(v):                      # v = C1 symbol handle (symbol +0x24 in C2)
    a = v >> 8
    e = ((a - v) & 0xffffffff) >> 4
    return (e - a + v) & 0xf    # == floor(15*u/16) % 16, u = v - (v >> 8)
```

A symbol leaf always sits under a load (IL `0x30`), and the load adds
`(0x30>>4) - 0x30 = 3 (mod 16)`.  The key the measurements fitted,
`k4(handle + 0x3000)`, is that sum: `k4(h + 0x3000) == (k4(h) + 3) % 16` for every
16-bit `h` ([replay entry](vc4-sortnode-is-a-replayable-function-of-handles.md)).
For two same-width leaves this reduces to comparing `(k4(h + 0x3000) + rot) % 16`, with
`rot` 0 for 4-byte operands (int, long, pointer, float), 15 for 2-byte and 14 for
1-byte operands.  These rotations are the widening conversion `0x34`, which adds 15
once for short to int and twice for char to short to int.  Each conversion also adds
`0x20` to the rank.  Global, parameter and local leaves use
the same formula.  A 4-byte leaf against a narrow leaf is decided by rank, not by
handles.  Because the binary rank adds the children's hashes, a carry out of the
low nibble can also change a rank.

C1 assigns handles sequentially over the whole TU: each typedef, extern, global
or forward declaration takes one, a prototype or enum two, and class/struct
definitions more.  Within a function the parameters come first, then the
leading locals **one consecutive handle each, in declaration order** (arrays and
aggregates included).  Statement labels are allocated while statements are
parsed, so locals declared inside later blocks receive handles after those
labels.

## Measurements

- C2-only replay of a fixed C1 IL capture (`c1xx -il P`, then `c2 -il P`), with
  only the two locals' handle numbers patched: 4,900 operand-order outcomes
  across handles `0xbb..0x8bb` agree with `k4(handle + 0x3000)` with no
  exceptions.  Patching handles while keeping the IL order unchanged flips the
  output, so declaration order matters only through the handles.
- Width rotations: twenty-state typedef sweeps for each operand type fit only
  `rot = 0/15/14` for 4/2/1-byte operands.
- In the real `SOURCE/CMBTMGR` TU, `KeepAttack` has seven such pairs.  The model
  predicted all 105 outcomes of a fifteen-state typedef sweep.  No sweep state
  reproduced retail, because sweeping shifts every handle together.  Changing the
  relative order did reproduce it.

## Using it

1. Confirm the residue is an operand pair whose instructions are otherwise
   identical.  Find its leaves' handles: run C1 with `-il <prefix>` (the command
   line is printed by `cl /Bd`).  Locals and parameters are `01 02|01 <u16 handle>
   00 <name>` records in the `sy` stream.  Globals appear in `gl` as
   `<u16 handle> 00 ?name@@3...`.
2. If a movable local is involved, look for a declaration order that gives every
   swapped pair the retail order and leaves every matching pair unchanged.  The
   /Od stack-slot rule restricts the search only within a name-hash bucket:
   `homm1.core.od_slots.slot_order` must still return the same order.  Compile
   each candidate in the real TU.  For non-leaf operands (2-D indices,
   `(1 << i) & mask`), hill-climb by compiling each candidate move of a local
   named in the residue.
3. The source operand order matters only when the weights tie.
4. If no movable local is involved (a parameter against a global or member
   chain), the outcome depends on TU-prefix handles or on rank.  If a typedef-count
   sweep before the function is flat, rank decides and the source structure differs.

Local declaration order is otherwise unconstrained, so choosing one that
reproduces retail is a reconstruction choice like choosing names that reproduce
stack slots.  The absolute handles still depend on everything before the
function.  An authentic edit earlier in the TU can therefore flip a fitted
function again; the ledger MAX keeps its exact peak.  Do not add declarations to
shift handles.

## Not established

- Whether other symbol classes take a different path through the leaf weight. A
  symbol of kind byte `0x18` weighs `0x10` with no hash; the others measured use
  `k4(handle)`.
- Exact constant and operator hashes for every IL opcode.  The table above is read
  from the disassembly; only the leaf, width-rotation and tie rules were measured
  in isolation.
- `/O2` units.  The same `sortnode` pass runs and replays exactly (the replay entry), but the
  optimizer and register allocator change the emitted order afterwards.

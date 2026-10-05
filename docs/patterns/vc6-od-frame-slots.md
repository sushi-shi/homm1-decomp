# VC6 /Od frame slots follow the folded name hash

The Buka VC6 SP5 `/Od` compiler assigns named locals to stack slots by a hash
of their spelling, not by declaration order. The rule is the one HoMM2 measured
on the same compiler; the NWC branches' VC4.0 variant does not apply:

```text
h = 0; for each character c:  h = (h >> 4) + h * 4 + c      (32-bit)
bucket = ((h ^ (h >> 16)) & 0xffff) & 0xf
```

Slots are assigned from the first local slot downwards (below the EH record in
`/GX` functions) by bucket ascending; within one bucket the later declaration
comes first. Each inner block scope is a separate table: its locals follow all
function-scope locals, one block after another in source order, each sorted by
the same rule. Expression temporaries, inline-expansion slots and the `this`
spill follow the named locals.

## Controls

A probe compile with the pinned toolchain (`/Od /Ob1 /GX /MT /G5 /Z7`):

| Declaration order | Buckets | `S_BPREL32` order, first slot first |
| --- | --- | --- |
| `a, b, c, d` | 1, 2, 3, 4 | `a, b, c, d` |
| `attackMask, oldSide, oldIndex, j` | 12, 9, 5, 10 | `oldIndex, oldSide, j, attackMask` |
| `fileId, highByte, size, buffer, i` | 1, 4, 1, 11, 9 | `size, fileId, highByte, i, buffer` |

Across the reconstructed tree, every `/Od` unit compiled with `/Z7` and its
local declarations read through libclang: the rule reproduces 835 of 839
functions without inner-block locals and 29 of 34 with them. The misses are
declarations the AST reader could not attribute (macro-declared locals).

## Consequence

When the instruction stream matches retail except for which `[ebp-N]` each
named local occupies, the remaining wall is the local spellings. Renaming a
local to a name in the required bucket reproduces the retail frame; any name in
the same bucket gives the identical layout. The VC4 hash previously used by
`homm1.core.od_slots` mispredicted these frames and is replaced there.

# VC4 /O2 register ties follow the C2 range id

HoMM1 VC4 (`C2.EXE` of the pinned `vc40` toolchain). This entry sharpens the tie key of
[/O2 register allocation](vc4-global-register-allocation-is-chaitin-briggs.md). Every step
below was read from C2.EXE and confirmed by tracing it.

Signature: equal-degree live ranges (typically callee-saved candidates across calls) swap
`esi`/`edi`/`ebx`/`ebp` between our build and retail. The interference graph is otherwise
identical: same ranges, degrees, weights and edges.

## Mechanism

1. **Range ids** (`0x4113de`). C2 walks the 32-bucket symbol table at `0x48a540`
   from bucket 0 to 31. Within a bucket it follows the chain, and within each symbol it
   follows its live ranges in order. Ranges are numbered from `0x4020`; the counter is at
   `0x481f40`. The bucket is the symbol's **C1 handle & 31**.
   - Inside one bucket the newer symbol comes first. For example, a local of the
     function precedes a header global in the same bucket.
   - **Symbols C2 creates itself have fixed buckets that no TU edit moves.** In
     `SetMusicQuality` the reversed-loop counter (`mov ebx, 10 / dec ebx`) sits in
     bucket `0xc`. The SEARCH temporaries sit in buckets `0, 4, 8, …`.
   - Ids numbered elsewhere (spill and temporary user ids at `0x4719a7`/`0x473586`)
     keep their ids.
2. **Nodes** hash by **id & 31** into `0x48b4e0` (user ids) and `0x48b560` (ids
   `0x8000+`). Within a bucket the newest node comes first (`0x4074bd`).
3. **Simplify** (`0x40ccd7`) builds its list by prepending bucket by bucket.
   - The list therefore runs: temporaries, then user ids from `id & 31 = 31` down to 0.
     With 32 or fewer ranges that is descending id.
   - It removes the node with the highest degree below K (6, or 7 when `ebp` is free).
     On a tie it takes the first node in the list, i.e. the **higher id**.
   - If no node is below K, it removes the lowest spill metric (`m3 = weight*100/degree`
     or `m4 = weight*100`). The driver tries both.
4. **Select** colours nodes in reverse removal order. Each node takes the first of
   `eax ecx edx esi edi ebx ebp` that no coloured or physical (id < 8) neighbour holds.

**Key:** among equal-degree ranges, the one with the **lower range id is coloured
first** and takes the earlier register (`esi` before `edi` before `ebx` before
`ebp`). The range id orders ranges by (C1 handle & 31, newer first, range order), with
C2-created symbols pinned to fixed buckets. TU state reaches a function only through
the `& 31` buckets of its own symbols and of the globals it caches, measured against
those pinned buckets.

## Measurements

Measured with an in-process C2 tracer that logged range-id assignment (id,
bucket), the simplify list, removals, edges and colours; the tracer and its
replay/solver scripts were research tooling and are not kept in the tree.

- **Replay.** Replaying every allocator pass from the traced list, degrees and
  edges was exact on every traced pass (soundmgr 50/50 and 49/49, WINMGR 27/27,
  INPUTMGR 10/10, SEARCH 21/21).
- **`soundManager::SetMusicQuality`.** Exact and non-exact graphs were
  isomorphic; only the ids differed. With `this` in bucket 0, `track` in 2, the
  AIL_serve import pointer in `0x19` and the counter in `0xc`, ties went
  `this`→esi, `track`→ebp, counter→edi, pointer→ebx. Retail needs
  `bucket(this) > bucket(track)`; because `track = this + 2`, that means
  `this & 31 ∈ {0x1e, 0x1f}`, and AIL_serve's bucket below the counter's fixed
  `0xc`. No single region offset satisfies both; over two region offsets (the
  mss.h declarations and the .cpp) there are four colouring classes, and 12 of
  12 traced compiles matched the predicted class.
- **Prediction against traced colourings** at random region offsets:
  SetMusicQuality 12/12, `heroWindowManager::AddWindow` 8/8,
  `searchArray::FindNearestObject` 8/8, `inputManager::Open` 8/8.
- **Spill rounds.** `searchArray::SeedPosition` re-runs the allocator after
  inserting spill code: 7 passes in 3 rounds (each round tries both spill
  metrics, then re-runs the one with the smaller spilled weight). Under TU
  offsets its graph itself changes (15 of 16 sampled offsets): its statics and
  parameters feed the operand sort, so the IL reaching the allocator differs and
  no allocator-only model predicts it. After-include offsets 0..600 never reach
  0 diff lines; single moves of its 24 static declarations (`.bss` order is
  name-hashed, so the move is data-neutral) do not either.

## Using it

1. Identify the tied ranges and their buckets (C1 handle & 31) in our build and
   the buckets retail requires; region handle offsets (top of TU, end of the
   includes, before the function, or header-level points such as
   `before:#include <mss.h>` when a cached global decides the tie) select the
   colouring class.
2. Realise a required offset authentically:
   - include order per the donor;
   - a value-preserving spelling that changes the handle count (a dropped cast,
     or `if (a && b)` against nested ifs:
     [control flow consumes handles](vc4-control-flow-consumes-c1-handles.md));
   - a retail-evidenced declaration.

   Then check the whole unit. `SetMusicQuality` is exact this way: MSS precedes
   `windows.h` as in Buka's include list, and `CDPlay` stores
   `m_currentTrack = track` without a cast.
3. If no class reaches retail, the residue is not the tie order. Look at the
   graph instead: temporaries, live ranges, references (see the Chaitin-Briggs
   entry). `AddWindow`, `FindNearestObject` and `inputManager::Open` are such
   cases.

## Not established

- How C2 picks the fixed buckets of the symbols it creates (`0xc` for the reversed-loop
  counter, multiples of 4 in SEARCH).
- Predicting allocator input when the operand sort changes the IL (operand-sort
  and allocator replay combined). SeedPosition needs it.
- `/Od` units have no register allocation. `advManager::DrawCell`'s `|=` residue (ADVMGR
  is `/Od`) belongs to the sortnode replay, not here.

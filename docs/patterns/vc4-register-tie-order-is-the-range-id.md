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

- **Tracer** `scripts/homm1/research/vc4trace/ra-graph.spec` (README there) logs:
  - the range-id assignment (id, bucket);
  - the simplify list, removals and edges;
  - the colours.
- **Replay.** `rasim check` replays every allocator pass from the traced list,
  degrees and edges, and was exact on every traced pass:
  - soundmgr at HEAD: 50/50;
  - soundmgr at d950846, base and with tu_state_noise trial 14: 49/49 each;
  - WINMGR: 27/27;
  - INPUTMGR: 10/10;
  - SEARCH: 21/21.
- **`soundManager::SetMusicQuality` (d950846 exact against HEAD 96.14).** The two
  graphs are isomorphic; only the ids differ.
  - At HEAD: `this` is in bucket 0, `track` in 2, the AIL_serve import pointer in
    `0x19`, and the counter in `0xc`. Ties go `this`→esi, `track`→ebp, counter→edi,
    pointer→ebx.
  - Retail needs `bucket(this) > bucket(track)`. Because `track = this + 2`, that means
    `this & 31 ∈ {0x1e, 0x1f}`.
  - Retail also needs AIL_serve's bucket below the counter's fixed `0xc`.
  - No single region offset satisfies both conditions. The c07cb76 header enums moved
    AIL_serve from bucket 0 to `0x19`.
  - `rasolve` predicts four colouring classes over two region offsets (the mss.h
    declarations and the .cpp). Each class has a distinct measured distance (23, 19,
    4, 0). 12 out of 12 traced compiles matched the predicted colouring.
- **Prediction against traced colourings** at random region offsets: SetMusicQuality
  12/12, `heroWindowManager::AddWindow` 8/8, `searchArray::FindNearestObject` 8/8,
  `inputManager::Open` 8/8.
- **Where prediction fails.** `searchArray::SeedPosition` matched 1/8. It re-runs the
  allocator after inserting spill code (7 passes), and the offsets change those spills,
  so the last pass's graph is not invariant. The replay needs a spill-code model there.

## Using it

1. `python3 -m homm1.research.vc4trace.rasolve UNIT FUNC [--point before:TEXT|after:TEXT|fn:DEC]... --validate 8`
   lists the colouring classes reachable by region handle offsets. It labels each class
   with a measured distance and reports how many traced compiles the prediction matched.
   The default points are the top of the TU, the end of the includes and the function.
   Add header-level points (`before:#include <mss.h>`) when a cached global decides the
   tie.
2. If a class reaches distance 0, realise its offsets authentically:
   - include order per the donor;
   - a value-preserving spelling that changes the handle count (a dropped cast, or
     `if (a && b)` against nested ifs: [control flow consumes handles](vc4-control-flow-consumes-c1-handles.md));
   - a retail-evidenced declaration.

   Then check the whole unit.

   `SetMusicQuality` became exact this way. MSS now precedes `windows.h` as in Buka's
   include list, and `CDPlay` stores `m_currentTrack = track` without the
   reconstruction's `static_cast<char>`. soundmgr went from 11 to 14 of 23 exact
   (`SetMusicQuality`, `StopSample`, `Open`), and no function scored lower.
3. If no class reaches 0, the residue is not the tie order. Look at the graph instead:
   temporaries, live ranges, references (see the Chaitin-Briggs entry).
   - `AddWindow`: two classes (44 and 36 lines), neither exact.
   - `FindNearestObject`: three classes; the current one (28 lines) is best.
   - `inputManager::Open`: one class.

## Not established

- How C2 picks the fixed buckets of the symbols it creates (`0xc` for the reversed-loop
  counter, multiples of 4 in SEARCH).
- Multi-pass spill replay. That is the next step for SeedPosition.
- `/Od` units have no register allocation. `advManager::DrawCell`'s `|=` residue (ADVMGR
  is `/Od`) belongs to the sortnode replay, not here.

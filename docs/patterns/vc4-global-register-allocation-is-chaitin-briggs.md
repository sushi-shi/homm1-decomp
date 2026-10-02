# VC4 /O2 register allocation is Chaitin-Briggs colouring with declaration-order ties

HoMM1 VC4 (`C2.EXE` of the pinned `vc40` toolchain). The algorithm below was read from
the disassembly. A patched copy of C2 dumped the interference graph, and a
re-simulation reproduced every dumped colouring order and colour that was checked.
Signature: call set, CFG and operand order match, but retail keeps a different
value in a register, spills a different variable, or colours the same variables
with a different permutation of `esi/edi/ebx/ebp`. The frame size can differ by one
spill slot.

## Mechanism

The source file names in C2 are `nglobreg.c`, `color.c` and `range.c`. The addresses
below are for the pinned `C2.EXE`.

1. **Candidates.** C2 renumbers each function's symbols densely from `0x4020`
   (user symbols, table `0x48b4e0`). Compiler temporaries start at `0x8000`
   (table `0x48b560`). The physical registers are nodes `0..7`. Nodes hash into 32
   buckets by `id & 31` (`0x4074bd`). The C2 ids follow C1's symbol-hash walk,
   ordered by **C1 handle & 31** from bucket 0. Function locals therefore get ids in
   declaration order. That order rotates when the TU prefix pushes the C1 handles
   across a multiple of 32.
2. **Weight** (`0x407037`): each reference adds a benefit times `5^loop depth`, read
   from table `0x481d28` = 1, 5, 25, 125. The total is capped at 30000.
   Conditional nesting has no effect. The benefit depends on the kind of
   reference: a pushed argument adds 4, and a definition or store adds about 5.
   Unallocatable symbols (for example, address-taken ones) carry 30000.
3. **Interference** edges link ranges that are live together. Ranges live across a
   call get edges to the physical nodes `eax/ecx/edx`. Copy-related nodes may be
   coalesced first (`0x40cad5`).
4. **Spill metric** (`0x40cb43`): `m3 = weight*100/degree` and `m4 = weight*100`.
   The value is 3000000 for weight 30000 or degree 0.
5. **Simplify** (`0x40ccd7`). The worklist is built by prepending every node:
   table `0x4000` buckets 0..31 first, then table `0x8000`. The list therefore
   runs from the highest temporary bucket down to user bucket 0. C2 then repeatedly
   removes one node:
   - among nodes with `degree < K`, the node with the **highest** degree;
   - if there is none, the node with the lowest metric.

   Ties keep the node that comes first in the list. Each removed node is pushed
   onto a stack, and its neighbours' degrees are decremented. `K` = 6, plus 1 when
   `ebp` is free (`0x4151f5`).
6. **Select** (`0x40cbc3`): pop the stack (last removed first). Each node takes the
   first register in the table at `0x47f390`, **`eax, ecx, edx, esi, edi, ebx,
   ebp`**, that no coloured or physical neighbour uses. A node with no free register
   is spilled. The driver tries both metrics, keeps the one with less spilled
   weight, inserts spill code and repeats.

Consequences:

- Live ranges that cross calls take `esi, edi, ebx, ebp` in **colouring order**.
  Colouring order is the reverse of simplify order.
- At equal degree, the earlier-declared local (lower C1 handle & 31) is
  coloured first and takes the earlier register.
- At equal metric, the later-declared local is pushed first, coloured last and
  spilled.
- Weight affects only spill choice, not which register a colourable node gets.
  Register choice follows degree and the tie order. A one-reference difference
  can flip a spill decision: in `icon::DrawToBuffer`, dropping one `frame`
  argument moved the register to `x`.

## Measurements

- Patched C2 (scratch only): jumps at `0x40cbc3` (select entry), `0x40cbd2`
  (select exit) and `0x4151f5` (per-function allocator) call a printf hook in the
  `.text` slack at `0x47c220`. The hook prints each node's id, weight, edge list,
  metrics and colour. A Python re-simulation of simplify and select reproduced
  the dumped colouring order and colours in every checked case:
  WINMGR (16 checked function graphs, up to 49 nodes with spills) and the micro
  probes.
- Micro probes, four `int` locals live across calls:
  - registers follow declaration order: `esi`, `edi`, `ebx`, `ebp`;
  - use counts 1..4 do not change the assignment;
  - inserting 14 to 16 typedefs before the function rotates the assignment
    exactly where a local's C1 handle reaches `0xc0`, and 17 typedefs restore it.
- Spill ties: equal-weight candidates competing for one register are spilled in
  reverse `handle & 31` order. The wrap points are at C1 handles `0xc0`, `0xe0`
  and `0x100`. One in-loop reference weighs the same as five straight-line
  references; a doubly nested reference weighs the same as 25.

## Using it

1. Classify the residue first: is it a different spill, a permutation of the
   callee-saved registers, or both?
2. **Permutation or spill between ranges with equal cost and degree:** reorder
   the local declarations. Declaring a local earlier colours it earlier, giving
   `esi` before `edi`, and makes it the survivor of a spill tie. If the
   initializers must keep their statement order, put a plain declaration block
   first and the assignments after it. An initialized declaration also fixes the
   statement order. Example: `heroWindowManager::UpdateScreenRegion` went from
   81 to 100 by declaring `top, left, bottom, right` before assigning
   `left, top, right, bottom`.
3. **A different spill or register winner by a margin:** count references times
   `5^depth` for the competing ranges. Retail usually had a different number of
   references, so look for duplicated statements that VC4 tail-merges. Examples:
   - `icon::DrawToBuffer` and `DimToBuffer` set `m_drawTop` and `m_drawBottom`
     in each orientation arm. Retail's tail merge carries the frame-entry address
     across the join, so `frame` no longer outweighs `x`.
   - `ClippedMonoIconToBitmap` increments `source` in each arm. Retail emits one
     `inc`, but `source` gains loop-weighted references and takes `ebp` from
     `rowOffset`.
4. A permutation that tracks degree rather than declaration order needs a
   different graph: different temporaries, live ranges or parameter reuse.
   Declaration order alone cannot fix it. `heroWindowManager::AddWindow` still has
   this residue. Moving its handles with the TU prefix cannot reproduce retail
   either (simulated over every permutation of handles).
5. Results depend on the TU prefix only through handle & 31. A header edit can
   therefore rotate the tie order of every later function.

## Not established

- The full benefit table of `0x40b5f7`; the reference kinds were only partly measured.
- Live-range splitting (`range.c`, `-norange`) and coalescing heuristics.
- How a node's position inside one bucket's chain is ordered: the simulation
  assumes the newest first. Every checked graph agreed, but no graph tested
  this directly.

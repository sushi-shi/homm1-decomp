# Hand-written assembly candidates in HoMM1

Surveyed 2026-09-19 against the February 1996 Windows `HEROES.EXE` pinned in
`config/retail/targets.json`:

```
size:   713216
sha256: 0d707d3456aacd470f4da601ac388a8b0be8976414b4ef689c8b849b9b2a1ce8
```

There are **17 strong candidates, covering 3,428 bytes of function bodies**.
Fourteen now have exact source matches: the three bit helpers, three bitmap
primitives, resource-name hash, tile renderer, and six small icon renderers.
The six icon identities and donor-style module names come from the Buka 2.1
sources, with PoL 2.0 as secondary correspondence evidence.

This inventory records the retail evidence behind the assembly claims. Entries
described as exact have source and match claims; the remaining candidates stay
inspection findings until reconstructed.

## Inventory

All addresses below are **RVAs**; add `0x00400000` for VAs. Body size ends at
the last reachable instruction. Partition size extends to the next start in
`config/retail/functions.tsv` and can include trailing `NOP`/`INT3` padding.
The distinction matters especially for `BitClear` and `TileToBitmap`.

| RVA | Body | Partition | Identity / observed behavior | HIGHLOW fixups |
| --- | ---: | ---: | --- | ---: |
| `0x79280` | `0xd5` | `0xd6` | Forward icon RLE blit | 6 |
| `0x79356` | `0xcb` | `0xcc` | Horizontally reversed icon RLE blit | 6 |
| `0x79422` | `0xd5` | `0xd6` | Forward monochrome icon fill | 6 |
| `0x794f8` | `0xd3` | `0xd4` | Horizontally reversed monochrome icon fill | 6 |
| `0x795cc` | `0xd9` | `0xda` | Forward icon-shaped dimming | 7 |
| `0x796a6` | `0xd6` | `0xda` | Horizontally reversed icon-shaped dimming | 7 |
| `0x7bac8` | `0x2e` | `0x2e` | `BitTest` — donor identity verified | 0 |
| `0x7baf6` | `0x20` | `0x20` | `BitSet` — donor identity verified | 0 |
| `0x7bb16` | `0x22` | `0x2a` | `BitClear` — donor identity verified | 0 |
| `0x7c82c` | `0x74` | `0x74` | Bitmap-to-bitmap rectangle copy; `BlitBitmap` correspondence | 4 |
| `0x7c8a0` | `0xe6` | `0xe6` | Rectangle move within one bitmap, with forward/backward paths | 3 |
| `0x7c986` | `0x4b` | `0x4c` | In-place rectangle dimming through a palette table | 3 |
| `0x7c9d2` | `0x43` | `0x46` | Solid-color rectangle fill | 2 |
| `0x7ca18` | `0x2a` | `0x2c` | Resource-name hash helper, called by `resourceManager::MakeId` | 0 |
| `0x7ca44` | `0x25f` | `0x260` | Clipped forward icon RLE blit | 41 |
| `0x7cca4` | `0x258` | `0x25c` | Clipped horizontally reversed icon RLE blit | 41 |
| `0x7d110` | `0x134` | `0x140` | `TileToBitmap` — strong donor/body correspondence | 8 |

All seventeen are call-free. A recursive walk from each entry, following both
conditional edges and `LOOP`, found no branch outside its partition. Bytes
after the last reachable instruction are only the padding described above.
This avoids classifying switch tables as instructions, which produced false
`STD`, `PUSHAD` and `XLAT` hits in an initial linear scan.

## Why these look like original assembly

All seventeen begin `push ebp; mov ebp, esp; push esi`; fourteen also save
`edi`. The fourteen non-BITS routines write `ebx` without saving it. That
register convention is a particularly strong distinction from ordinary
compiler-generated C/C++ functions. There is no compiler local stack frame.

The graphics routines add independent evidence: explicit `CLD`/`STD`,
`LODSB`/`STOSB`, `XLAT`, `LOOP`, mixed 16/32-bit counters, and shared absolute
scratch storage. The six small icon routines use the same 12-byte frame
descriptor setup. These observations support manual assembly; they do not
establish whether each original unit used a separate `.asm` file or inline
assembly.

The HoMM2 Buka reconstruction keeps the corresponding icon modules in `.cpp`
files. Those substantially larger HoMM2 bodies establish names and module
boundaries, but they do not prove the source language of these different
HoMM1 implementations. MASM is the current exact reconstruction form, not a
claim that the original HoMM1 filename extension is known.

### First priority: the bit helpers

The existing HoMM2 Buka reconstruction has a real
`src/BASE/BITS.asm` with all three bodies. Direct comparison of the entire
HoMM1 bodies, without masking or normalization, gives:

| Function | HoMM1 RVA | Buka 2.1 RVA | PoL 2.0 RVA | Identical bytes |
| --- | --- | --- | --- | ---: |
| `BitTest` | `0x7bac8` | `0xc2ed4` | `0xd1594` | 46 |
| `BitSet` | `0x7baf6` | `0xc2f02` | `0xd15c2` | 32 |
| `BitClear` | `0x7bb16` | `0xc2f22` | `0xd15e2` | 34 |

The three bodies contain **no HIGHLOW relocations or external calls**.
`BitClear` ends at RVA `0x7bb38`, followed by eight `INT3` bytes before the
next function. It was absent from the source claims at the start of this
survey. No direct calls to it were found; the matching donor sequence still
establishes its identity. `BitTest` and `BitSet` have callers in puzzle and
town/building code, corroborating game ownership.

Preserve the unusual memory width when reconstructing: the routines advance
by `index >> 3`, but test/update a **DWORD** at that byte address. A byte-only
rewrite would not reproduce the original access width.

The donor files used for byte verification were:

- `homm2/investigation/extracted/buka-rip/HMM2PL_portable.exe`, SHA-256
  `1614e941f380a51c3183816997db722bda075a478c23c7539103a9be05dc5008`.
- `homm2/investigation/extracted/img-pol/HEROES2W.EXE`, SHA-256
  `bc8f362dd49216c9fbcee1eb2e0429b467082a93330dd84ff95757c0182fc8a3`.

Paths are relative to `/home/sheep/Projects`. Buka supplies the preferred
assembly source correspondence; PoL independently confirms the bytes.

### Other small targets

The resource-name hash at `0x7ca18` is only 42 bytes and has no relocations.
The call at `0x76600` inside `resourceManager::MakeId` establishes its role.
It processes a NUL-terminated string, masks characters with `0x7f`, subtracts
`0x20` for values at least `0x60`, and updates a 16-bit accumulator with
`xchg ah, al; rol ax, 1; sub ax, bx`. This is not HoMM2's later 32-bit
`MAKEFILEID` algorithm; use the HoMM1 bytes rather than copying that C++ body.

The 67-byte rectangle fill and 75-byte rectangle dimmer are also compact.
The fill uses `rep stosb`; the dimmer uses `lodsb; xlat; stosb; loop`.
Both reference stride scratch at RVA `0xa1ba4`; the dimmer also references
the palette table at `0xa1aa0`. These are code-required identities only, not
initializer or data-coverage claims.

### Tile and icon families

`TileToBitmap` at `0x7d110` is called from `advManager::DrawCell`, including
sites `0x2aa39`, `0x2ab13` and `0x2ad98`. It uses the same `0x0fff` tile index
mask, `0x8000`/`0x4000` flip flags, four directional paths, and eight-pixel
unrolled reverse-copy loops as Buka's `src/BASE/TILE.asm` at RVA `0xc2554`.

It is **not a drop-in copy** of HoMM2: HoMM1 reads bitmap width/pixels at
`+0x10`/`+0x14`, checks the tile count at tileset `+0x0e`, has a one-row
forward loop instead of HoMM2's sixteen-row unrolling, and uses a WORD
scratch counter at `0xa1bcc`. Tile flag scratch is at `0xa1bc8`. Its final
instruction is a jump back to the common epilogue, ending at `0x7d244`;
stopping at the earlier `ret` would truncate two flip paths.

The six small icon bodies are strongly corroborated by calls from cursor
and map rendering and the icon wrappers. They share scratch identities
`0xa1808` through `0xa1818`. The dimming pair reads destination pixels
through the palette table and advances rows by a literal `0x280` (640).
The reconstructed `Icon2b`, `Iconf2b`, `Iconm2b`, `Iconmf2b`, `Icond2b`, and
`Icondf2b` modules match all six retail bodies and their relocations exactly.
Their claims end at the final `ret`; the following `NOP`/`INT3` bytes remain
linker partition padding outside the functions.
The clipped pair is a later priority: about 600 bytes each and 41 HIGHLOW
sites per body, with additional shared clipping state.

## False leads and limits

`OLDASM.CPP` is not enough to establish assembly. The embedded filename is
referenced at `0x73834` by an assert in the `Random` correspondence at
`0x73820`. That routine calls the assert helper and random generator and has
an optimized C/C++ shape. The nearby icon decoder at `0x73ad0` also looks
compiler-generated; multiple `rep movsd` sequences alone are insufficient.

Some automatic donor suggestions assign the six small icon bodies to
`hero::GiveSS`/`TakeSS` or assign the dimmer to CRT `__store_number`.
Their inspected bodies contradict those suggestions. Do not use those
candidate labels as ownership evidence.

The seventeen entries are a strong starting set, not a proof that no other
manual assembly exists. Integration and match scoring remain separate work.
The current AGENTS.md restriction on manufacturing matches with naked
assembly remains applicable; this survey does not change the build contract.

## Reproduction

Run inside the pinned environment:

```sh
nix develop .#build
sha256sum build/orig/HEROES.EXE
homm1 sema disasm 0x7bac8 --size 0x78
homm1 sema disasm 0x79280 --size 0x500
homm1 sema disasm 0x7c82c --size 0x6d4
homm1 sema disasm 0x7d110 --size 0x140
homm1 sema disasm 0x765f0 --size 0x20
homm1 sema disasm 0x73820 --size 0x40
```

For the direct donor comparison, still inside that shell:

```sh
PYTHONPATH=scripts python - <<'PY'
from pathlib import Path
from homm1.core.pe import Pe

h1 = Pe('build/orig/HEROES.EXE')
root = Path('/home/sheep/Projects/homm2/investigation/extracted')
donors = [
    (Pe(root / 'buka-rip/HMM2PL_portable.exe'), 0xc2ed4),
    (Pe(root / 'img-pol/HEROES2W.EXE'), 0xd1594),
]
for name, offset, size in [
    ('BitTest', 0, 0x2e), ('BitSet', 0x2e, 0x20),
    ('BitClear', 0x4e, 0x22),
]:
    for donor, start in donors:
        assert h1.read(0x7bac8 + offset, size) == donor.read(start + offset, size)
    print(name, size, 'bytes identical in both donors')
PY
```

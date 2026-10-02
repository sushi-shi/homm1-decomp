# Retail .bss order constrains global names

VC4 orders a TU's uninitialized globals by `ident_hash(name) % 1024`, ascending,
with equal keys putting the later definition first
([docs/patterns/vc4-data-emission-order.md](../docs/patterns/vc4-data-emission-order.md)).
LINK keeps each object's `.bss` run contiguous and in order. So inside one retail
object, a higher `.bss` address must have an equal or higher key. That gives a
window for each original name:

- an object whose current name breaks the order is misnamed (or belongs to a
  different TU);
- its original name's key lies between the keys of its in-order neighbours;
- a donor name for the same global that lands in that window is naming
  evidence. A window is typically 10..60 keys wide out of 1024.

## Method

For each unit, collect the `DATA(va)` definitions in `.bss` (0x004a4680 to
0x004d5f40), sort them by address and compute each name's key. Take the longest
non-decreasing subsequence of keys as the names that are in order. Everything
else is a misfit, and its window runs from the previous in-order key to the next
one (ties allowed). Gaps between claimed objects do not matter, because order
does not depend on size or alignment.

```python
def ident_hash(name):
    v = 0
    for c in name:
        v = ((v >> 7) + v * 4 + ord(c)) & 0xFFFFFFFF
    return v

def key(name):
    return ident_hash(name) % 1024
```

For random names, the expected longest in-order run of n keys is about 2*sqrt(n).
Long consistent runs therefore show the rule held in retail, and that most
names in those units are right.

## Per-unit consistency

Measured after the KB donor renames (commit "KB globals: HoMM2 donor names
proven by VC4's .bss name-hash order").

| Unit | `.bss` objects | in key order | Notes |
|---|---:|---:|---|
| SOURCE/KB | 104 | 91 | 84 before the seven donor renames; random expectation ~20 |
| SOURCE/PHILAI | 60 | 35 | ValueOfEventAtPosition's module globals have invented names |
| SOURCE/REMOTE | 27 | 26 | `giNumNetGuests` needs a key in [517,592] |
| SOURCE/GAME | 17 | 7 | band mixes names from several authors; `s_adjacentMonster*` are statics |
| SOURCE/ADVMGR | 16 | 11 | DrawCell's `s_draw*` state has invented names |
| BASE/LZHUF | 16 | 13 | |
| SOURCE/netwin | 14 | 5 | Netbios state names are invented |
| SOURCE/FINDPATH | 13 | 7 | search statics are partly invented |
| SOURCE/kbwin | 10 | 10 | consistent |
| SOURCE/COMMAND | 9 | 9 | consistent |
| BASE/MOUSEMGR | 8 | 8 | consistent |
| SOURCE/wingraph | 7 | 4 | |
| BASE/soundmgr | 6 | 4 | |
| SOURCE/CURSOR | 5 | 5 | consistent |
| BASE/WINMGR | 2 | 1 | |
| SOURCE/SMACKMGR | 2 | 2 | consistent |
| SOURCE/SPELLAI | 2 | 1 | |

## Names adopted on this evidence

Each HoMM1-invented KB name fell outside its window. The Buka 2.1 name of the
same global, with matching semantics, falls inside it:

| HoMM1 name (key) | Window | Donor name (key) |
|---|---|---|
| `gbKingOfTheHill` (701) | [783,793] | `gbIAmGreatest` (785) |
| `gbStandardHighScore` (604) | [736,746] | `giHighScoreType` (743) |
| `gbUseClippedIconRenderer` (164) | [687,708] | `gbIconClipOn` (693) |
| `gcRegCDDrive` (343) | [629,646] | `gcRegCDRomPath` (644) |
| `giWeekSpecial` (471) | [153,176] | `giWeekTypeExtra` (169) |
| `giMonthSpecial` (505) | [364,400] | `giMonthTypeExtra` (379) |
| `gcCongratsText` (489) | [794,858] | `gcWinText` (831) |

## Unclaimed objects placed by the rule

These ADVMGR `.bss` gaps are not referenced from retail code. Their names must
hash into these windows:

| Address | Size | Window |
|---|---|---|
| 0x004c4f30-0x004c4f47 | 24 | [203,414] |
| 0x004c4f64 | 8 | [502,526] |
| 0x004c50b0-0x004c50bf | 16 | [596,957] |

For the 8-byte object at 0x004c4f24, the window depends on which object it
belongs to:
- as ADVMGR's first object, its key is <= 203 (`cPanel`);
- in FINDPATH, its key is >= 501 (`gSearchNextY`), a weak bound because
  FINDPATH's statics are partly invented.

## Limits

A key that fits proves nothing alone: each window admits about 1-6% of
arbitrary names. A name is adopted only when a donor names the same global
with the same role. Equal-key ties also depend on first-declaration order.
That order can come from a header extern rather than from the definition.

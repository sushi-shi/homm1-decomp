# Hero deallocation: retail layout constraints

Pinned retail `hero::Deallocate` is RVA `0x6cee2`, length `0x452`.
The no-argument `ret` and Dismiss caller establish the HoMM1 signature;
Buka's conditional `updateMap` argument is a later revision. HoMM1 always
mobilizes the current hero and hides the route. Buka's network map-change,
weekly-visit replacement guard, event-flag clearing and campaign reward
logic are absent. This evidence is layout recovery, not a completed body
or an initializer claim.

| Entity | Retail offsets and widths |
| --- | --- |
| hero | signed byte id `0`, owner `1`, x `0x1e`, y `0x1f`, destinations `0x20/0x21`; unsigned location byte `0x23`; town id byte `0x24`; army `0x57`; embarked test `0xae & 0x80` |
| player | stride `0x105`; hero count byte `0x13`, current hero `0x14`, locator `0x15`, eight hero IDs at `0x16`, two available IDs at `0x1e` |
| game | players `0x20c`; town records `0x121a1` with stride55; available heroes `0x1431d`; boats stride8 with hero ID at `0x1448c`; boat slots `0x14586` |
| town | occupying hero byte `0x15` |
| adventure manager | cursor-active byte `0x183`; context lock dword `0x198` |
| world cells | address `game + 0x626 + x*720 + y*10` reaches flags byte; clears bit `0x40` |

Globals used by this body: game pointer VA `0x4c6d48`, adventure manager
pointer `0x4c7e18`, current-player signed byte `0x4af748`, surrender signed
byte `0x4c6720`, retreat signed byte `0x4c6d4c`. No storage claims are made
until the owner layouts are complete.

The local census is seven four-byte stack homes: this `-0x1c`, available
slot int `-0x18`, town pointer `-0x14`, loop index short `-0x10`, old owner
int `-0xc`, hero-list index signed byte `-8`, player pointer `-4`.
The constructor at `0x6ba90` independently confirms army `+0x57` and the
byte-width identity, owner and coordinate fields; CalcMobility `0x6bb6d`
independently tests the embarked flag and army array. Player Read/Write at
`0x38df8/0x38c00` provide the next structural controls. Their skipped save
bytes do not by themselves prove the runtime-only gaps in playerData.

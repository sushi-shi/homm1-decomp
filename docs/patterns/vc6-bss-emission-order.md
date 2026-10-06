# VC6 `.bss` emission order: name key, then zero initializers

Measured with the pinned HoMM1 Buka VC6 SP5 compiler under
`/Od /Ob1 /GX /MT /G6` on minimal TUs (40 random identifiers of mixed
`int`, `char`, `short`, pointer, `double` and array types, two seeds).

## Observations

- **Uninitialized definitions** are emitted in ascending
  `key16(name) & 0x3ff`, with the same `key16` as the `/Od` local-slot
  buckets:

  ```python
  def key16(name):
      h = 0
      for c in name:
          h = ((h >> 4) + h * 4 + ord(c)) & 0xFFFFFFFF
      return (h ^ (h >> 16)) & 0xFFFF
  ```

  `name` is the identifier for a file-scope definition (`static` or
  external) and the decorated name without the leading underscore for a
  function-local static (`?lzz@?1??f1@@YAHXZ@4HA`). Type, size and definition
  position do not matter.
- **Equal keys** emit the later definition first (`vaca`, `vaai`, `vabe` with
  one key, defined in that order, come out `vabe`, `vaai`, `vaca`). What
  counts is the symbol's first declaration in the TU, so an `extern` in a
  header decides: swapping `gMapName` and `gbBlackoutPlayer` (both key 623)
  in `KB.h` swaps them in `SOURCE/KB`, while swapping their definitions
  does not.
- **Zero-initialized definitions** (`char zz0 = 0;`) also go to `.bss`, after
  every uninitialized one, in definition order. A zero-initialized function
  local static takes its function's position in that order, and empty-string
  literals placed in `.bss` follow all of them.
- The same rule holds for the `/O2` units (`SOURCE/FINDPATH`, `SOURCE/SEARCH`).
- A **static data member** definition is keyed by its full decorated name
  (`?member@Class@@2HA`), in the same sort as the file-scope identifiers
  (15 random members of one class and 15 globals: no inversion).
- Compiler static-destructor guards (`_$S<n>`) take part under their `$S<n>`
  name. A destroy-once guard byte is emitted for a run of class-type static
  member definitions; the run's first member gets it, and a definition after
  intervening functions starts a new guard. `<n>` comes from a TU-wide counter
  shared with the `$E` initializer/destructor helpers and the `.CRT$XCU`
  entries. Every static data member declaration advances it by one (an `int`
  member as well), as does each name the `<string>` prelude creates; function
  bodies and ordinary declarations do not.
- In `BASE/Audio` the music members' guard and the device member's guard
  are 11 counter values apart. With three member declarations before the
  music definitions they are `$S19` (key 962) and `$S30` (key 961), so the
  device guard sorts first; retail has the music guard first (0x004cdf70)
  and the device guard after it (0x004cdf71). Any count of declarations
  from four up orders them as retail (four gives `$S20`/`$S31`).

- **Alignment.** A record-typed (struct, class or union) global of 8 bytes
  or more starts 8-byte aligned, whatever `#pragma pack` or `/Zp1`-`/Zp4`
  gives its members, and so does an array of such records; a character or
  `short` array of any size, and a record under 8 bytes, start 4-byte aligned.
  A `.data` or `.bss` section is 4-byte aligned, or 8 when it holds an
  8-aligned object.

## Use

Within one retail object, ascending `.bss` address among uninitialized
definitions is ascending key. A recovered name whose key falls outside the
window between its retail neighbours is not the retail spelling, or the
definition is a local static, or it was zero-initialized. A global placed
after the hash-sorted run (for example `EveryOther` after `S1cursorTurning`
in `SOURCE/CURSOR`) points to a zero initializer or a local static.

## Applying it

Per object, sort the claimed `.bss` symbols by retail address and keep the
longest key-ascending run; the rest need a name in the key window left
between their kept neighbours, a zero initializer, or (for ties) another
declaration order. Sibling spellings are the strongest evidence a window
offers: `iMainWinScreenHeight` (key 410) and `iTempX`/`iTempY` (496/497)
next to `gMainWinScreenWidth` and `iTempY`, `gBigFont`/`gSmallFont` (79/389)
in `SOURCE/KB`, and `cColorBits` beside `cAndBits` in `BASE/MOUSEMGR` all
fit their windows without search.

## Alias defines

The source spells the readable name; where that name's key falls outside its
retail window, the owning header maps it to a spelling whose key fits,
directly above the `extern` (file statics above their definition, function
statics above their declaration):

```cpp
#define gGame gpGame // spelling fixes .bss order
extern class game* gGame;
```

The compiler hashes the storage spelling, and function-local statics take it
into their decorated name (`?s_direction_4@?1??SeedPosition@...`): a define
of the identifier works for them too, measured on `SOURCE/SEARCH`, whose
`SeedPosition` statics keep their retail order behind readable names. Tie
order still follows the first declaration. A define is visible wherever its
header is, so the readable name must not also spell a local, member or
parameter there; a file or function static's define stays in its `.cpp`.
The generated trees (`homm1 clean`) drop these defines.

## Does not establish

The key orders definitions; it does not supply names. A name invented to fit
a window is not evidence.

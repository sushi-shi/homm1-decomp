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
- Compiler static-destructor guards (`_$S<n>`) take part under their `$S<n>`
  name. In `BASE/Audio`, `$S30` (key 961, device) sorts before `$S19` (key
  962, music); retail has the music guard first (0x004cdf70) and the device
  guard after it (0x004cdf71), so retail's guard numbers differ.
  The `$E` initializer/terminator functions and the `$S` guards and
  `.CRT$XCU` entries share one counter: each `RefPtr` static takes four
  `$E` numbers and one `.CRT$XCU` `$S`, and the first static needing a
  guard also takes the guard's number, so the device guard is always the
  music guard plus 11. Neither include order nor the order of
  the three definitions moves the music guard off 19 (the first 16 numbers
  are taken before the first definition), and retail's initializer order in
  `.text` matches the current one; the swapped pair stays open.

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
next to `iMainWinScreenWidth` and `iTempY`, `gBigFont`/`gSmallFont` (79/389)
in `SOURCE/KB`, and `cColorBits` beside `cAndBits` in `BASE/MOUSEMGR` all
fit their windows without search.

## Does not establish

The key orders definitions; it does not supply names. A name invented to fit
a window is not evidence.

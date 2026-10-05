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
  one key, defined in that order, come out `vabe`, `vaai`, `vaca`).
- **Zero-initialized definitions** (`char zz0 = 0;`) also go to `.bss`, after
  every uninitialized one, in definition order.
- Compiler static-destructor guards (`_$S<n>`) take part under their `$S<n>`
  name. In `BASE/Audio`, `$S30` (key 961, device) sorts before `$S19` (key
  962, music); retail has the music guard first (0x004cdf70) and the device
  guard after it (0x004cdf71), so retail's guard numbers differ.

## Use

Within one retail object, ascending `.bss` address among uninitialized
definitions is ascending key. A recovered name whose key falls outside the
window between its retail neighbours is not the retail spelling, or the
definition is a local static, or it was zero-initialized. A global placed
after the hash-sorted run (for example `EveryOther` after `S1cursorTurning`
in `SOURCE/CURSOR`) points to a zero initializer or a local static.

## Does not establish

The key orders definitions; it does not supply names. A name invented to fit
a window is not evidence.

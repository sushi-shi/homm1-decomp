# VC4 data emission order: .data by definition, .bss by name hash

HoMM1 VC4 (pinned `vc40` toolchain, `SOURCE/PHILAI` flags), measured on
minimal TUs and on `SOURCE/PHILAI`. Relevant when a TU's data layout has to
follow retail order, or when deciding whether a definition's position in the
source is constrained by layout at all.

## Observations

- **Initialized `.data`.** Every file-scope variable with an initializer is
  emitted in definition order. Variables come first, ahead of all of the TU's
  string literals, even literals used by functions defined before the variable.
  The literals follow in first-use order. A function-local `static` with an
  initializer joins the variable group at its position in parse order; it is
  not placed among its function's literals.
- **`.bss`.** Uninitialized definitions are ordered by
  `ident_hash(name) % 1024`, ascending, not by definition order:

  ```python
  def ident_hash(name):          # same hash as the /Od local-slot buckets
      v = 0
      for c in name:
          v = ((v >> 7) + v * 4 + ord(c)) & 0xFFFFFFFF
      return v
  ```

  Minimal TU defining `int zeta; int alpha;`, then `S gS;`, then `int mid;`.
  The keys are zeta 804, alpha 435, gS 495 and mid 220. The emitted `.bss` is
  `mid` +0, `alpha` +4, `gS` +8, `zeta` +0xc. All 60 `.bss` definitions of
  `SOURCE/PHILAI` come out sorted by this key. Moving a `.bss` definition
  within the TU therefore does not move it in the object; only renaming does.

  Equal keys keep the later definition first: `int vaae; int vaba; int vabb;
  int vaaf;` (keys 411, 411, 412, 412) emit `vaba`, `vaae`, `vaaf`, `vabb`.

  The rule runs backwards too. Within one retail object, ascending `.bss`
  address must be ascending key, so each object's original name hashes into
  the window between its neighbours' keys. Checking a donor or invented name
  against that window is naming evidence.
- **Dynamic initializers.** The compiler-generated initializer functions
  (`_$E<n>`) for a file-scope object with a constructor are emitted in `.text`
  at the definition's position, between the functions before and after it.
  The `.CRT$XCU` entry points at them. A definition such as `SVSearchArray`
  is therefore pinned to the gap between the retail functions that surround
  its initializer (0x41f2a9, between `RVOfPosition` and
  `StrategicValueOfPosition`).
- **Named constants.** A `static const float` that is loaded rather than
  folded is emitted as a named `.rdata` object in definition order, ahead of
  the anonymous float/double literals. Equal values are not merged. PHILAI's
  pool at 0x48c0a8 (1.5, 1.25, 1.1, 0.15, 1.12, 1.0, 1.0, then the literals)
  is reproduced byte for byte only with Buka's named block declared in that
  order.

## Open

Retail's interleaving of variables with literals is the `/Gi` per-function
section layout: see [incremental compilation](vc4-gi-incremental-compilation.md).


Retail PHILAI `.data` interleaves two variables with string literals:
`bSVSearchArrayInUse` (0x48f7b8) sits after `GetBestHero`'s literals, and
`bEvaluatingTravelGates` (0x48f824) after `BuildCreature`'s. The
vars-before-literals emission above cannot produce that from one TU in any
definition order, and a function-local static does not produce it either.
How retail got this layout (different storage, a different compile of the
object, or a mechanism not yet seen) is unresolved.

## Does not establish

The `.bss` key makes source position irrelevant to `.bss` layout. It says
nothing about where the original source defined those globals; position still
moves C1 symbol handles (see
[operand sort keys](vc4-operand-sort-key-is-the-symbol-handle.md)).

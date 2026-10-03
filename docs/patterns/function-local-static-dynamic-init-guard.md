> Imported from Giten `39384dc6726478357b5efd42c66522781e8310fe`.
> This is donor evidence, not a validated HoMM1 VC4 rule. Commands and source
> examples describe that donor. Re-prove applicable mechanisms with VC4.

# A guarded initializer may be a function-local static

A bit test and conditional branch around a bit update and one-time initializer
can be the expansion of:

```cpp
int Next() {
    static long state = InitialSeed();
    return Advance(state);
}
```

The [recorded VC5 examples](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/function-local-static-dynamic-init-guard.md)
include multiple local statics using distinct bits of a shared guard.
The `game RNG` (historical donor reference) is a concrete source example.

Follow control flow and references to the initialized object. A read/OR/store
without the conditional initialization path can simply update ordinary flags.
The guard and value need not be adjacent. Bit operations alone do not uniquely
prove a local static.

Check enclosing linkage and all emitters before deciding whether state is
shared or per-TU. Do not invent guard globals, pin volatile compiler ordinals,
or infer source duplication from multiple emitted copies.

Matching the guard does not validate the initializer: preserve side effects,
evaluation order, and the number of calls. An object's address is not its stored
pointer value; in particular, a CString object is not its character buffer.

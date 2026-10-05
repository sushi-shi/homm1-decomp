> Unmeasured hypothesis: observed with MSVC 5.0; not measured on this
> target's VC4 or VC6 compilers. Re-prove the mechanism before relying on it.

# A guarded initializer may be a function-local static

A bit test and conditional branch around a bit update and one-time initializer
can be the expansion of:

```cpp
int Next() {
    static long state = InitialSeed();
    return Advance(state);
}
```

Observed MSVC 5.0 examples include multiple local statics using distinct bits
of a shared guard.

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

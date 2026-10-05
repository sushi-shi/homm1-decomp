> Unmeasured hypothesis: observed with MSVC 5.0; not measured on this
> target's VC4 or VC6 compilers. Re-prove the mechanism before relying on it.

# Sibling scopes can change stack-slot reuse

When alternative branches use distinct locals, declaring those locals inside
their respective scopes can change the frame and stack displacements:

```cpp
switch (mode) {
case Save: {
    int value = ReadValue();
    SaveValue(value);
    break;
}
case Load: {
    Object* result = LoadObject();
    UseObject(result);
    break;
}
}
```

An MSVC 5.0 A/B reported a smaller frame, matching retail, after moving
arm-specific declarations from function scope into their arms.

This demonstrates a scope-sensitive allocation, **not** the old universal claim
that MSVC 5.0 overlays slots only for disjoint lexical scopes. A shared retail slot
does not uniquely recover source scope, and a frame-size difference can also
come from spills, temporaries, alignment, or EH.

Compare actual slot accesses and complete lifetimes, accounting for changing
push depth. Test meaningful local ownership; do not add arbitrary blocks solely
to steer allocation.

## Dead parameter homes

A scoped local can also land in the stack home of a parameter that is dead by
then. In an observed MSVC 5.0 case, one `case` arm declares two out-locals
whose addresses it passes; the compiler places them in the homes of two
already-consumed parameters and the frame shrinks from 0x10 to 8 bytes,
matching retail. Declared at function scope, the same locals get their own
frame slots. So a retail local addressed at a parameter's `[esp+n]` home is a
hint that the source scoped it narrowly, not that the source reused the
parameter.

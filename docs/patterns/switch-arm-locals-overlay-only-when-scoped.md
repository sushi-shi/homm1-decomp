> Imported from Giten `39384dc6726478357b5efd42c66522781e8310fe`.
> This is donor evidence, not a validated HoMM1 VC4 rule. Commands and source
> examples describe that donor. Re-prove applicable mechanisms with VC4.

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

The [recorded SerializeFields A/B](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/switch-arm-locals-overlay-only-when-scoped.md)
reports a smaller matching frame after moving arm-specific declarations from
function scope into their arms.

This demonstrates a scope-sensitive allocation, **not** the old universal claim
that VC5 overlays slots only for disjoint lexical scopes. A shared retail slot
does not uniquely recover source scope, and a frame-size difference can also
come from spills, temporaries, alignment, or EH.

Compare actual slot accesses and complete lifetimes, accounting for changing
push depth. Test meaningful local ownership; do not add arbitrary blocks solely
to steer allocation.

## Dead parameter homes

A scoped local can also land in the stack home of a parameter that is dead by
then. In OpJumpUnlessStatContest (0x4348b0) one `case` declares two
out-locals whose addresses it passes; cl 5.0 places them in the homes of the
already-consumed `level` and `swap` parameters and the frame shrinks from 0x10
to 8 bytes, matching retail. Declared at function scope, the same locals get
their own frame slots. So a retail local addressed at a parameter's `[esp+n]`
home is a hint that the source scoped it narrowly, not that the source reused
the parameter. Lane 2's field-object residues (a local "stored in a dead
parameter slot") may be the same mechanism where the scoping was not tried.

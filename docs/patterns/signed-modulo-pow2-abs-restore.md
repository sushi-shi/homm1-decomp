> Imported from Giten `39384dc6726478357b5efd42c66522781e8310fe`.
> This is donor evidence, not a validated HoMM1 VC4 rule. Commands and source
> examples describe that donor. Re-prove applicable mechanisms with VC4.

# Signed remainder can include sign correction around a mask

A signed remainder by a positive power of two cannot generally be replaced by
an unsigned mask: negative dividends require a negative or zero remainder.

A recorded VC5 sequence computes an unsigned magnitude, masks it, and restores
the original sign:

```asm
cdq
xor eax, edx
sub eax, edx
and eax, 7fh
xor eax, edx
sub eax, edx
```

With EDX retaining the original sign mask, this implements signed remainder
modulo 128. The [historical example](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/signed-modulo-pow2-abs-restore.md)
reports this sequence even after a mask that made the input nonnegative.

Recognize the data flow before introducing handwritten absolute-value logic.
This does not prove a unique C++ expression or that VC5 always uses this
lowering. Check operand width, signedness, preceding range constraints, and
whether the sign mask is actually preserved through the sequence.

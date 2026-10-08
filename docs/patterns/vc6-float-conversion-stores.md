# VC6 explicit float-conversion stores

Measured in HoMM1 Buka's complete `SOURCE/PHILAI` translation unit with the
pinned VC6 SP5 unoptimized profile.

`GetGameAttentionValue` converts each random quotient to `float` before adding
the double base weight:

```cpp
attention->gameBuildingAttention = static_cast<float>(Random(0, 100) / 500.0) + 0.23;
```

Without the explicit conversion, VC6 emits `FILD; FDIV; FADD`. With it, VC6
emits retail's `FILD; FDIV; FST dword ptr [temporary]; FADD`. Two occurrences
account for the additional eight bytes of compiler temporary storage. The
stores do not pop the x87 value, so their presence alone does not establish
that subsequent arithmetic reloads a rounded value.

Restoring the conversions gives a complete byte/reference match without
adding source locals.

# VC6 local-static symbol names

Measured with the pinned HoMM1 Buka VC6 SP5 compiler, under `/Od`, `/Od /Z7`
and `/O2`, with `/GX /MT /G5` held constant:

```cpp
static int fileValue;
int externalValue;
int* Pick(int n) {
    static int localValue;
    return n ? &fileValue : &localValue;
}
```

| Declaration | VC6 COFF spelling |
| --- | --- |
| External | `?externalValue@@3HA` |
| File static | `_fileValue` |
| Function static | `_?localValue@?1??Pick@@YAPAHH@Z@4HA` |

VC6 does not append VC4's `$S<n>` file-static ordinal, and it adds an
underscore to the function-static mangling. Debug information does not
change these outcomes. Scope ordinals remain volatile and use the existing
canonicalization. `core/msvc_names.py:data` selects the ABI from the compiler
contract; VC4 rules are retained for the maintained NWC branches.

The control explains FINDPATH/SEARCH claim-to-object joins. It does not
justify merging same-named file statics across translation units: their
unit ownership and distinct retail addresses remain necessary.

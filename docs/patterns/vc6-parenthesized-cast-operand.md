# VC6 parenthesized cast operands keep a separate `fild`

Measured with the pinned HoMM1 Buka VC6 SP5 compiler under
`/Od /Ob1 /GX /MT /G5`. When the right operand of a floating multiply or
divide is a cast of an integer lvalue, VC6 normally folds the conversion into
the arithmetic instruction. Parenthesizing the whole cast expression stops the
fold; the operand is loaded with its own `fild` and combined with the popping
form:

```cpp
int a[4], b[4];
float f;
f = static_cast<float>(a[i]) / static_cast<float>(b[i]);   // fild a; fidiv b
f = static_cast<float>(a[i]) / (static_cast<float>(b[i])); // fild a; fild b; fdivp
f = a[i] / (float)(b[i]);                                  // fild a; fidiv b
f = a[i] * (static_cast<float>(b[i]));                     // fild a; fild b; fmulp
```

Parentheses inside the cast do not change the result; only the parenthesized
cast expression does. Changing the cast to `double` does not produce the
separate load here: the division remains `fidiv`, and a following `3.0f` literal
becomes a double operand.

`philAI::GetBestBHC` (`SOURCE/PHILAI`, RVA `0x497c8`) is the retail control.
Its creature-balance term is

```cpp
static_cast<float>(ideal[townNo]) / (static_cast<float>(strengths[townNo])) / 3.0f + 0.66
```

which emits retail's `FILD; FILD; FDIVP; FDIV dword; FADD qword`. HoMM2 Buka's
`GetBestBHC` spells the same parenthesized cast.

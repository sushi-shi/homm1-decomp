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

## Second control: a call result times a parenthesized cast

`ScaleSampleVolume` (`BASE/Audio`, RVA `0x69d6e`, `/Od /Ob1 /GX /MT /G6`) is the
second retail control. With an unparenthesized operand, VC6 emits
`call; fimul [ebp+8]`:

```cpp
return GetEffectsVolume() * (static_cast<float>(volume)) / 127.0f;
// call GetEffectsVolume; fild dword [ebp+8]; fmulp st(1),st; fdiv dword [127.0f]
```

A generated corpus of 6,000 one-line functions compiled with the Audio flags
produced 85 exact `call; fild [ebp+8]; fmulp st(1),st; fdiv dword` bodies. They
crossed callee kind, return and parameter types, cast spelling, grouping, divisor
spelling, statement shape and return type. Every exact body had a parenthesized
cast operand, whether `((float)v)`, `(static_cast<float>(v))` or a cast inside
`((C) * (...))`. A separate sweep of 5,129 command-line combinations compiled
eight unparenthesized spellings. It covered `/Od` and `/O1`, `/O2`, `/Ox`,
`/Og`, `/Ot`, `/Os` and `/Oy`. It also covered `/Ob0` through `/Ob2`, `/Op`,
`/Za`, `/G5` and `/G6`, calling conventions, `/GX`, `/Gy`, `/Gf` and `/GF`,
`/QIfdiv`, `/Z7`, `/J`, `/vd0`, `/GZ`, `/Ge` and `/Gs`. None emitted `fmulp`.

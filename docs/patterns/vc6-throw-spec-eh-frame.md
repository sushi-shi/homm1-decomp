# VC6 `throw()` functions keep new-expression temporaries without an EH frame

Measured on the pinned VC6 SP5 `/Od /Ob1 /GX /MT /G6` compiler (HoMM1 Buka
`BASE/miscwin` `FadeIn` and `FadeOut`).

Under `/GX`, `T* p = new T;` with an out-of-line constructor allocates three
compiler temporaries: the `operator new` result, the conditional constructor
result and a third copy made when the unwind state returns to -1. The cleanup
state also forces the `fs:[0]` registration record. Retail Buka shows the
three-temporary sequence with **no** registration record:

```asm
call   ??2@YAPAXI@Z          ; operator new
mov    [ebp-0x1c], eax       ; $T1
...                          ; ctor or 0 -> [ebp-0x28] $T2
mov    eax, [ebp-0x28]
mov    [ebp-0x18], eax       ; $T3 (the /GX state copy)
mov    ecx, [ebp-0x18]
mov    [ebp-0x14], ecx       ; pal
```

Scratch controls of `void f() { pal* p = new pal; use(p); }`:

| Source / flags | EH frame | new temporaries |
| --- | --- | --- |
| `/GX-` | no | 2 |
| `/GX` | yes | 3 |
| `/GX`, constructor `__declspec(nothrow)` | no | 2 |
| `/GX`, function `throw()` or `__declspec(nothrow)` | no | 3 |

Only a function-level empty exception specification reproduces retail. MSVC
does not mangle exception specifications, so `?FadeIn@@YAXH@Z` is unchanged.
Declaring both fades `throw()` under the ordinary `/GX` unit profile closed
both functions exactly and left every other `miscwin` body unchanged, which
retired the unit's former `/GX-` profile. A missing third temporary with no EH
frame is the signature to look for elsewhere.

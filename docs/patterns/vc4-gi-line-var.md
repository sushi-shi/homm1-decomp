# Retail assertions use VC4 /Gi line statics

Several HoMM1 units pass assertion lines as a per-function `short` in `.data`
plus a constant, never as an immediate:

```
movsx eax, word ptr [0x4a1694]   ; 605, StartSample's line static
add   eax, 0x25                  ; +37 -> line 642
push  eax
push  0x4a1698                   ; "D:\Heroes\Base\Soundmgr.cpp"
push  ecx
call  ProcessAssert
```

VC4 emits exactly this for `__LINE__` under incremental compilation (`/Gi`, which
requires `/Zi`). A probe compiled with the pinned toolchain:

```cpp
#line 605 "D:\\Heroes\\Base\\Soundmgr.cpp"
void f(int a) {
    g = 1;
#line 642
    ProcessAssert(a, __FILE__, __LINE__);
}
```

gives `?__LINE__Var@?1??f@@YAXH@Z@4FA` (a function-local static short, value 605)
and `movsx eax, [__LINE__Var]; add eax, 0x25`. Without `/Gi` the line is the
immediate `push 0x282`. The static belongs to the function that contains the
`__LINE__` token: soundmgr's line 52 static is read by both CDStop and CDPlay,
because the ordinary (non-`inline`) member `ValidatePreviousPosition` (at
original line 52) is expanded into both by `/Ob2`; an `inline` definition instead folds the line to a
constant.

Retail values (`.data`, Soundmgr.cpp): 52 (ValidatePreviousPosition), 605
(StartSample), 740 (StopAllSamples), 808 (ModifySample), 900
(AdjustMusicVolumes), 1008 (PlayAmbientMusic), 1118 (PollSound, five assertions).
They are the original source lines of those functions, in source order.

Other units with the same pattern (currently modelled as explicit `...AssertLine`
/ `...LineBase` globals): RESMGR, INPUTMGR, MOUSEMGR, WINMGR, miscwin, EVENTS,
NOOPT, PATH, TOWNMGR, wingraph, netwin (`netlo.cpp`). Each is a `/Gi` candidate;
the explicit globals and their header externs are not retail declarations.

Converting BASE/soundmgr to this form (flags `cpp_o2_inline_gi`, `#line`
directives, non-inline helpers, header externs removed) also changes code in
units that include `soundmgr.h` (CMBTMGR, REQUEST): removing its 12 line-global
externs shifts their C1 symbol handles.

# /Gi data placement: local statics vs `__LINE__Var` (HoMM1 VC4, measured)

Measured with the pinned VC4.0 `C1XX`/`C2`. This continues
[the listing note](vc4-gi-listing-packs-functions.md) on retail's
parse-order `.data`.

## How C2 orders data

C1XX writes initialized definitions to the IL `init` stream (`in`). It writes
functions and their string literals to the `exp` stream (`ex`). The C2 main
routine (0x0043a96f onward) does three things in a fixed order:

1. It opens `in` and `gl`.
2. It consumes the whole `in` stream (0x0043a0dd).
3. Only then does it run the function loop over `sy`/`ex` (0x0043a232).

No branch in this sequence depends on a flag. So anything C1 writes to `in`
lands ahead of every literal of the object. Plain `/Gi`, `/Gi` with listings,
`/Gm`, `/FR`/`/Fr`, `/YX`, `/Zd`, `/Gi` without `/Zi`, a missing `/Fd`, the
VC4 debug default switches, and incremental recompiles (functions edited or
added, new variables and `__LINE__` uses added against an existing `.idb`) all
leave file-scope variables first.

## Function-local statics follow their function under /Gi

Without `/Gi`, a function-local `static` with an initializer joins the
variable group. With `/Gi`, C1 writes it into `ex` with its function, so it
is emitted at the head of that function's data, ahead of the function's
literals, wherever it is declared in the body:

    void f3(int x) { puts("LIT_THREE"); if (x) { static short late = 0x4444; ... } }
    /Gi:  ... | LIT_TWO | DD.. LIT_THREE | ...      (DD = late, at f3's head)

This holds with and without a listing switch. Retail's parse-order variables
fit this. PHILAI's `bSVSearchArrayInUse` (0x0048f7b8) comes at the start of
`StrategicValueOfPosition`'s data and `bEvaluatingTravelGates` (0x0048f824) at
the start of `ValueOfEventAtPosition`'s. Each is referenced only by that
function (0x0041f2c3 and 0x0042278b respectively). Buka models both at file
scope, but the order evidence supports function-local statics under `/Gi`.

## `__LINE__Var` always goes to `init`

`__LINE__` under `/Gi` is expanded in C1XX at 0x00499090: it emits the text
"`__LINE__Var` + (line − function start)". The per-function symbol is
created at 0x00499140 and written to `in`, so all `__LINE__Var` words sit
at the top of the object's data (in the order f4, f3, f2 in a three-function
test). This was tested for free functions, member functions, `/GX` frames,
nested blocks, and functions that also declare local statics.

Retail places each line word at the head of its function's data, where a
`/Gi` local static would go:

- TOWNMGR's word 0x0048ed8c (1483) precedes `BuyBuild`'s four `__FILE__`
  literals and its other literals, after the previous functions' literals.
- RESMGR's four words each precede their function's `__FILE__` literal.

With the pinned toolchain, retail's code and retail's data placement for
these words can't both come from `__LINE__`:

- `__LINE__` (with `#line` for the value) gives retail's code
  (`movsx eax, word [word]` / `add eax, N`), but puts the word at the top.
- A function-local `static short` holding the line base, used as
  `base + N`, gives the same code and retail's placement. But it is not the
  `__LINE__Var` symbol, and no source or donor shows such a spelling.

Which one to use is a reconstruction-policy decision. The retail evidence
alone doesn't decide whether the original C1XX build emitted `__LINE__Var`
like other local statics.

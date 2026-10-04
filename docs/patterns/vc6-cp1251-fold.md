# VC6 inline CP1251 case folding

Measured in HoMM1 Buka's SOURCE/ARMY with `/Od /Ob1 /GX /MT /G5`.
The early-return `CyrillicToUpper` and `CyrillicToLower` helpers in
`include/SOURCE/KB.h` expand into range checks, arithmetic and a shared byte
return slot. The argument's global byte is reread at each expression;
no independent argument-copy local is emitted. Preserve unsigned-byte casts:
signed `char` comparisons would reject the Russian range.

The helper accepts ASCII and the contiguous Russian alphabet, plus the
separate Yo pair. Other CP1251 characters pass unchanged; this is not a full
Unicode or locale-aware case converter. Ordinary C++ helpers recover the
shared operation without open-coded blocks or artificial frame padding.

HoMM2's corresponding helpers at revision
`e0689d3f71b2942b544fd677cb54085a13503d7b` supplied corroboration. HoMM1's
retail assembly remains the authority: all five emitted folds were executed
for every input byte and checked against both the candidate and an independent
ASCII/Russian Unicode oracle. The complete hydra routine also has the same
instruction sequence and CFG after separately verified stack placement and
references. Ranged and melee whole-function matching remains open.

See [combat evidence](../../config/retail/buka-combat-messages.json) for
addresses, sizes, case-table hashes and source/object fingerprints. The
per-callsite byte stack slot is a compiler-generated return temporary, not
an invitation to add a source local to fit a frame.

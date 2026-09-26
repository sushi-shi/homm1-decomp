# VC4 inline exception registrations

Retail HISCORE Open RVA 0x10bf has a 0x169-byte owner. Its registration stub is inside that owner, followed by the real FS-chain restoration and return. Truncating the owner at the first unwind action omitted its epilog. All 58 parsed registration groups in the current retail census have certified inline continuation; their 116 FuncInfo/map identities remain named. Packed out-of-line controls retain the existing separate code records.

The fix preserves every owner byte on both sides. Registration and unwind pointers normalize to the same owner plus the exact original offset; FuncInfo/map and ___CxxFrameHandler identities stay strict during code-first comparison. Invalid FuncInfo or an unsupported inline continuation fails closed. The CxxFrameHandler RVA 0x803b0 is independently identified by evidence/homm1-dna-bands.tsv as pinned VC4 libc.lib member 1877.

Seven InlineEHControls cover complete inline owner and epilog, protected identities, malformed FuncInfo, malformed return, register-paired FS restoration, inline versus packed census ownership, and separate packed registration naming. They run in the normal `homm1 test` suite; all 175 integrated tests pass, along with the build and normal verification gates. The pinned Giten whole-tree and Gruntz audits were repeated.

In the final integrated SOURCE/HISCORE TU under the evidenced /GX profile,
constructor, Open and Close match exactly. Main is a complete ordinary
0x269-byte body at 99.93; Update remains untouched. The private constructor
control had a small compiler-state difference that disappeared in the combined
TU. Raw object/report/source snapshots are retained in the ignored
`build/sol-integration/winddown/` checkpoint.

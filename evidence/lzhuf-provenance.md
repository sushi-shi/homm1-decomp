# LZHUF codec-object provenance

The pinned February 1996 Windows executable contains two code-generation
families in its LZHUF codec. `DecodeData`, `EncodeData`, and the four retained
encoder tree functions are optimized Microsoft C/C++. The five decoder
functions at RVAs `0x7fca5..0x80187` use the register convention and code
shapes found in the Watcom-built DOS family. They communicate with the VC4
wrapper through globals rather than through the Microsoft calling convention.

This establishes that a prebuilt or inherited codec object was linked into the
Windows executable. It does **not** establish that the Windows build invoked
Watcom or that the game team maintained two active compiler toolchains. The
algorithm is classic Okumura LZHUF, so a vendor-supplied or older project object
is plausible; the exact object provenance and ownership remain unproven. TAPR's
preserved JNOS 1.11f source contains a decoder adaptation explicitly dated
1994-12-19. Its non-Borland `GetBit` and `GetByte` branch uses register locals
for the bit buffer and length which correspond closely to the retail register
allocation. This narrows the source family, but it does not prove that the game
used JNOS or establish the compiler version.

The distinction is established from the retail bytes rather than source-name
similarity. Against `HEROES_dos_demo12_1995-11-28.exe`, `GetBit`,
`DecodePosition`, `UpdateDecoderTree`, and `ReconstructDecoderTree` have the
same normalized instruction streams; only relocated global addresses and call
targets differ. The private memmove body at Windows RVA `0x7fc58` is byte for
byte identical to the 77-byte DOS body at raw offset `0xf3fa0`. The decoder
core has the same loop after the Windows adaptation loads its output pointer
and length from globals. The other three surveyed DOS releases contain the
same helper signatures, and the August 1997 Windows release retains the same
decoder bodies.

No tested compiler, source form, and flag combination reproduced the retail
decoder objects. This includes Watcom 10.0, 10.0a, 10.0b, and 10.5. The six
decoder members under `vendor/lzhuf/decoder` are manual MASM reconstructions
from the pinned retail bytes, not recovered original vendor assembly source.
Keeping the helper and five routines separate preserves the external call
relocations found in retail; combining the same exact bodies into one module
resolves those calls and scores 99.93%. Each separate member matches its retail
function at 100%, including relocation symbol and addend.
`vendor/lzhuf/reference/decoder_correspondence.c` remains the ordinary C
correspondence source used for type and compiler experiments. The encoder is
reconstructed in `vendor/lzhuf/encoder.cpp`; it remains ordinary C++ and is
compiled with the optimized VC4 profile proven by its instruction selection.
Compiler family does not imply that this imported algorithm belongs to the
game's authored source tree.

The decoder run has stronger evidence for an assembly-bearing source than for
five independently compiled C functions. The Windows sequence is completely
unpadded: the 77-byte helper ends at odd-addressed `GetBit` RVA `0x7fca5`, and
the following extents end exactly at `0x7fd2a`, `0x7fdf3`, `0x7ff0c`, and
`0x8005f`. Every isolated Watcom 10 C build tested aligns subsequent functions.
Retail `DecodePosition` also contains the inlined `GetByte` body with its own
nested `push ebx`/`pop ebx`; normal C auto-inlining, including forced
`-oe=1000`, merges register allocation and removes that nested save. The retail
`Decode` body explicitly preserves EAX as well as the usual nonvolatile
registers. A preserve-all `#pragma aux` probe does not reproduce that shape and
instead makes Watcom save GS. Together with the identical DOS run, these facts
point to an assembler module, a Watcom `#pragma aux` instruction body, or a
hand-adjusted vendor object which follows the Watcom register ABI. They do not
identify the original vendor or source archive.

Period archives establish an assembly lineage for this exact algorithm, but
do not yet supply the 32-bit retail module. DiscMaster preserves `LZ_C.ARJ`
from the x2ftp mirror (archive timestamp 1994-06-18, SHA-256
`9a0b91e50dab0ae31ce45b365b3004fb79e5d8a61514bfa32e9f390a71c04018`).
Its `LZHUF.C` is the 1989 Yoshizaki/Okumura source and contains the familiar
`GetBit`, `DecodePosition`, `reconst`, and `update` C functions; despite the
old x2ftp catalogue suffix “(asm)”, that archive contains six text files and
no assembler or object module. Separately, the preserved LHarc 1.13 source
contains a hand-written `HUF.ASM`, identified in its header as the adaptive
Huffman module dated 1989-05-04. It exports `_reconst`, `_update`, and
`_DecodePosition`, places `_TEXT` in a byte-aligned code segment, and spells
out the bit-buffer and tree operations in assembly. This is direct historical
precedent for an assembly implementation of the same function family and is
consistent with retail's unpadded run, though the preserved file is 16-bit and
is not the HoMM1 object. A DiscMaster filename/format search found four
`LZHUF.OBJ` records representing three unique files; all three identify as
8086 Microsoft OMF. No indexed 32-bit object named `LZHUF.OBJ` was found.
The downloaded archives and objects remain ignored research artifacts under
`build/research`.

A systematic compiler experiment scored 27 Watcom 10.0a optimizer profiles,
then crossed the leading profiles with 386, 486, and Pentium register and
stack conventions. With the current ordinary-C reconstruction, the inherited
`-3r -otexan` profile scores 43.20% across the five decoder functions. Removing
alias relaxation (`-3r -otex`) raises that to 45.85%; selecting Pentium register
code generation (`-5r -otex`) raises it to 48.46%. The corresponding Watcom
10.5 matrix peaks at 43.41% (`-3r -otex`). These measurements select the current
10.0a `-5r -otex` build contract, but they do not establish exactness: the
different per-function winners show that source form matters. Current
ordinary-C measurements are 22.80% for `GetBit`, 53.74% for `DecodePosition`,
98.12% for `UpdateDecoderTree`, 29.42% for `ReconstructDecoderTree`, and 55.03%
for `Decode`. The `UpdateDecoderTree` body is instruction and operand exact;
only the retail save order and unpadded extent remain different. This makes an
exact Watcom compiler revision useful for reproducibility, but insufficient by
itself to recreate the retail decoder module from ordinary C.

The adjacent 10.0 compiler revisions were tested from a preserved Watcom
collection rather than inferred from their version labels. The original 10.0
GA `WCC386.EXE` has SHA-256
`a4f4da1b8b2cd4e38b2b041975fc6b1d131255ae5eb575828b02f927fbf9bf65`.
Applying Watcom's original `PTCH23.A` produces SHA-256
`c3666de94f6fa6800f452dae8acf45505ecdb62f0ade2cc27cc86c2d9e8e2b6b`,
byte for byte the compiler extracted from the pinned 10.0a CD. Applying
`PTCH23.B` to that result produces SHA-256
`2b6672b7cb109452a31cb7fd6bf6776f518f43466f66dc3efd8365708fd243c6`.
With the same source and `-5r -otex` contract, 10.0a and 10.0b emit identical
native OMF objects. The GA compiler falls to 35.86%, 51.17%, 88.41%, 17.82%,
and 45.67% on the five functions respectively. Thus the relevant code
generator change is patch A; patch B does not affect this integer codec, and
10.0a remains the minimal period compiler justified by the retail comparison.

The wrapper at `0x19b4a` accepts a string and two 32-bit values, formats them
as `%s : % 8d  % 8d`, and writes the result to `KB.LOG`; `LogStr(char *, long,
long)` is the code-required identity used by `DecodeData`. The CRT function at
`0x81b00` is `memmove`, identified both by its body and by the two encoder tree
reconstruction calls.

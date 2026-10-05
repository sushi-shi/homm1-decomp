# LZHUF vendor codec

The linked implementation and source-correspondence material have distinct
locations:

| Path | Build role |
| --- | --- |
| `encoder.cpp` | Compiled as the single `BASE/LZHUF` object with the pinned Buka VC6 `/Od /G5` profile. It contains the game-facing wrappers, encoder, shared globals, and initialized Huffman tables. |
| `decoder/Decoder.asm` | Assembled as the single `BASE/LZHUFDEC` object: one private memory-move helper and five decoder routines, in retail order. |
| `reference/decoder_correspondence.c` | Readable ordinary-C reconstruction used for type and compiler experiments. It is not a build input. |
| `reference/paul-edwards-1990-lzhuf.c` | Unmodified historical source snapshot used only as provenance evidence. It is not a build input. |
| `reference/jnos-1.11f-lzhuf.c` | Unmodified historical source snapshot used only as provenance evidence. It is not a build input. |

A safe Rust port of the complete codec, byte-exact with these objects, is in
[`tools/homm1-lzhuf`](../../tools/README.md). `homm1 verify lzhuf-oracle`
compares it with the retail `EncodeData`/`DecodeData` executed under Wine.

The `initialSon`, `initialFrequency`, and `initialParent` arrays are defined
directly in `encoder.cpp`. Both `DecodeData` and `EncodeData` copy these
templates into the shared working trees. There is no separate table translation
unit.

We did not find a compiler, source form, and flag combination that reproduces
the retail decoder objects. This includes the tested Watcom 10.0, 10.0a, 10.0b,
and 10.5 variants. The files in `decoder/` are therefore manual MASM
reconstructions from the pinned retail bytes; they are not recovered original
vendor assembly source.

The six reconstructed procedures form one module. In retail they are
byte-contiguous with odd starts and no fill, while LINK separates objects with
`CCh` fill. LINK also pulls the group starting with `LzhufMemmove`, although
LZHUF references only `Decode`. The assembler resolves calls between the
procedures without relocations. The comparison gives each such call the REL32
relocation that the delinked target carries
(`homm1.compare.canonicalize.relocate_in_object_calls`). Their register
convention and instruction bodies match the Watcom-built DOS family. The
tested Watcom C compilers did not reproduce, from ordinary C, the byte-aligned
layout or the retained nested saves.

Buka 2003 did not recompile the decoder with its VC6 compiler. All six Buka
decoder bodies equal the NWC Windows module (February 1996, May 1996 and
August 1997 builds) after masking absolute addresses, including the
Windows-only `Decode` adaptation. They keep the Watcom register convention
(callee-saved `ECX`/`EDX`, no stack frame, a nested `push ebx` in
`DecodePosition`) and the unpadded odd starts, none of which VC6 `/Od` or `/O2`
C produces. The single difference is in `DecodePosition`: Buka encodes
`cmp dx,0FFFFh` with a word immediate (`66 81 FA FF FF`), as the German DOS
build of 1995-10-09 also does, while every other NWC build uses the
sign-extended byte immediate (`66 83 FA FF`). The masked `DecodePosition` and
`ReconstructDecoderTree` bodies match only the German DOS and Buka images. Buka
therefore linked a decoder object from the same pre-built family, not one
rebuilt from C. The HoMM2 Buka 2.1 donor uses bzip2 rather than LZHUF and has no
corresponding decoder. In the reconstruction the full-width immediate is written
`cmp dx,WORD PTR 65535`: ML 6 keeps a word immediate only when it is explicitly
typed. A forward-referenced constant, `TEXTEQU`, `OPTION M510` and
`OPTION NOSIGNEXTEND` all still select the byte form.

`reference/decoder_correspondence.c` establishes types, algorithm
correspondence, and compiler provenance. Watcom C/386 10.0a reproduces the
complete `UpdateDecoderTree` instruction body, but emits a different save order
and function padding. It is evidence rather than the linked implementation.

`reference/paul-edwards-1990-lzhuf.c` is the Paul Edwards 1990 revision. Its
explicitly 16-bit `GetBit` and `GetByte` forms agree with important details in
the retail decoder. It is an unmodified snapshot of commit
`cca43442b667aef1a139119c72095f71a86ad42a` from
<https://github.com/vonj/snippets.org/blob/cca43442b667aef1a139119c72095f71a86ad42a/lzhuf.c>.
Its SHA-256 is
`ab580116f1b7d5510f9f5ca65e7cdc4ee93f6ed156c31365166bb67225f74379`.

`reference/jnos-1.11f-lzhuf.c` is an unmodified file from TAPR's
preserved JNOS 1.11f source archive. Its header dates Jack Snodgrass's adaptation
to JNOS 1.10h to 1994-12-19. Its alternative non-Borland `GetBit` and `GetByte`
implementations use the register locals seen in the retail code generation.
The archive is
<ftp://ftp.tapr.org/software_lib/tcpip/jnos/jnos111f/jnos111f.zip>, with
SHA-256 `d5f52dcf12d29788dc0e1feb5cf49d7e8a86b0a18e1040d4d14a4ed36e5fcb47`;
the extracted file's SHA-256 is
`239cb266cb5c1387f70c79e9c4806fd7edd780a96c15e34b17a2cd9d63d7abc2`.
This is source-family evidence rather than proof that New World Computing used
JNOS itself.

## Runtime behaviour

Executing the retail functions (`homm1 verify lzhuf-oracle`) establishes
these properties of the linked codec; the reconstruction keeps all of them:

- `text_buf` is shared by both directions and never cleared. `EncodeData`
  fills `text_buf[0..4036)` with spaces, but its search trees also compare
  the stale look-ahead and mirror bytes, so short encodings depend on earlier
  calls in the same process.
- `DecodeData` fills nothing. A stream that copies from the encoder's space
  prefill decodes those bytes from whatever the receiver's window holds; in a
  fresh process they become zero bytes. The shipped `REMOTE.GAM` is affected:
  decoded in a fresh process, two spaces at offsets 221 and 222 become zeros.
- For empty input the `unsigned short` look-ahead count wraps, and
  `EncodeData` codes 65 536 window positions after a zero length prefix.
- `TransmitSaveGame` (`SOURCE/GAME.cpp`) sends the value `EncodeData`
  returns as the transfer size. That value excludes the four-byte length
  prefix, so the last four bytes of the stream are never sent, and the
  receiver decodes them from the uninitialised end of its `malloc` buffer.
  For the shipped `REMOTE.GAM`, filling the missing bytes with `00h` or
  `CDh` changes the last 26 or 29 decoded bytes. This is retail behaviour
  and is reconstructed as such.

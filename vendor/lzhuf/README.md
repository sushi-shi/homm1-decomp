# LZHUF vendor codec

The linked implementation and source-correspondence material have distinct
locations:

| Path | Build role |
| --- | --- |
| `encoder.cpp` | Compiled as the single `BASE/LZHUF` object with the optimized VC4 codec profile. It contains the game-facing wrappers, encoder, shared globals, and initialized Huffman tables. |
| `decoder/*.asm` | Assembled as six separate decoder objects: one private memory-move helper and five decoder routines. |
| `reference/decoder_correspondence.c` | Readable ordinary-C reconstruction used for type and compiler experiments. It is not a build input. |
| `reference/paul-edwards-1990-lzhuf.c` | Unmodified historical source snapshot used only as provenance evidence. It is not a build input. |
| `reference/jnos-1.11f-lzhuf.c` | Unmodified historical source snapshot used only as provenance evidence. It is not a build input. |

The `initialSon`, `initialFrequency`, and `initialParent` arrays are defined
directly in `encoder.cpp`. Both `DecodeData` and `EncodeData` copy these
templates into the shared working trees. There is no separate table translation
unit.

We did not find a compiler, source form, and flag combination that reproduces
the retail decoder objects. This includes the tested Watcom 10.0, 10.0a, 10.0b,
and 10.5 variants. The files in `decoder/` are therefore manual MASM
reconstructions from the pinned retail bytes; they are not recovered original
vendor assembly source.

The six reconstructed members remain separate because calls between them carry
external relocations in retail. Combining the same exact instruction bodies
into one assembler module resolves those calls internally and no longer matches
the retail object topology. Their register convention and instruction bodies
match the Watcom-built DOS family, while their byte-aligned layout and retained
nested saves were not reproduced by the tested Watcom C compilers from ordinary
C.

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

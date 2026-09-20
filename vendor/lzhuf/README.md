# Legacy LZHUF decoder

`decoder/` is the six-member assembler implementation linked by the retail
game: the private memory-move helper followed by five decoder routines. Each
member is kept separate because calls between them carry external relocations
in retail. Their register convention and instruction bodies match the
Watcom-built DOS family, while their byte-aligned layout and retained nested
saves cannot be emitted by the tested Watcom C compilers from ordinary C.

`decoder.c` remains the ordinary C source reconstruction used to establish
types, algorithm correspondence, and compiler provenance. Watcom C/386 10.0a
reproduces the complete `UpdateDecoderTree` instruction body, but emits a
different save order and function padding. It is evidence rather than the
linked implementation.

`upstream/lzhuf.c` preserves the closest located historical source family. It
is the Paul Edwards 1990 revision, whose explicitly 16-bit `GetBit` and
`GetByte` forms agree with important details in the retail decoder. The file is
an unmodified snapshot of commit
`cca43442b667aef1a139119c72095f71a86ad42a` from
<https://github.com/vonj/snippets.org/blob/cca43442b667aef1a139119c72095f71a86ad42a/lzhuf.c>.
Its SHA-256 is
`ab580116f1b7d5510f9f5ca65e7cdc4ee93f6ed156c31365166bb67225f74379`.

`upstream/jnos-1.11f-lzhuf.c` is an unmodified file from TAPR's preserved
JNOS 1.11f source archive. Its header dates Jack Snodgrass's adaptation to
JNOS 1.10h to 1994-12-19. In particular, its alternative `GetBit` and
`GetByte` implementations for non-Borland compilers use the register locals
seen in the retail code generation. The archive is
<ftp://ftp.tapr.org/software_lib/tcpip/jnos/jnos111f/jnos111f.zip>, with
SHA-256 `d5f52dcf12d29788dc0e1feb5cf49d7e8a86b0a18e1040d4d14a4ed36e5fcb47`;
the extracted file's SHA-256 is
`239cb266cb5c1387f70c79e9c4806fd7edd780a96c15e34b17a2cd9d63d7abc2`.
This is source-family evidence rather than proof that New World Computing
copied JNOS itself.

`encoder.cpp` is the VC4-compiled half of the imported codec, including the
game-facing memory wrappers. `tables.inc` holds the initialized Huffman
templates shared by both implementation families.

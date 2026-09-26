# DDSetPalette (retail RVA 0x40a8)

Buka 2.1 and PoL 2.0 SOURCE/wingraph.cpp supply the busy/foreground and
DirectDraw-object guards, primary-surface palette attachment, error check,
and return values. Retail calls surface vtable slot 31 (offset 0x7c),
reads source-line base as a signed word at VA 0x48e824, adds 20, and reports
the filename `D:\Heroes\Source\wingraph.cpp`. PoL retains the named line-base
source form, with its release-specific addend. The 0xb3-byte function matches
100% in code mode.

The full original IDirectDrawSurface interface is transcribed from Microsoft
DDRAW.H in the preferred donor toolchain, lines 749–789. Its 36 real SDK
operations remain in order; no fake virtual slots or game bodies are added.
All unrecovered SDK record types are forward-declared and pointer-only.

Code-use DATA identities are primary surface pointer VA 0x48e5b4,
palette pointer VA 0x48e5c0 (four bytes each), and source-line-base word
VA 0x48e824 (two bytes). Data initializers remain unclaimed.

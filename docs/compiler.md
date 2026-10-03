# Compiler and toolchain

The working compiler is **Visual C++ 4.0, 10.00.5270**. Complete per-unit
profiles live in `config/units.toml`; `/Od`, optimized BASE profiles and inline
controls are separate measured choices. `/Z7` supplies COFF function extents.
Do not substitute Giten VC5 or HoMM3 VC6 flags.

## Setup

```sh
nix develop .#build
homm1 toolchain install
homm1 toolchain check --id vc40
homm1 tool wine --init
homm1 build
```

`config/toolchains.json` pins media, components and release hashes.
Installation explicitly downloads a SHA-256-checked VC4/MASM 6.11 release;
`init` and `build` do not fetch tools implicitly. Tools, media and the local
Wine prefix stay in ignored `build/`. Inherited CL, _CL_, INCLUDE and LIB cannot
change compilation. SDK filenames are lowercase for native Clang analysis;
Wine uses the same verified compiler inputs.

Reproduce the release with
`nix-shell scripts/toolchain/create-toolchain-release.nix`. The builder verifies
original media, reconstructs the MASM disk, expands KWAJ members, checks outputs
and normalizes archive metadata. VC4 media is the
[MSVC4x archive](https://archive.org/details/msvc4x), `MSVC40.iso`, SHA-256
`961326efbfbd299794e2cbb102e9ff3bfe78ebf91a11b89978095f59e0aea93e`;
MASM comes from PCjs's preserved diskette. `toolchain install --id ID --media PATH`
is the component-repair route; the VC4 disc does not contain MASM.

## Identification limits and callback control

AppAbout (VA `0x45c15c`, ordinal 1) is a 144-byte stdcall dialog callback.
VC2.0 (9.00), VC2.2 (9.10) and VC4.0 (10.00.5270) all emit identical `/Od`
code for the original probe; their `/O2` versions are 73 bytes and differ.
Retail's linker 3.00 agrees with VC4's linker 3.00.5270, but this callback
alone does not prove every object's compiler, packing or exception settings.

Retail ends the callback with `leave; ret 16` at VA `0x45c1e9`, before the
next prologue at `0x45c1ec`. Message `0x110` returns true; `0x111` splits command
ID, handle and notification code, including unused local stores. ID 1 calls
EndDialog; other paths call PollSound and return false. An explicit default/break
adds a five-byte `/Od` jump absent from retail. The operand at RVA `0x5c1ac`
relocates to EndDialog's IAT slot `0xd661c`; the call at `0x5c1da` targets
PollSound VA `0x44f640`. Source: SOURCE/kbwin.

Inspect with `homm1 walls semdiff 0x0005c15c` and `homm1 inspect --json`.
FPO `.debug$F` records are debugger metadata, excluded from code comparison.

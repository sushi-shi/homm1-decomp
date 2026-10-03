# Compiler and toolchain

This branch uses **Visual C++ 4.1, CL 10.10.6038 / LINK 3.10.6038**.
The [original-media and real-TU controls](patterns/vc41-win95-1997.md)
explain the selection. `config/units.toml` retains the measured `/Gi`, `/Od`,
optimized BASE and `/GX` profiles and pins the recovered source paths.

```sh
nix develop .#build
homm1 toolchain install
homm1 toolchain check --id vc41
homm1 tool wine --init
homm1 build
```

The hash-pinned `toolchain-win95-1.2-v1` bundle contains VC4.1, MASM 6.11,
WinG 1.0 and DirectX 1, including RC/RCDLL/CVTRES. Reproduce it with
`nix-shell scripts/toolchain/create-toolchain-release.nix`; see the controls
above for original-media hashes and the installed-files reproduction route.
Retail game and runtime DLLs are separate user inputs.

The following observations describe the earlier VC4.0 reconstruction and
remain historical evidence; they do not override this branch's VC4.1 pins.

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

# Tooling and provenance

The repository is independent of sibling checkouts at runtime. Its starting
points are local snapshots of:

- `homm3-decomp` at `d2bd86aa746da522f8d1af2c77fb657233fc4a2e`: verified input
  staging in `scripts/homm3/core/inputs.py`, the readable/mapped PE section
  distinction in `core/image.py`, CLI layout, and the Nix package definitions
  and lock for objdiff 3.7.3 and vostok-delinker.
- `homm2-decomp` at `299514f88900c0cf30ba03422c72830a38fc1cb7`: the COFF/RES
  reader and its portable tests (`scripts/homm2/core/{coff,test_coff}.py`).

The configuration layout follows `gruntz` at
`b1de0e555576a215898907b8ec8ed5423368883e`: structural RVA/kind base censuses,
separate provider tables, and a root per-TU manifest with `[build]`, `[flags]`
and `[[unit]]`. Only the channels with evidence are populated here.

Both HoMM projects dedicate their own tooling to CC0. The copied reader retains
its design notes; imports and module names have been adapted to HoMM1.
The PE report parser is new and handles this target's imports, exports and
base-relocation table. It does not guess function boundaries from prologues.

## Commands

Use `nix develop` for analysis, or `nix develop .#build` to also obtain Wine
with a repository-local prefix. The shell supplies `homm1`, objdiff GUI/CLI,
vostok-delinker, Ninja, LLVM and GNU binutils. Python 3.11+ is sufficient for
the standalone `./homm1` CLI; `disasm` additionally requires GNU objdump.

```sh
homm1 init --exe /path/to/HEROES.EXE --editor-exe /path/to/EDITOR.EXE
homm1 status --json
homm1 inspect
homm1 inspect --target editor --json
homm1 disasm 0x0045BB45 --size 0x80
homm1 check
homm1 test
homm1 object /path/to/compiler-output.obj
```

The game is required by `init`; the editor is optional. Explicit input paths
override `HOMM1_EXE` / `HOMM1_EDITOR_EXE`, which override existing staged copies.
Every selected input must match its size and SHA-256 pin. Bad explicit inputs
are rejected even if a valid staged copy exists. Subsequent analysis reads and
verifies the staged copies without consulting the environment. Reports contain
integer VAs/RVAs; `inspect` renders the principal addresses in hexadecimal.

`config/retail/functions.tsv` records **located** game function starts and
structural kinds. The separate `functions_exports.tsv` provider records the
export names and provenance. Neither table asserts sizes or source ownership.
`data.tsv` is an empty starting census. These are sparse censuses, not complete
partitions; gaps must not be interpreted as function/data extents.
`config/units.toml` admits the `app_about` fragment with one source-owned
`RVA(rva, size)` claim. See [configuration](../config/README.md).

## Matching loop

[Compiler setup and evidence](compiler.md) document the pinned VC4.0 compiler
and the VC2.x comparison controls. The repository commands are:

```sh
homm1 toolchain install --id vc40 --media /path/to/MSVC40.iso
homm1 toolchain check --id vc40
homm1 build
homm1 probe --ids vc20 vc22 vc40  # after installing the other candidates
```

`build` verifies retail and compiler hashes, checks source claims and their
baseline, generates `build/build.ninja`, compiles, carves an independent retail
COFF object, compares resolved bytes and the relocation stream, and runs
objdiff-cli. It writes `build/match-report.json` and an objdiff project in
`build/objdiff/`. Ninja depends on source, local headers, tooling and compiler
configuration. Compiler failure removes stale objects and retains diagnostics.
Any byte/size/relocation difference returns a failure; a visual objdiff score
cannot override that verdict. The tracked exact baseline also detects dropped
or shortened claims.

The initial carver supports **one exported four-argument stdcall dialog body
per source/object**, with one `RVA` annotation and no additional emitted code.
It binds the claim to the export name and the compiled COFF symbol. It only
accepts reviewed IAT DIR32 and direct-call/jump REL32 references. Every retail
HIGHLOW field in the body must be covered, and encoded targets/addends must
agree with the PE bytes. No relocation field is masked. The target object's
code comes from retail, with the reviewed relocations reversed into COFF
addends; it is not copied from the compiler's output.

This deliberately small carver does not use vostok-delinker's PDB path yet.
The pinned package remains available for the later multi-function/data pipeline.
The build is not a linked/runnable game, a full-TU reconstruction, or a coverage
score for the whole executable. The compiled callback still references an
unimplemented retail service. Exact historical compiler attribution and CRT/SDK
provisioning remain open.

## Next work

Recover more independent function/type evidence and compiler probes. Extend
claim extraction beyond the explicit dialog ABI, add full instruction decoding
for broader relocation recovery, and establish multi-function/TU boundaries
before admitting a larger unit. Preserve the Gruntz census/provider separation.
Add PDB-based delinking, data ownership and linker reproduction when the evidence
supports them. HoMM3's VC6 profiles, Dreamcast roster and fixed-base assumptions
must not be carried over to HoMM1.

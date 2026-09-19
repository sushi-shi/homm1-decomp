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
`config/units.toml` deliberately admits no units yet. See [configuration](../config/README.md).

## Bring up the matching compiler next

1. Identify and locally provision candidate MSVC 2.x compiler/CRT/SDK versions.
   Linker 3.00 and the observed unoptimized code narrow the search; they do not
   establish an exact compiler, runtime mode, or complete command line.
2. Compile small probes under Wine and compare their prologues, stack homes,
   member calling convention, instruction choices, and COFF records to retail.
   Pin the validated compiler artifacts by hash before adding an automatic fetch.
3. Establish function extents and relocation ownership for a small evidenced
   unit. Preserve HoMM1's HIGHLOW relocations and initialized/BSS distinction.
4. Add compiler wrapper, Ninja configuration, delinker inputs, objdiff project,
   and relocation-aware comparison together. Admit that unit and record a real
   baseline only after the end-to-end loop runs.

The available objdiff/delinker packages do not by themselves establish a
matching build. HoMM3's VC6 `/O2 /Gr /GX` profiles, Dreamcast symbol roster and
fixed-base assumptions must not be carried over. HoMM2's later code and types
are research references; they are not recovered HoMM1 source.

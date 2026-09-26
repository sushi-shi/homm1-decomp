# Compiler profiles

`config/units.toml` is authoritative for each unit's complete flag set.
The pinned compiler is VC4; do not substitute Giten's VC5 `/Ox /Zp1` profile
or HoMM3's VC6 settings. ABI and layout flags affect extraction as well as
compilation. New claims need independent retail-backed controls.

See [compiler evidence](compiler.md) for measured probes and identification
limits. `homm1 toolchain check` checks the installed pinned toolchain.

# Tooling

`homm1` dispatches to the Python package here. Use command help and the
[command map](../docs/tooling-map.md); the [build guide](../docs/build-system.md)
describes generated artifacts. All entry points use `homm1.core.usage.logged`.
Run `homm1 audit usage` and `homm1 build` after changes.

Keep reusable analysis in the existing package; a module belongs here only if
a `homm1` command or the build graph reaches it. Reports and disposable
experiments belong under ignored `build/`.

`toolchain/create-toolchain-release.*` reproduces the pinned VC4.1 compiler
archive, [toolchain-win95-1.2-v1](https://github.com/sushi-shi/homm1-decomp/releases/tag/toolchain-win95-1.2-v1);
`merge-units.sh` is the repository's manifest merge-driver wrapper.

# Tooling

`homm1` dispatches to the Python package here. Use command help and the
[command map](../docs/tooling-map.md); the [build guide](../docs/build-system.md)
describes generated artifacts. All entry points use `homm1.core.usage.logged`.
Run `homm1 audit usage` and `homm1 build` after changes and
[repeat donor review](../docs/tooling-inheritance.md).

Keep reusable analysis in the existing package. `homm1.labels.donor_align`
remains a research-only correspondence tool; it does not generate source or
admit claims. Completed bootstrap generators and their embedded source copies
are retired. Reports and disposable experiments belong under ignored `build/`.

`toolchain/create-toolchain-release.*` reproduces the pinned compiler archive;
`merge-units.sh` is the repository's manifest merge-driver wrapper.

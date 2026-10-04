# homm1-decomp-buka

C++ reconstruction of **Heroes of Might and Magic — Buka 2003**, using VC6 SP5.
Original English is preserved alongside the Russian translation catalog.

**Matching: unavailable.** Strict delinking is incomplete, so no valid Buka
comparison percentage has been generated. Sources compile and the candidate
links; the port is still in progress.

See [port changes and evidence](docs/buka-2003.md). Supply your own executable
and game assets.

## Branches

Win95 1.2 is maintained on [decomp-win95-1.2](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.2).
See the [1.1 → 1.2 changes](https://github.com/sushi-shi/homm1-decomp/blob/decomp-win95-1.2/docs/win95-1.2.md)
for behavior differences and port validation.

```text
decomp-win95-1.0 ---> decomp-win95-1.1 ---> decomp-win95-1.2 ---> decomp-buka-2003
        |
        v
source-win95-1.0
```

| Branch | Purpose |
| --- | --- |
| [decomp-win95-1.0](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.0) | Reconstruction of the February 1996 Win95 1.0 `HEROES.EXE` |
| [decomp-win95-1.1](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.1) | Reconstruction of the May 1996 Win95 1.1 `HEROES.EXE` |
| [decomp-win95-1.2](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.2) | Maintained reconstruction of the August 1997 Win95 1.2 `HEROESW.EXE`, using VC4.1 |
| [decomp-buka-2003](https://github.com/sushi-shi/homm1-decomp/tree/decomp-buka-2003) | Buka port; implementation and target migration in progress |
| [source-win95-1.0](https://github.com/sushi-shi/homm1-decomp/tree/source-win95-1.0) | Generated clean source for Win95 1.0 |

## Quickstart

With Nix flakes enabled, run from the repository root. The build currently
compiles the sources, then stops at unprovided Buka data identities during
strict delinking. Inherited source/data claims still need migration:

```sh
nix develop .#build
homm1 init --exe /path/to/HEROES.EXE
homm1 toolchain install
homm1 tool wine --init
homm1 build
homm1 match BASE/MOUSEMGR
homm1 verify status
homm1 play --data /path/to/HEROES   # optional: run the build
```

The optional editor is supplied with `init --editor-exe /path/to/EDITOR.EXE`.
Retail inputs, tools, Wine state and generated reports stay in ignored `build/`.

See [the matching workflow](docs/tooling.md), [setup and editors](docs/workflow.md),
and the [documentation index](docs/README.md).
Contributor rules and verification commands are in [AGENTS.md](AGENTS.md).

## License

Project-authored source and tooling use [CC0 1.0](LICENSE). Dependencies retain
their own terms; retail inputs, compiler binaries and game assets are excluded.

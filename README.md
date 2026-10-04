# homm1-decomp-buka

Porting **Heroes of Might and Magic** from Windows 95 1.2 to the 2003 Buka
release, using ordinary C++ and preserving the original English alongside
Russian catalogs. Work is on `decomp-buka-2003`, branched from 1.2 at
`43cf275fee2f`. See [the Buka migration](docs/buka-2003.md) for documented
behavior changes, evidence, validation and remaining work.

The active target is now the Buka `HEROES.EXE`, with VC6 SP5 and Russian
catalog output. Audiere device, music, sample playback and the Smacker loop
are implemented and compile. The candidate links without unresolved symbols,
and all seven Russian resource payloads match retail. Strict comparison is not available yet: the
inherited NWC address census and claims still need migration, and the Buka
absolute-reference manifest needs review. The old score ledger has been reset.
Supply your own game executable and assets; they are not included here.

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
compiles the sources, then stops at the unmigrated retail census:

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

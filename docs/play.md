# Playing the build

`homm1 play` builds, links the candidate with its resources, installs it beside
your game data and starts it:

```sh
nix develop .#build
homm1 play --data "/path/to/HEROES"     # the folder is remembered
homm1 play                              # later launches
homm1 play --retail                     # the staged retail EXE, as a Wine control
```

`--data` is an installed Windows 95 Heroes of Might and Magic folder. It must
hold `DATA/HEROES.AGG`, `SMACKW32.DLL` and `MSS32.DLL`. The game reads its
sound and video files from `D:\HEROES\SOUND` and `D:\HEROES\ANIM` on the CD; pass
`--cd` with the CD's contents (a copy or a mount), or the runner links those
folders from `--data` when it has them. Without either, sound effects and
videos are missing. CD audio music is not played.

The candidate must carry `.rsrc`: install the pinned resource compiler with
`homm1 toolchain install --id vc41 --media build/downloads/MSVC41.iso`.
`--dry-run` stops before launching. The runner is `homm1.graph.play`:

| Path under `build/game-wine/` | Contents |
| --- | --- |
| `game/` | `HEROESW.EXE` replaced on each launch; `DATA`, `MAPS` and `GAMES` copied once, so saves and high scores stay here and your folder is never written; other folders linked; the runtime DLLs |
| `cd/` | Drive `D:` as a CD-ROM when no `--cd` is given: `_autorun/autorun.exe` for the CD check and links to `SOUND`/`ANIM` |
| `prefix/` | The game Wine prefix, separate from the build prefix: 640x480 virtual desktop, `D:` as CD-ROM, the game's `AppPath`/`CDDrive` registry values |
| `play.sh` | gamescope integer-scales the 640x480 desktop (`PLAY_W`/`PLAY_H` set the output, `PLAY_GAMESCOPE=0` uses plain Wine) and stops the prefix's wineserver on exit; outside a shell with gamescope it enters `.#play` |

WinG resolves to Wine's built-in `wing32`. The WinG 1.0 SDK media pinned for the
headers ships a native `WING32.DLL`, but it thunks to 16-bit WinG, which the
WoW64 Wine cannot host, so it is not installed. No proprietary DLL is committed
or fetched; the Smacker and Miles DLLs come from your installation.

The generated `source-buka-2003` tree has no runner: its `build.py` writes
`build/<locale>/HEROES.EXE`, which runs from an installed Buka game folder
(see [clean source](clean-source.md)).

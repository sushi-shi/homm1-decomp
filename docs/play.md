# Playing the build

`homm1 play` builds and links the candidate with its resources, then runs it in
a Wine prefix it prepares from your copy of the Buka 2003 game:

```sh
nix develop .#build
homm1 play --game /path/to/game-or-cd.iso   # first run: import the game copy
homm1 play                                  # later runs
homm1 play --retail                         # the staged retail HEROES.EXE, as a Wine control
homm1 play --window -- ARGS                 # 640x480 Wine desktop window; ARGS go to the game
```

The runner is `homm1.graph.play`. The generated `source-buka-2003` tree carries
the same file as `play.py` behind `nix run .#play`, which builds the tree's own
`build/<locale>/HEROES.EXE` instead of the candidate. Both use the same per-user
state directory, so a game copy imported by one serves the other.

`--game` accepts an installed game folder, the CD (a mount or a copy of its
files), or the CD image (`.iso`) or a `.zip`/`.7z` of either; repeat it to
combine an installed folder with the CD. From the CD, unshield unpacks the
InstallShield cabinet `autorun/launch/Setup1/data1.cab` into the installed
layout and the CD's `Tracks/` music is copied. The copy is checked against the
retail file names and sizes; `DATA/heroes.agg` and the Smacker, Miles and
Audiere DLLs also against their SHA-256. Nothing is written to the copy.

| Path under `$XDG_DATA_HOME/homm1-buka/` (or `--state`) | Contents |
| --- | --- |
| `game/` | The installed game: `DATA`, `ANIM`, `SOUND`, `MAPS`, `GAMES` and the runtime DLLs, copied once and never overwritten (saves and high scores stay here); `HEROES.EXE` is replaced on every launch |
| `cd/` | Drive `D:` as a CD-ROM, holding `Tracks/`: the CD music, or links to the installed `SOUND` files when no CD was given |
| `retail/` | The copy's retail `HEROES.EXE`; the source tree's build takes its icon from it |
| `prefix/` | The game's Wine prefix, separate from the build prefix |
| `play.json` | The `--game` sources and the last locale played |

What the game needs at run time, all from the source:

- `SetupCDDrive` (kbwin.cpp) opens `DATA\HEROES.AGG` relative to the current
  folder (the runner starts the game in `game/`), then looks for
  `Tracks\02-AudioTrack 02.ogg` on a CD-ROM drive, first at the registry's
  `HMM1 CDDrive`, then on every drive `GetDriveTypeA` reports as
  `DRIVE_CDROM`. The prefix maps `D:` to `cd/` and marks it `cdrom` under
  `HKLM\Software\Wine\Drives`.
- `ReadPrefs` reads `HKLM\SOFTWARE\Buka\3DO\Heroes of Might and Magic
  Platinum\1.000`. The runner writes `AppPath` (the game folder) and `HMM1
  CDDrive` (`D:`) into the 32-bit registry view, where the 32-bit game reads
  them in the 64-bit prefix. On its first start the game finds no `HMM1
  MusicVolume`, applies its defaults and writes its settings itself.
- `PlayMusic` plays `Tracks\NN-AudioTrack NN.ogg` from the CD drive, or
  `SOUND\HEROESnn.ogg` when the music source setting is off; both through
  Audiere. Without the CD, `cd/Tracks` links each track to the installed
  `SOUND` file that holds the same piece.
- WinG is Wine's built-in `wing32`; the game ships its own Smacker, Miles and
  Audiere DLLs and loads DirectDraw from Wine.
- The Russian program's window title and message boxes are Windows-1251, so it
  runs under `ru_RU.UTF-8` (the flake supplies the locale archive).

Full screen, the game asks DirectDraw for 640x480 in 8 bits. When the display
cannot switch to that mode (an X server without the mode, such as Xvfb at
another size), `SetDisplayMode` fails and the start-up crashes in
`SetGraphicsType`, reading through a null screen bitmap; the retail executable
does the same. `--window` runs the game inside a 640x480 Wine virtual desktop,
where the mode change always succeeds, and the runner suggests it when a
full-screen start fails within seconds.

`--prefix-reset` deletes and recreates the prefix (and so the game's settings);
`--dry-run` prints each step and changes nothing. The candidate must carry
`.rsrc`; install the pinned resource compiler with `homm1 toolchain install`.

# Differences from the original program

The native port keeps the reconstructed game logic. This ledger lists every
place where the port's behaviour differs from the original Windows program,
and why. A change that alters an observable outcome adds a row in the same
commit.

## Corrected defects (shared with the Visual C++ build)

| Area | Original | Now |
| --- | --- | --- |
| High score files | Each of the ten reads asked for the size of the whole table, so the first read filled every entry and the other nine hit the end of the file; a longer file would overflow the table. | Each entry is read as its own 87-byte record. Same scores for every shipped or game-written file. |
| Map list text | Map names and descriptions were copied from the map header with `strcpy`: a description of 101 characters or more overflowed the 101-byte list field, and an unterminated name ran into the next one. | Copied bounded to the field and terminated; a too-long description is cut at 100 characters. |
| Map list scan | The second pass over the folder could insert more names than the first pass counted if files appeared in between. | The second pass stops at the first pass's count. |
| Archive lookup | A resource id not in the archive read the directory entry one past its end before reporting the error. | The bound is checked first; the same error is reported. |
| Archive directory | The entry count and offsets were trusted. | A directory that does not fit the file, or an entry outside the file, is refused with the archive error. Every shipped archive passes. |
| Map extra records | The record count and lengths came straight from the file into a 255-entry table and heap blocks read through record structures. | A count beyond the table or a length beyond the file is reported as a file error; each block is at least as large as the largest record. Every shipped map passes unchanged. |
| Truncated files | Short reads were ignored and play continued with partly loaded state. | A truncated save or map is reported as a file error. |
| Save names | A save name was used as a `printf` format (`sprintf(genName, filename)`, `sprintf(m_saveName, filename)`): a `%` in a typed name read arbitrary memory. | The name is copied. |
| New map | `NewMap` copied the map name onto itself (`strcpy(gMapName, gMapName)`), which is undefined. | The copy is skipped when source and destination are the same. |
| Saving | The file was truncated, then written; a failure in between lost the old save. | Natively, the new save or map replaces the old one only once completely written. The Visual C++ build writes as before. |
| Editor terrain blending | `BlendTerrain` tested the map edges against 72, one past the last cell, and so read terrain beyond the cell grid for cells on the east and south edges (and, for the north-east neighbour, the south-east one, beyond the bottom row); the result depended on whatever followed the grid. | The edges are the last column and row; the north-east quirk is kept where the cell exists. |
| Editor map reader | The extra-record count and lengths came from the file unchecked; a record was read through a larger editing structure than its file length. | Counts and lengths beyond the table or the file make the map invalid (the editor reports a file error); each record is allocated at least as large as the editing structure. Every shipped map reads unchanged. |
| Network save decoder window | The decoder (`DecodeData` and the assembly `Decode`) never initialised its sliding window, which the encoder starts as spaces: a match into the initial window (a run of spaces in the first 4 KB of a save, such as a padded name) copied whatever the previous encode or decode had left there, zeros in a fresh process. Only reachable in network play. | `DecodeData` starts the window as spaces, as the encoder does. |
| Editor map writer order | The file was opened before the town, mine and obelisk checks ran; a file that could not be created returned before them. | The file is written after the checks; if it cannot be written the editor reports the error after them. |

## Host differences (native port only)

| Area | Original | Now |
| --- | --- | --- |
| Settings | Registry key `HKLM\SOFTWARE\Buka\3DO\Heroes of Might and Magic Platinum\1.000`; the modem init string was stored with a length of 4 bytes. | `$XDG_CONFIG_HOME/homm1/heroes.cfg`, same value names; the init string is stored whole. |
| First start | Full screen. | In a window (F4 switches, as in the original). Later starts use the saved choice; `--window` and `--fullscreen` override it. |
| Music source | The CD's `TRACKS` folder on a CD drive (asking for the CD when absent). | The `Tracks` folder in `$HOMM1_CD`, a `cd` folder beside the game folder, or the game folder; without it, the installed digital music in `SOUND`. |
| Menu bar | A Windows menu bar in windowed mode (options, window sizes, help). | Drawn by the port in the same place with the game's small font and Windows' colours, for the game and the editor; an open menu runs a modal loop as Windows' did. Window-size commands size the window. |
| Help and About | WinHelp file `HELP\HEROES.HLP`; an About dialog box. | Help is unavailable and its item greyed; About shows the About box's text in a message box. |
| Network and serial play | NetBIOS, modem and direct cable. | Not available: choosing them reports the original's "NetBIOS not found" or serial error. |
| Single instance | A named event refused a second copy. | No check. |

## Retail defects kept

| Area | Behaviour |
| --- | --- |
| Remote message filter | `RemoteMain` clears 30 bytes of the 30-entry `i32` recent-message table, leaving most of it from a previous session. Only reachable in network play, which the port does not offer. |
| Drawing routines | The portable drawing routines reproduce the assembly they replace, including its edge cases: dimmed sprites step rows by a fixed 640 bytes, `TileToBitmap` accepts a tile index equal to the tile count, and a flipped clipped sprite placed wholly left of the target reads before its run. None is reached by the shipped data in the tested paths; the bit routines and loops with zero or negative counts, which in the assembly read past a byte or ran away, are bounded. |
| Network save compressor | `EncodeData` with an empty input wraps a 16-bit length and overruns its output. `DecodeData` trusts the stream: it writes the length the stream claims (the receiver's buffer holds 78,000 bytes) and reads input until it has produced it. The portable decoder adds `DecodeDataBounded`, which refuses a stream larger than its output or shorter than its code and reads nothing outside the given input, for the network transport to use. Only reachable in network play. |
| Repeated pad byte | Saved games store the second byte of a player's two-byte unknown field twice; kept for file compatibility. |

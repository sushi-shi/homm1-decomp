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
| Network save stream length | `TransmitSaveGame` sent as many bytes as `EncodeData` returned, its code without the four-byte length the stream begins with, so the stream's last four bytes never left; the receiver decoded the save's last bytes (the end of the map visit flags) from whatever its buffer held. | The whole stream is sent. A receiver of the original takes it unchanged. Saves from the original still arrive short. |
| Network save decoder window | `DecodeData` started from the window the last compression in the program had left (zero before the first), while the encoder starts from spaces and refers to them for early runs of spaces: the padding of the map name arrived as zeros. | The decoder starts from spaces, so it decodes the original's streams right too. |
| Network save buffers | The compressor's output buffer was as large as the save, although a stream can be longer; the receiver trusted the peer's save size, segment indexes and acknowledgement requests as indexes into its 500-entry tables and its buffer, and the compressed save's decoded size for its decode buffer. | The output buffer has room for twice the save. A size beyond the tables or a decoded size beyond the buffer refuses the transfer; segments and requests outside them are ignored. Every transfer between two programs is unchanged. |
| Network messages | Messages were copied to and from the wire as whole structures; a message's own payload size was not checked against its packet, nor a chat line's length against the 60-byte line it is copied into; bytes left uninitialized (the guest count's padding, a player exit's third byte, the second half of an acknowledgement map, a battle hand-off without a town) went out as they were. | Each message is encoded and decoded field by field in the original's layout (`remoteRecords.h`). A message whose size disagrees with its packet is dropped like a bad CRC; chat lines are cut to the line; uninitialized bytes are sent as zero. |
| Editor map writer order | The file was opened before the town, mine and obelisk checks ran; a file that could not be created returned before them. | The file is written after the checks; if it cannot be written the editor reports the error after them. |

## Host differences (native port only)

| Area | Original | Now |
| --- | --- | --- |
| Settings | Registry key `HKLM\SOFTWARE\Buka\3DO\Heroes of Might and Magic Platinum\1.000`; the modem init string was stored with a length of 4 bytes. | `$XDG_CONFIG_HOME/homm1/heroes.cfg` (`%APPDATA%\homm1\heroes.cfg` on Windows, the site's IndexedDB in a browser), same value names; the init string is stored whole. The file is replaced atomically, like a save. |
| First start | Full screen. | In a window (F4 switches, as in the original). Later starts use the saved choice; `--window` and `--fullscreen` override it. |
| Music source | The CD's `TRACKS` folder on a CD drive (asking for the CD when absent). | The `Tracks` folder in `$HOMM1_CD`, a `cd` folder beside the game folder, or the game folder; without it, the installed digital music in `SOUND`. |
| Menu bar | A Windows menu bar in windowed mode (options, window sizes, help). | Drawn by the port in the same place with the game's small font and Windows' colours, for the game and the editor; an open menu runs a modal loop as Windows' did. Window-size commands size the window. |
| Help and About | WinHelp file `HELP\HEROES.HLP`, opened at its contents (`HELP_FINDER`); an About dialog box. | Help converts the book to one HTML page and shows it in the system's browser (a new tab in the browser build), opening at the contents and index; WinHelp's pop-up and secondary windows become in-page links, macros are not run. The item is greyed only when the file is missing. About shows the About box's text in a message box. |
| Network play | NetBIOS sessions on the local network; the host's name found by broadcast. | TCP: the host listens on a port (`--port`, default 1995), the guest joins `--join ADDRESS[:PORT]` or the host whose broadcast (the original's datagram, over UDP) it receives. Name registration always succeeds; a host whose port is taken waits as for a guest. Plays other native programs only. |
| Serial play | A COM port at the chosen speed, with a modem or a cable. | A TCP stream: for a direct connection the host listens and the guest connects; for modem play a Hayes modem is emulated (RING, ATA, ATDT with the address typed as the number). The chosen port and speed are kept but unused, and the line has no speed limit. A break (never sent by the game) is ignored. |
| Random numbers | The Microsoft C runtime's `rand`. | The same generator, in every native build (the C library's own gave other numbers). |
| Single instance | A named event refused a second copy. | No check. |
| Game data in a browser | Installed from the CD. | The page copies the player's game folder (and, if given, the CD's music and the help file) into the site's IndexedDB once; saved games and maps are written back there and can be downloaded from the page. |
| Window in a browser | A window of the chosen size. | A canvas of the game's size (menu bar included) that the page scales to the browser window; the window-size commands change the canvas, full screen is the browser's. |
| Log on Windows | None. | The native Windows programs are GUI programs; their log goes to a redirected stderr, the parent console or `%APPDATA%\homm1\homm1.log`. |

## Retail defects kept

| Area | Behaviour |
| --- | --- |
| Remote message filter | `RemoteMain` clears 30 bytes of the 30-entry `i32` recent-message table, leaving most of it from a previous session, where an old message id can make a new message look repeated. |
| Drawing routines | The portable drawing routines reproduce the assembly they replace, including its edge cases: dimmed sprites step rows by a fixed 640 bytes, `TileToBitmap` accepts a tile index equal to the tile count, and a flipped clipped sprite placed wholly left of the target reads before its run. None is reached by the shipped data in the tested paths; the bit routines and loops with zero or negative counts, which in the assembly read past a byte or ran away, are bounded. |
| Network save compressor | `EncodeData` with an empty input wraps a 16-bit length and overruns its output; the game never sends an empty save. A corrupt compressed stream (only from a faulty peer: every packet is checked by its CRC) can make the decoder read past the received data, as the original's could. |
| Repeated pad byte | Saved games store the second byte of a player's two-byte unknown field twice; kept for file compatibility. |

# Differences from the original program

The native port keeps the reconstructed game logic. This ledger lists every
place where the port's behaviour differs from the original Windows program,
and why. A change that alters an observable outcome adds a row in the same
commit.

## Corrected defects (shared with the Visual C++ build)

| Area | Original | Now |
| --- | --- | --- |
| High score files | Each of the ten reads asked for the size of the whole table, so the first read filled every entry and the other nine hit the end of the file; a longer file would overflow the table. | Each entry is read as its own 87-byte record. Same scores for every shipped or game-written file. |
| High score names | The high score screen passed each entry's player and scenario names from the file to `sprintf` as the format: a `%` in a name read, and with `%n` wrote, through arguments that were never passed. | The names are shown as they are. Every shipped or game-written table shows the same. |
| Map list text | Map names and descriptions were copied from the map header with `strcpy`: a description of 101 characters or more overflowed the 101-byte list field, and an unterminated name ran into the next one. | Copied bounded to the field and terminated; a too-long description is cut at 100 characters. |
| Map list header values | A map header's size and difficulty indexed the 3 size names and 5 difficulty names the scenario list shows. | A header whose size or difficulty is out of range is treated as absent (the file name is listed, as for a map without a header). |
| Long file names | The file requester copied listed names into fields sized for 8.3 names (`gMapName` holds 12 characters, an extension 4): a longer name, which Windows 95 and the native hosts list, overflowed them. The selected map's name was also used as a `sprintf` format. | Names longer than 8.3 are left out of the list; the map name is shown as text. |
| Map list scan | The second pass over the folder could insert more names than the first pass counted if files appeared in between. | The second pass stops at the first pass's count. |
| Archive lookup | A resource id not in the archive read the directory entry one past its end before reporting the error. | The bound is checked first; the same error is reported. |
| Archive directory | The entry count and offsets were trusted. | A directory that does not fit the file, or an entry outside the file, is refused with the archive error. Every shipped archive passes. |
| Archive resources | Icons, bitmaps, tilesets and palettes were read with the sizes their data claimed, past their archive entry (uninitialised memory at the end of the file; an allocation of up to 4 GB); an icon's frame table and frame command offsets were not checked against its data; a backdrop read row by row into the screen was not checked against it; a font's glyph icon name was not terminated and its icon not checked for the 162 glyphs the text routines index; a pointer bitmap shorter than its 32 x 32 pixels left them to whatever followed. | A resource that does not fit its archive entry or the structure it is read through is reported as a damaged archive, the error a damaged archive directory gives. The whole shipped archive loads, and all 150 pointer bitmaps are full size. |
| Icon and tile drawing | The drawing routines followed a frame's commands wherever they led: past the icon's data, and with rows wider or more numerous than the frame's, past the destination. The C++ clip routines did the same, and the one used for monsters on the adventure map wrote left of its clip rectangle for a frame cut by it. `TileToBitmap` drew index == count from beyond the tileset and assumed square tiles when flipping. | Every icon and tile routine reads only inside the icon's or tileset's data (a command past it ends the frame, a pixel past it is 0) and writes only inside the destination's pixels; tile index == count draws nothing. Frames that fit draw as before, including the clip routine's offset draw inside the destination. |
| Text layout | `LineLength` counted lines forever for a word wider than the line, `LineWidth` never got past a line break, and `DrawBoundedString`, for such a word, read and wrote before its copy of the text and drew the rest of the text on every line. | A word wider than the line is broken before the glyph that overflows; a line break ends a line. Text that fits is laid out as before. The survey's random input found the same hang typing a long word into the editor's map description, and its display check the text drawn past the field; test also in `game_regressions_test`. |
| Map extra records | The record count and lengths came straight from the file into a 255-entry table and heap blocks read through record structures. | A count beyond the table or a length beyond the file is reported as a file error; each block is at least as large as the largest record. Every shipped map passes unchanged. |
| Truncated files | Short reads were ignored and play continued with partly loaded state. | A truncated save or map is reported as a file error. |
| Saved game players | The player count, the current player and the human flags came from the file unchecked: the current player indexed the player table, a count of 0 divided by zero and a file without a human among its players made the search for the player to watch loop forever. The game difficulty, map size and map difficulty indexed the tables of their names. | A saved game whose player count is not 1 to 4, whose current player is not one of its players, that has no human among them or whose difficulty or map size is out of range is reported as a file error. Every shipped saved game loads unchanged. |
| Save names | A save name was used as a `printf` format (`sprintf(genName, filename)`, `sprintf(m_saveName, filename)`): a `%` in a typed name read arbitrary memory. | The name is copied. |
| New map | `NewMap` copied the map name onto itself (`strcpy(gMapName, gMapName)`), which is undefined. | The copy is skipped when source and destination are the same. |
| Saving | The file was truncated, then written; a failure in between lost the old save. | Natively, the new save or map replaces the old one only once completely written. The Visual C++ build writes as before. |
| Editor terrain blending | `BlendTerrain` tested the map edges against 72, one past the last cell, and so read terrain beyond the cell grid for cells on the east and south edges (and, for the north-east neighbour, the south-east one, beyond the bottom row); the result depended on whatever followed the grid. | The edges are the last column and row; the north-east quirk is kept where the cell exists. |
| Editor map reader | The extra-record count and lengths came from the file unchecked; a record was read through a larger editing structure than its file length. | Counts and lengths beyond the table or the file make the map invalid (the editor reports a file error); each record is allocated at least as large as the editing structure. Every shipped map reads unchanged. |
| Network save stream length | `TransmitSaveGame` sent as many bytes as `EncodeData` returned, its code without the four-byte length the stream begins with, so the stream's last four bytes never left; the receiver decoded the save's last bytes (the end of the map visit flags) from whatever its buffer held. | The whole stream is sent. A receiver of the original takes it unchanged. Saves from the original still arrive short. |
| Network save decoder window | `DecodeData` started from the window the last compression in the program had left (zero before the first), while the encoder starts from spaces and refers to them for early runs of spaces: the padding of the map name arrived as zeros. | The decoder starts from spaces, so it decodes the original's streams right too. |
| Network save buffers | The compressor's output buffer was as large as the save, although a stream can be longer; the receiver trusted the peer's save size, segment indexes and acknowledgement requests as indexes into its 500-entry tables and its buffer, and the compressed save's decoded size for its decode buffer. | The output buffer has room for twice the save. A size beyond the tables or a decoded size beyond the buffer refuses the transfer; segments and requests outside them are ignored. Every transfer between two programs is unchanged. |
| Network messages | Messages were copied to and from the wire as whole structures; a message's own payload size was not checked against its packet, nor a chat line's length against the 60-byte line it is copied into; bytes left uninitialized (the guest count's padding, a player exit's third byte, the second half of an acknowledgement map, a battle hand-off without a town) went out as they were. | Each message is encoded and decoded field by field in the original's layout (`remoteRecords.h`). A message whose size disagrees with its packet is dropped like a bad CRC; chat lines are cut to the line; uninitialized bytes are sent as zero. |
| Editor map objects | The cells' references were trusted: a hero or town cell naming an extra record that was not loaded was read through a null or freed pointer, a hero record's hero id indexed a 36-entry table, an artifact cell's number a 37-entry table, a mine in the last column had its resource marker read from beyond the map, and a cell whose secondary trigger is a hero became a hero without a record when the object over it was cleared. | A map with such a cell is invalid (the editor reports a file error). Every shipped map reads unchanged. |
| Editor map writer order | The file was opened before the town, mine and obelisk checks ran; a file that could not be created returned before them. | The file is written after the checks; if it cannot be written the editor reports the error after them. |
| Failed saves | A saved game whose file could not be created ended the program with the file error; failed writes (a full disk) were ignored and left a cut-off save. | The error is shown and play goes on; the previous save is kept (natively the file is replaced atomically). The network transfer file is still fatal. Test: `save_diskfull`. |
| Adventure route map | `advManager::Close` freed the route map with `delete`, not `delete[]`. | `delete[]`. No visible change. |
| Spell book and the artifact table | `GiveArtifact` recorded every artifact's owner in the 37-entry random artifact table; the spell book (37) overwrote the byte after it, the first boat's id (Tournament Edition X21). | The spell book is not recorded. |
| Computer player: obelisk value | The base value of the ultimate artifact was read before testing for none, at index -1 once it had been dug up; the value was not used. | Tested first. No change in play. |
| Path search at the map's edge | `TestPossibleDirections` let up to six cells beyond the west and north edges through (`x <= -7`) and read the visibility table outside the grid for them; the computer's search then read search nodes and the monster table outside the grid for them. `PushPoint` refused such cells, so they were never reached. | Off-map neighbours are refused at once. No change in routes. |
| Path search: monsters on the edge | For a monster on the map's edge the computer's search read the search nodes beside it outside the grid (the tail of the search queue on the west edge, memory past the search array on the east edge) and could count the monster as reached from a stale node. | Neighbours outside the grid are skipped. |
| Route tracing | `BuildPath` followed a route back through cells the search had not reached and could leave the grid; on failure it kept the steps it had collected, and `ShowRoute` drew them from a start outside the grid, writing before the route map. | A trace that leaves the grid fails, and a failed trace has no steps. |
| Search without a current hero | `SeedPosition` took the address of hero -1 for a player with no current hero (unused). | No address is taken. |
| Tavern heroes of absent players | `PerWeek` tested the byte before the hero availability table for the empty (-1) tavern slots of players not in the game, and set it to -1 when it held 0x40. | Empty slots are skipped; the random draws are unchanged. |
| Computer player: buying a hero | Valuing a tavern hero, whose owner is -1, read the upgrade weight from before the player table. | The buyer's (the current player's) weight is used. |
| Mine flags at the map's edge | `ClaimMine` puts the owner's flag on a cell above or left of the site; for a site on the top rows or left columns (W95M1234's alchemist's lab at (40,1)) it wrote outside the grid: onto the bottom cell of the column to the left, or before the grid into player data. | Such a site shows no flag. |
| Wide stacks at the battlefield's edge | `AttemptAdjacentAttack` (computer and neutral stacks) tested its second alternative also for a direction off the battlefield (hex -1), reading the bytes before the hex table; a wide stack on a start hex reaches it in most battles. | Off-battlefield directions are skipped. |
| Area spells off the battlefield | The computer's fireball evaluation (`EffectSpellDamage`) read hex -1 for a neighbour off the battlefield and, when its bytes looked like a stack, marked a stack outside the table (Tournament Edition X17). | Skipped. |
| Spell targets of vacated hexes | The computer's spell evaluation took the address of army -1 for hexes a stack had left (unused). | No address is taken. |
| Dimension Door | A target on the border beyond the map's edge was tested as cell (0,0) and could teleport the hero off the map (Tournament Edition X28). | The spell fails as on unsuitable ground. |
| Clicks on the border | At the map's edge the adventure view shows the border beyond it; clicking there looked the cell up in the visibility table outside the grid (mostly 0xFF, so "explored"), then in the search nodes, and a right click described cell (0,0). | Border cells count as unexplored: a right click shows the border text, a left click does nothing. |
| Quick info for unnamed objects | The object name table has 63 entries; cells on the shipped maps carry object types 63 to 127, whose quick info read the tables after it (the town names, then the event texts). | The same texts are chosen without leaving the tables. |
| Quick info for odd resource and monster cells | A resource or monster cell whose picture is not one of theirs (two on PNM31234) indexed the name tables far past their end. | The object type's name is shown. |
| View World trees | The small tree icons have 98 frames, the map's trees 101; the last three were drawn from a frame entry past the table (about 25 shipped maps). | They are not drawn. |
| View World mine letters | For a hero standing on a mine the letter came from mine record number *hero id*, up to frame 23 of 7 (Tournament Edition TE-FIX-8). | From the mine the hero stands on. |
| Town Gate without towns | After "no town" the spell went on and teleported the hero into the town named by the byte before the town list. | It stops after the message. |
| T key without towns | The town key tested `m_townCount >= 0` and selected the stale or empty (-1) first entry. | Nothing happens without towns. |
| Steps off the map | A keypad step off the edge fetched cell (0,0) and ran its event (with coordinates -1 or 72, writing outside the sound table) before `ValidMove` refused the step. | The step is refused first. |
| Skeletons with no artifacts left | With all artifacts in play the skeleton's message named artifact -1; the gold paid instead was not mentioned. | The gold message is shown. |
| Stale combat spell target | The spell target was remembered from the last pointer move across casts and combats; a click without moving cast at a hex that could since have emptied, and the targeted spell used a missing stack. | The target is checked again on the click. |
| Campaign crests | `RandomizePlayerCrests` read a fourth crest from a three-entry table (the next field, 30, which is no crest). | The fourth player's crest is random, as it was. |
| Editor: random map dialog | Closing the dialog read the terrain share before the table (`BalanceTerrainPercents(-1)`), about 2e-314 in the shipped editor. | 0 is used. Same shares. |
| Editor: extra record table | Each town or hero placed takes a record that is not given back when it is erased; after 255 the table overflowed and its size table overwrote the record pointers. | A placement that needs a record is refused when the table is full. |
| Editor: record allocation | Placed towns and heroes were allocated with `new` and freed with `free`. | Allocated with `malloc`. |
| Editor: Undo after New or Random Map | The undo copy still named the old map's records, which New had freed; Undo brought back cells pointing at them. | The new map is the undo state. |
| Editor: castles that find no site | `PlaceTowns` went on with an uninitialised site when every candidate rated 0 or less; its "no site" check could not fire. | The attempt ends and the generator tries again (as for too few castles). |
| Editor: object pieces off the map | `PlaceOverlay` checks only the cells an object stands on; its other pieces (shadows) at the map's edge were written outside the grid (past the last column into the object id table, above the top row onto the previous column's bottom cell). | Pieces outside the map are not placed. |
| Editor's load requester | Selecting the requester's name field asked it for its text; in the load requester it is not a text entry and does not answer, and the original copied from whatever the reply's text pointer held. | Nothing is copied when there is no answer. |
| Uninitialised memory (valgrind) | The first fade-out faded the working palette before anything was set in it; the adventure manager compared its current terrain before setting it; a combat stack's luck and spell countdown were not set when it was created, and the combat drawing read the stack slots past each side's stacks. Each held whatever the memory held: zero in the original's freshly allocated objects in practice, a previous battle's values later. | They start at black, no terrain, no luck, no spell, and hidden. |
| Teleporting out of sight | The current hero's position is the view's centre (`DemobilizeCurrHero` takes it from there) and `MoveHero` moves the view with every step, shown or not, but `TeleportTo` moved the view only when the teleport was shown. A computer player's Dimension Door out of the watching player's sight was undone when the hero was put down (after the spell and the movement were spent), and from a boat the hero was put down onto the boat's or another hero's cell, which left a hero cell behind; its later owner of -1 was drawn with a flag icon read from before the player table (a crash in the survey). | The view always follows the teleport, so the hero arrives where the spell sends it; a hero without an owner is drawn without a flag. The survey checks after every computer turn that each hero cell names a hero standing there. |
| Heroes the map places | Starting heroes and the taverns' heroes were drawn among all heroes, including those the map places; a hero drawn for a player (the campaign's lord scenarios give player 0 one) or offered in a tavern that the map also placed was given twice, which left hero cells naming a hero standing elsewhere or without an owner (and the computer then read the player table at -1 attacking it). | The heroes the map places are reserved before the draws. |
| Boats on an occupied dock | The computer bought a boat onto its shipyard's dock whatever stood there (a hero in a boat included), and the shipyard's test against the selected hero compared the cursor's position in the view with the dock's on the map, so a boat could be built under the hero; the hero's cell became the boat's, another hero could board it on the same cell, and hero cells were left behind as above. A dock below the map's bottom row was outside the grid. | Boats are built only onto a free dock inside the map (the Tournament Edition's TE-FIX-1 and TE-FIX-2). |
| Editor: tool managers | The terrain, object, details and eraser tools were deleted through `baseManager`, which has no virtual destructor. | Each is deleted as its own type. No visible change. |
| Adventure commands without a hero | With no hero selected, `DoAdvCommand` used hero -1 (the bytes before the hero table): unused for the town view, but a move command left over from a hover while a hero was selected moved it; the search key (`ProcessSearch`) also searched with it. | Without a selected hero a left-over move command is dropped and the search key does nothing. Found by the survey's random input. |
| Map contents | The game trusted what a map's cells and records name: object types, pictures, town, mine and boat numbers, extra record numbers, the heroes and garrisons in them, ground tiles, the number of obelisks. An edited or damaged map indexed outside the tables and the map with them (town -1, hero 40, creature 200, owner -1 written into the player table, a ninth hero per player, more than 48 obelisks). | `game::MapDataValid` checks them when a game starts on a map (`NewMap`), and a map that breaks them is refused with the file error, like an unreadable one. Every shipped map and campaign map passes. Test: `game_regressions_test`. |
| Starting hero without a town | On a map that names its towns' owners, a player left without a town got its starting hero at the record before the town table. | No starting hero for such a player. |
| Ultimate artifact placement | With one human player who had no hero, the distance test read the hero before the hero table. | The distance test is skipped then. |
| Random artifacts beyond the supply | With more random artifact sites than artifacts the draw found none (-1, stored as 255) and the site wrote entry 255 of the 37-entry artifact table (into the boats). | Such a site is left empty. |
| Site masks | Gazebo sites are numbered from 1 into a 32-bit mask; with 32 or more the shift was 32 or more, which x86 masks. | The mask is written out. Same result. |
| Computer player: tavern heroes as defenders | Valuing a fight against a hero without an owner read the variable before the human player table. | No owner counts as a computer player. |
| Missing movies | A movie missing from `ANIM` (a game folder copied without it) was played as a null movie, which crashes on its first use (the native port ended with a segmentation fault). | The movie is skipped as if it had played to its end, so the next one (or the main menu) follows; the native port logs it. |

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
| Remote message filter | `RemoteMain` clears 30 bytes of the 30-entry `i32` recent-message table, leaving most of it from a previous session, where an old message id can make a new message look repeated. The Tournament Edition clears the whole table (`BUG-NET-2`). |
| Drawing routines | The portable drawing routines reproduce the assembly they replace, including its edge cases: dimmed sprites step rows by a fixed 640 bytes, a flipped clipped sprite placed wholly left of the target reads before its run, and a clipped frame starts its right-edge test from the previous draw's column. None is reached by the shipped data in the tested paths; the bit routines and loops with zero or negative counts, which in the assembly read past a byte or ran away, are bounded, and no routine reads outside the icon's data or writes outside the destination. |
| Network save compressor | `EncodeData` with an empty input wraps a 16-bit length and overruns its output; the game never sends an empty save. A corrupt compressed stream (only from a faulty peer: every packet is checked by its CRC) can make the Visual C++ build's decoder read past the received data, as the original's could; the native build decodes it with `DecodeDataBounded`, which reads nothing past the receive buffer and refuses the transfer when the stream ends before its save. |
| Tick count | The game compares its millisecond clock as signed 32-bit values; at 2^31 a delay's deadline lies "behind" the clock and `DelayTil` ends the program with an assertion. The original's clock counted from Windows start-up, so this happened after 24.8 days of uptime; the port's counts from 1,000,000 at start-up, so it takes 24.8 days of running the game. `HOMM1_TICK_START` reproduces it. |
| Repeated pad byte | Saved games store the second byte of a player's two-byte unknown field twice; kept for file compatibility. |

## Retail gameplay bugs (kept)

Found while hunting the defects above. They give wrong results without
memory errors, so the port keeps them, as the Visual C++ build does: plain
`port` stays faithful to the original game. The Tournament Edition
(`source-te`, and `port-te` on the native port) fixes them, the rows
marked TE as the edition itself does, the others as the `BUG-*` rows of its
`docs/te/changes.tsv`, which record each fix and its reproduction; rows it
keeps as the original game has them say so. The last column names the
fix.

| Area | Bug | Reproduction | On the Tournament Edition |
| --- | --- | --- | --- |
| Skeletons | A hero with all 14 artifact slots full is told he finds 1000 gold, but none is given (`DoEvent`, `SKELETON_ARTIFACT`). | Visit a skeleton holding an artifact with a hero carrying 14 artifacts; the treasury does not change. | Fixed (`BUG-ADV-1`). |
| Ghost retaliation (TE X10) | A retaliating ghost stack grows from its kills twice. | Attack ghosts that kill something in retaliation; compare their count with the kills. | Fixed by the edition (X10). |
| Bad luck (TE X11) | Bad luck triggers on `SRandom(1,12) < -luck`, good luck on `<= luck`, so bad luck is rarer. | `army::CheckLuck`. | Fixed by the edition (X11). |
| Auto-resolved losses (TE X12) | Losses roll 0..100 against the kill chance. | `armyGroup::DamageGroup`. | Fixed by the edition (X12). |
| Duplicate stacks (TE X13) | `UpdateArmyGroup` writes every combat stack's survivors to the first slot of its creature type: an army with two stacks of one creature loses or duplicates troops after a battle. | Fight with two separate stacks of peasants, lose some of the second; the first stack takes the losses. | Fixed by the edition (X13). |
| Wandering monster count (TE X14) | Survivors are summed into the 8-bit cell byte and wrap above 127/255. | Leave more than 127 monsters alive after a fight with a wandering stack. | Fixed by the edition (X14). |
| Experience (TE X15) | Stacks that grew in combat (ghosts, resurrection) subtract experience. | `ExperienceValueOfStack`. | Fixed by the edition (X15). |
| Map-placed heroes (TE X27) | Heroes placed on the map start with movement computed before their artifacts and owner are known. | A map hero carrying boots starts with normal movement on day 1. | Fixed by the edition (X27). |
| Tavern (TE X30) | `PerWeek` does not reserve the heroes it offers, so one hero can be offered in two taverns (and bought twice). | Play several weeks with 3–4 players and compare the taverns. | Fixed by the edition (X30). |
| Obelisks (TE X32) | Pieces per obelisk are 48 / count rounded down: visiting every obelisk need not complete the puzzle. | A map with 18 obelisks: 2 pieces each, 12 never revealed. | Fixed by the edition (X32). |
| Ultimate artifact hint (TE X33) | The hint chance is a signed byte; above 127 it is negative. | `ComputeUALoc`. | Fixed by the edition (X33). |
| Shipwreck (TE X34) | The shipwreck rewards from the cell the hero came from, not its own. | `DoEvent`, `MAP_OBJECT_SHIPWRECK`. | Fixed by the edition (X34). |
| AI army value (TE X36) | Morale and luck scaling multiply integers that overflow for large armies. | `philAI::FightValueOfStack` with a very large army. | Fixed by the edition (X36). |
| Elves' second shot (TE-FIX-3) | Elves fire their second shot without a shot left. | Elves with one arrow left. | Fixed by the edition (TE-FIX-3). |
| AI auto-resolve (TE-FIX-4) | When the defender wins `QuickCombat` applies the win to the attacking hero and gives the defender the defender's own experience. | Two computer heroes fight; the defender wins. | Fixed by the edition (TE-FIX-4). |
| Recruit maximum (TE-FIX-7) | The affordable count is stored in 16 bits before it is limited by the creatures available; a large treasury wraps it. | More than 32767 affordable creatures. | Fixed by the edition (TE-FIX-7). |
| Town footprint (TE-MAP-1) | Only the town's entrance cell names the town. | `SetupTown`. | Fixed by the edition (TE-MAP-1). |
| AI spell evaluation | Bless and Curse are valued on stacks whose damage is a single value (TE-UNR-1). | `DetermineEffectOfSpell`. | Fixed by the edition (TE-UNR-1). |
| AI creature purchase | The value of the stack a purchase would replace uses `gMonsterDatabase[slot]`, the slot number as a creature. | `philAI::EvaluateOneTimeCreaturePurchase`. | Fixed (`BUG-AI-1`). |
| AI resources | `MaxBuyableCreatures` returns only the last resource's (gold's) count. | `philAI::MaxBuyableCreatures`. | Fixed (`BUG-AI-2`). |
| AI sites | `ValueOfEventAtPosition` tests `gMapVisitFlags[x][y] && gCurPlayerBit` (`&&` for `&`): any player's visit counts. | A site visited only by another player. | Fixed (`BUG-AI-3`). |
| AI routes | `BuildPath`'s check that a node belongs to its cell uses `&&` where `||` is meant, so stale routes pass (also drawn as bogus routes). | A node whose x matches its cell but whose y does not. | Fixed (`BUG-AI-4`). |
| AI relocation | `ResetHeroRVs` uses `m_x` for the y distance. | Two heroes in one column, 30 cells apart. | Fixed (`BUG-AI-5`). |
| AI artifacts | The computer ignores `GiveArtifact`'s "no free slot" and loses the artifact with 14 slots full. | A computer hero carrying 14 artifacts on an artifact, a skeleton, a daemon cave or a huge ghost site. | Fixed (`BUG-AI-6`). |
| Berserk | `GoBerserk`'s flying branch can loop without end; its walking branch can leave no action set. | Berserk on a flier with nothing in reach, or on a walker hemmed in by obstacles. | Fixed (`BUG-CMB-1`). |
| Combat stats | `army::Init` adds the hero's Attack and Defense into 8-bit fields, which wrap above about 100. | A hero with Attack 120. | Fixed (`BUG-CMB-2`). |
| Summon Boat on the edge | Next to the map's edge the boat is summoned beside (0,0) with a coordinate of -1 or 72. | Cast Summon Boat with the hero on the edge of SEL21234. | Fixed (`BUG-ADV-2`). |
| Campfire | Clears the ambient sound at the view's centre instead of the campfire's cell. | A computer hero takes a campfire away from the view's centre. | Fixed (`BUG-ADV-3`). |
| Puzzle | Off-map cells of the puzzle show cell (0,0). | An ultimate artifact near the map's edge. | Fixed (`BUG-ADV-4`). |
| Stray hero and town cells | Some shipped maps (DNL3, AES3, PNM3, UHS6) have hero or town triggers with no record; clicking them shows hero or town 0, and selects it if owned. | PNM31234: defeat the monsters at (7,45) or (33,61) and click their cells. | Fixed (`BUG-ADV-5`). |
| Campaign crests | The crest table should be read at `[i-1]`, so the enemy lords' crests are random. | Campaign scenarios 5-8: the enemy lord's crest. | Fixed (`BUG-CAM-1`). |
| Thieves' Guild | Crystal is counted with wood and ore in the Thieves' Guild view (`TOWNMGR`). | 20 crystal and no wood or ore. | Kept as retail: the grouping ("Wood, Crystal & Ore") was intended (`BUG-TWN-1` reverted). |
| Recruiting a hero | `RecruitHero` sets the owner of both tavern heroes. | With the original tavern, the same hero offered in two taverns. | Fixed (`BUG-TWN-2`). |
| Weekly monster growth | `PerWeek` adds 1 to 10 to a recruiting site's 8-bit stock (`m_objectMetadata +=`). | The stock grows only while below 100, so it stays at 109 or less: it cannot wrap. | Not a defect. |
| Editor generator | A broken bubble sort ranks the regions; `minX--` for `minY--`; `direction % 1`; `castleRegion[c-1]`; road destinations take `.x` for `y`; the road trace stops on its destination; a missing `else` places stone liths twice on desert. | Random maps (MAPOBJ.cpp). | Fixed (`BUG-GEN-1–5`). |
| Editor | A `u8 != -1` test is always true (EDITMGR.cpp, map writer); the vertical scroll knob clamps x; `PaintGround` redraws only its first cell; extra records are not freed on erase or undo. | Save a map without a lighthouse; place and erase towns until the record table fills. | Fixed (`BUG-EDT-1–4`). |

## Tournament Edition fixes

The Tournament Edition (`docs/te/catalogue.md` on `source-te`) lists 26 bug
fixes. Those that remove a crash or undefined behaviour are in the port too,
as the rows above; the others change play and stay in the edition.

| TE row | Kind | In the port |
| --- | --- | --- |
| X17 AI spell evaluation off the board | memory (hex -1) | Yes: "Area spells off the battlefield". |
| X21 spell book and the boat table | memory (write past table) | Yes: "Spell book and the artifact table". |
| X28 Dimension Door outside the map | memory (off-map hero) | Yes: "Dimension Door". |
| TE-FIX-8 View World mine letters | memory (icon frame past table) | Yes: "View World mine letters". |
| X23 experience never negative | crash (assertion) only with negative experience from data | Map heroes with negative experience are refused ("Map contents"); the added rule stays TE. |
| X10, X11, X12, X13, X14, X15, X27, X30, X32, X33, X34, X36 | gameplay / balance | No (listed above). |
| TE-FIX-1, TE-FIX-2 boats onto an occupied dock | state corruption (hero cells left behind, later index -1) | Yes: "Boats on an occupied dock". |
| TE-FIX-3, TE-FIX-4, TE-FIX-7 | gameplay | No (listed above). |
| TE-MAP-1 | gameplay | No. |
| TE-UNR-1 | AI behaviour | No. |
| TE-UI-1 spell book forward path | interface | No. |
| TE-QOL-6 catapult residue | display (no memory error) | No. |
| TE-RU-1 Russian end sequence | none (same behaviour) | No. |

## The Tournament Edition (port-te)

On `port-te` the game is the Tournament Edition; `docs/te/README.md`
("The edition on the native port") lists each platform row's native form,
the save format and protocol through the codecs, and which of the retail
gameplay bugs above the edition fixes. Host differences of port-te besides
those of the port:

| Area | Edition on Windows | port-te |
| --- | --- | --- |
| Settings | Registry key `…\HeroesWorld TE\EN` or `\RU`. | `heroes-te-en.cfg` or `heroes-te-ru.cfg` in the settings folder, for both programs. |
| Idle processor use | `Sleep(1)` per message-pump pass, 1 ms timer period, `GetMessage` every 127 ms. | A 1 ms sleep per pass natively (not in the browser); SDL's 1 ms timer period on Windows; no blocking `GetMessage`. |
| Music | `Tracks\NN-AudioTrack NN.ogg` or `Audio\Track NN.flac` in the game folder. | The same files first, then the port's CD folder, then `SOUND`. |
| Window title | The wrapper's "Heroes of Might and Magic TE". | The program's catalog title (`window.gTitle`, the editor's `editor.window.title`). |
| Network peers | NetBIOS group `Empire TE1 `, serial tag `TE`, checksum seed. | The same, and the TCP session frame carries the protocol version: other versions are refused. |

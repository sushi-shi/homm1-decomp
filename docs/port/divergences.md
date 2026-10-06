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
| Long words in text fields | `font::LineLength` broke lines only at spaces; for a word wider than the field it started each next line where the last began and counted without end, so typing a long word into a multi-line field (the editor's map description) hung the program. It and `DrawBoundedString` also read and wrote the byte before the text for such a word on the first line. | `LineLength` breaks such a word where it overflows; `DrawBoundedString` draws as before without touching the byte before the text. Found by the survey's random input; test: `game_regressions_test`. |
| Uninitialised memory (valgrind) | The first fade-out faded the working palette before anything was set in it; the adventure manager compared its current terrain before setting it; a combat stack's luck and spell countdown were not set when it was created, and the combat drawing read the stack slots past each side's stacks. Each held whatever the memory held: zero in the original's freshly allocated objects in practice, a previous battle's values later. | They start at black, no terrain, no luck, no spell, and hidden. |
| Teleporting out of sight | The current hero's position is the view's centre (`DemobilizeCurrHero` takes it from there) and `MoveHero` moves the view with every step, shown or not, but `TeleportTo` moved the view only when the teleport was shown. A computer player's Dimension Door out of the watching player's sight was undone when the hero was put down (after the spell and the movement were spent), and from a boat the hero was put down onto the boat's or another hero's cell, which left a hero cell behind; its later owner of -1 was drawn with a flag icon read from before the player table (a crash in the survey). | The view always follows the teleport, so the hero arrives where the spell sends it; a hero without an owner is drawn without a flag. The survey checks after every computer turn that each hero cell names a hero standing there. |
| Editor: tool managers | The terrain, object, details and eraser tools were deleted through `baseManager`, which has no virtual destructor. | Each is deleted as its own type. No visible change. |
| Adventure commands without a hero | With no hero selected, `DoAdvCommand` used hero -1 (the bytes before the hero table): unused for the town view, but a move command left over from a hover while a hero was selected moved it; the search key (`ProcessSearch`) also searched with it. | Without a selected hero a left-over move command is dropped and the search key does nothing. Found by the survey's random input. |
| Map contents | The game trusted what a map's cells and records name: object types, pictures, town, mine and boat numbers, extra record numbers, the heroes and garrisons in them, ground tiles, the number of obelisks. An edited or damaged map indexed outside the tables and the map with them (town -1, hero 40, creature 200, owner -1 written into the player table, a ninth hero per player, more than 48 obelisks). | `game::MapDataValid` checks them when a game starts on a map (`NewMap`), and a map that breaks them is refused with the file error, like an unreadable one. Every shipped map and campaign map passes. Test: `game_regressions_test`. |
| Starting hero without a town | On a map that names its towns' owners, a player left without a town got its starting hero at the record before the town table. | No starting hero for such a player. |
| Ultimate artifact placement | With one human player who had no hero, the distance test read the hero before the hero table. | The distance test is skipped then. |
| Random artifacts beyond the supply | With more random artifact sites than artifacts the draw found none (-1, stored as 255) and the site wrote entry 255 of the 37-entry artifact table (into the boats). | Such a site is left empty. |
| Site masks | Gazebo sites are numbered from 1 into a 32-bit mask; with 32 or more the shift was 32 or more, which x86 masks. | The mask is written out. Same result. |
| Computer player: tavern heroes as defenders | Valuing a fight against a hero without an owner read the variable before the human player table. | No owner counts as a computer player. |

## Host differences (native port only)

| Area | Original | Now |
| --- | --- | --- |
| Settings | Registry key `HKLM\SOFTWARE\Buka\3DO\Heroes of Might and Magic Platinum\1.000`; the modem init string was stored with a length of 4 bytes. | `$XDG_CONFIG_HOME/homm1/heroes.cfg` (`%APPDATA%\homm1\heroes.cfg` on Windows, the site's IndexedDB in a browser), same value names; the init string is stored whole. The file is replaced atomically, like a save. |
| First start | Full screen. | In a window (F4 switches, as in the original). Later starts use the saved choice; `--window` and `--fullscreen` override it. |
| Music source | The CD's `TRACKS` folder on a CD drive (asking for the CD when absent). | The `Tracks` folder in `$HOMM1_CD`, a `cd` folder beside the game folder, or the game folder; without it, the installed digital music in `SOUND`. |
| Menu bar | A Windows menu bar in windowed mode (options, window sizes, help). | Drawn by the port in the same place with the game's small font and Windows' colours, for the game and the editor; an open menu runs a modal loop as Windows' did. Window-size commands size the window. |
| Help and About | WinHelp file `HELP\HEROES.HLP`, opened at its contents (`HELP_FINDER`); an About dialog box. | Help converts the book to one HTML page and shows it in the system's browser (a new tab in the browser build), opening at the contents and index; WinHelp's pop-up and secondary windows become in-page links, macros are not run. The item is greyed only when the file is missing. About shows the About box's text in a message box. |
| Network and serial play | NetBIOS, modem and direct cable. | Not available: choosing them reports the original's "NetBIOS not found" or serial error. |
| Single instance | A named event refused a second copy. | No check. |
| Game data in a browser | Installed from the CD. | The page copies the player's game folder (and, if given, the CD's music and the help file) into the site's IndexedDB once; saved games and maps are written back there and can be downloaded from the page. |
| Window in a browser | A window of the chosen size. | A canvas of the game's size (menu bar included) that the page scales to the browser window; the window-size commands change the canvas, full screen is the browser's. |
| Log on Windows | None. | The native Windows programs are GUI programs; their log goes to a redirected stderr, the parent console or `%APPDATA%\homm1\homm1.log`. |

## Retail defects kept

| Area | Behaviour |
| --- | --- |
| Remote message filter | `RemoteMain` clears 30 bytes of the 30-entry `i32` recent-message table, leaving most of it from a previous session. Only reachable in network play, which the port does not offer. |
| Drawing routines | The portable drawing routines reproduce the assembly they replace, including its edge cases: dimmed sprites step rows by a fixed 640 bytes, `TileToBitmap` accepts a tile index equal to the tile count, and a flipped clipped sprite placed wholly left of the target reads before its run. None is reached by the shipped data in the tested paths; the bit routines and loops with zero or negative counts, which in the assembly read past a byte or ran away, are bounded. |
| Network save compressor | `EncodeData` with an empty input wraps a 16-bit length and overruns its output. Only reachable in network play. |
| Tick count | The game compares its millisecond clock as signed 32-bit values; at 2^31 a delay's deadline lies "behind" the clock and `DelayTil` ends the program with an assertion. The original's clock counted from Windows start-up, so this happened after 24.8 days of uptime; the port's counts from 1,000,000 at start-up, so it takes 24.8 days of running the game. `HOMM1_TICK_START` reproduces it. |
| Repeated pad byte | Saved games store the second byte of a player's two-byte unknown field twice; kept for file compatibility. |

## Retail gameplay bugs (kept)

Found while hunting the defects above. They give wrong results without
memory errors, so the port keeps them, as the Visual C++ build does; each
needs a decision before it changes. Rows marked TE are fixed in the
Tournament Edition (`source-te`).

| Area | Bug | Reproduction |
| --- | --- | --- |
| Skeletons | A hero with all 14 artifact slots full is told he finds 1000 gold, but none is given (`DoEvent`, `SKELETON_ARTIFACT`). | Visit a skeleton holding an artifact with a hero carrying 14 artifacts; the treasury does not change. |
| Ghost retaliation (TE X10) | A retaliating ghost stack grows from its kills twice. | Attack ghosts that kill something in retaliation; compare their count with the kills. |
| Bad luck (TE X11) | Bad luck triggers on `SRandom(1,12) < -luck`, good luck on `<= luck`, so bad luck is rarer. | `army::CheckLuck`. |
| Auto-resolved losses (TE X12) | Losses roll 0..100 against the kill chance. | `armyGroup::DamageGroup`. |
| Duplicate stacks (TE X13) | `UpdateArmyGroup` writes every combat stack's survivors to the first slot of its creature type: an army with two stacks of one creature loses or duplicates troops after a battle. | Fight with two separate stacks of peasants, lose some of the second; the first stack takes the losses. |
| Wandering monster count (TE X14) | Survivors are summed into the 8-bit cell byte and wrap above 127/255. | Leave more than 127 monsters alive after a fight with a wandering stack. |
| Experience (TE X15) | Stacks that grew in combat (ghosts, resurrection) subtract experience. | `ExperienceValueOfStack`. |
| Map-placed heroes (TE X27) | Heroes placed on the map start with movement computed before their artifacts and owner are known. | A map hero carrying boots starts with normal movement on day 1. |
| Tavern (TE X30) | `PerWeek` does not reserve the heroes it offers, so one hero can be offered in two taverns (and bought twice). | Play several weeks with 3–4 players and compare the taverns. |
| Obelisks (TE X32) | Pieces per obelisk are 48 / count rounded down: visiting every obelisk need not complete the puzzle. | A map with 18 obelisks: 2 pieces each, 12 never revealed. |
| Ultimate artifact hint (TE X33) | The hint chance is a signed byte; above 127 it is negative. | `ComputeUALoc`. |
| Shipwreck (TE X34) | The shipwreck rewards from the cell the hero came from, not its own. | `DoEvent`, `MAP_OBJECT_SHIPWRECK`. |
| AI army value (TE X36) | Morale and luck scaling multiply integers that overflow for large armies. | `philAI::FightValueOfStack` with a very large army. |
| Elves' second shot (TE-FIX-3) | Elves fire their second shot without a shot left. | Elves with one arrow left. |
| AI auto-resolve (TE-FIX-4) | When the defender wins `QuickCombat` applies the win to the attacking hero and gives the defender the defender's own experience. | Two computer heroes fight; the defender wins. |
| AI boats, shipyard (TE-FIX-1, TE-FIX-2) | The computer buys a boat onto an occupied dock; the shipyard refuses boats when the hero shares the dock's row or column. | `philAI::CheckBuyStuff`, `townManager::Main`. |
| Recruit maximum (TE-FIX-7) | The affordable count is stored in 16 bits before it is limited by the creatures available; a large treasury wraps it. | More than 32767 affordable creatures. |
| Town footprint (TE-MAP-1) | Only the town's entrance cell names the town. | `SetupTown`. |
| AI spell evaluation | Bless and Curse are valued on stacks whose damage is a single value (TE-UNR-1). | `DetermineEffectOfSpell`. |
| AI creature purchase | The value of the stack a purchase would replace uses `gMonsterDatabase[slot]`, the slot number as a creature. | `philAI::EvaluateOneTimeCreaturePurchase`. |
| AI resources | `MaxBuyableCreatures` returns only the last resource's (gold's) count. | `philAI::MaxBuyableCreatures`. |
| AI sites | `ValueOfEventAtPosition` tests `gMapVisitFlags[x][y] && gCurPlayerBit` (`&&` for `&`): any player's visit counts. | |
| AI routes | `BuildPath`'s check that a node belongs to its cell uses `&&` where `||` is meant, so stale routes pass (also drawn as bogus routes). | |
| AI relocation | `ResetHeroRVs` uses `m_x` for the y distance. | |
| AI artifacts | The computer ignores `GiveArtifact`'s "no free slot" and loses the artifact with 14 slots full. | |
| Berserk | `GoBerserk`'s flying branch can loop without end; its walking branch can leave no action set. | |
| Combat stats | `army::Init` adds the hero's Attack and Defense into 8-bit fields, which wrap above about 100. | A hero with Attack 120. |
| Summon Boat on the edge | Next to the map's edge the boat is summoned beside (0,0) with a coordinate of -1 or 72. | Cast Summon Boat with the hero on the edge of SEL21234. |
| Campfire | Clears the ambient sound at the view's centre instead of the campfire's cell. | |
| Puzzle | Off-map cells of the puzzle show cell (0,0). | |
| Campaign lord scenarios | The lord scenarios give player 0 a random starting hero before the map's own heroes are placed; when the draw picks a hero the map also places, the one hero is given to both players (the survey's hero cell check reports it on day 1 in some runs of side 1 scenario 4 and its counterparts). | `homm1_survey campaign 1 4 10 14` |
| Stray hero and town cells | Some shipped maps (DNL3, AES3, PNM3, UHS6) have hero or town triggers with no record; clicking them shows hero or town 0, and selects it if owned. | |
| Campaign crests | The crest table should be read at `[i-1]`, so the enemy lords' crests are random. | |
| Thieves' Guild | Resources are grouped wrongly in the Thieves' Guild view (`TOWNMGR`). | |
| Recruiting a hero | `RecruitHero` sets the owner of both tavern heroes. | |
| Weekly monster growth | `m_objectMetadata +=` can wrap the 8-bit count. | |
| Editor generator | A broken bubble sort ranks the regions; `minX--` for `minY--`; `direction % 1`; `castleRegion[c-1]`; road destinations take `.x` for `y`; the road trace stops on its destination; a missing `else` places stone liths twice on desert. | Random maps (MAPOBJ.cpp). |
| Editor | A `u8 != -1` test is always true (EDITMGR.cpp, map writer); the vertical scroll knob clamps x; `PaintGround` redraws only its first cell; extra records are not freed on erase or undo. | |

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
| TE-FIX-1, TE-FIX-2, TE-FIX-3, TE-FIX-4, TE-FIX-7 | gameplay | No (listed above). |
| TE-MAP-1 | gameplay | No. |
| TE-UNR-1 | AI behaviour | No. |
| TE-UI-1 spell book forward path | interface | No. |
| TE-QOL-6 catapult residue | display (no memory error) | No. |
| TE-RU-1 Russian end sequence | none (same behaviour) | No. |

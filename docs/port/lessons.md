# Porting lessons: the bug classes and their guards

A reconstructed Windows game of this period carries assumptions that held on
one compiler, one pointer width, one byte order and one operating system. A
port that only makes the code compile tends to keep them, and they come back
as crashes, corrupted saves or wrong pictures long after the first screen
works. This file lists the classes of defect such a port runs into, how each
one shows, and what in this port prevents it. Each guard names the code that
implements it, so that a later change can tell when it removes one.

The classes are ordered roughly from "silent data corruption" to "visible
glitch".

## 1. File records that depend on in-memory layout

**Mechanism.** The original wrote and read its files by handing whole
structures, or arrays of them, to `write` and `read`: heroes, towns, mines,
boats, map cells, high scores, the resource archive's directory, the map
header. The file format is therefore whatever the compiler's layout was:
`#pragma pack(1)`, 4-byte `long`, 4-byte pointers, little-endian integers. A
port that changes any of these (natural alignment for speed, a 64-bit target,
a big-endian target, a field widened to hold UTF-8) silently writes a
different format and reads garbage from the shipped files, often without any
crash.

**Guard.**

- Every file record is encoded field by field, little-endian, at its declared
  width: `RecordWriter`/`RecordReader` (`include/PLATFORM/Records.h`) and the
  per-record codecs (`include/SOURCE/saveRecords.h`, `src/SOURCE/SAVEREC.cpp`).
  Saved games, maps, campaign maps, high scores, map headers and the archive
  directory go through them; nothing in the game writes a structure with
  `sizeof`.
- The record sizes are named constants (`HERO_RECORD_SIZE` = 182, ...). The
  structures the game still reads straight out of file data in memory (icon
  frame tables, map extra blocks) and the ones that are file images keep
  their packed layout and carry `H1_STATIC_ASSERT(sizeof(...) == ...)` in
  `SAVEREC.cpp`, which also compiles under Visual C++ 6.
- `tests/port/records_test.cpp` checks that each codec writes exactly its
  record size, decodes back to the same bytes and refuses one byte less.
  With the game data it parses **every shipped map, campaign map, saved game,
  high score table and the archive directory** to exactly their file length
  and re-encodes each byte for byte.

## 2. Packed runtime structures

**Mechanism.** The reconstruction marks most classes `#pragma pack(1)`
because that was the original layout. For runtime-only classes the packing
buys nothing but misaligned members: a reference or pointer to a misaligned
`i32` or `float` (passed to a function, or a nested struct used through its
own type) is undefined behaviour. x86 forgives it until the optimizer emits an
aligned vector load; other CPUs fault.

**Guard.** Runtime classes (managers, widgets, `game`, `hero`, `town`,
`playerData`, `army`, `tag_message`, ...) are naturally aligned. Packing
remains only on file and wire images (`IconEntry`, `aggEntry`, map extra
records, high score entries, `mapCell`, network packets) and on the four
resource classes whose layout the Windows build's assembly routines address
by offset (`resource`, `bitmap`, `icon`, `tileset`). The codecs never bind a
reference to a member of a packed structure: multi-byte fields are read by
value (`RecordReader::GetI32()`), arrays element by element. The sanitizer
build checks alignment (section 12).

## 3. Pointer width and `long` width

**Mechanism.** On a 64-bit host a pointer no longer fits in an `i32`, and on
LP64 hosts `long` is 8 bytes. Code that casts pointers to integers
(`H1_ASSERT(reinterpret_cast<i32>(ptr))`, `(u32)key + i`), stores pointers in
integer slots, or uses `long`/`unsigned long` in a structure shared with a
file or a library, changes size or truncates addresses.

**Guard.**

- `include/H1/Ints.h` defines `i8`...`u64` from `<stdint.h>` everywhere but
  Visual C++ 6, so the fixed-width names mean what they say.
- The game has no `long` left in shared structures: the Smacker `Smack`
  structure, which the original declared with `unsigned long`, uses `u32`.
- Pointer-to-integer casts are compile errors natively
  (`-Werror=int-to-pointer-cast`, and C++ rejects narrowing pointer casts);
  the three that existed were rewritten as pointer comparisons or pointer
  arithmetic.
- Message payloads (`tag_message`) keep a pointer and integers in one union;
  every reader of the pointer arm is paired with a writer of the same arm.

## 4. Byte order and unaligned reads of file data

**Mechanism.** Reading a little-endian 16-bit value with
`*(i16*)(buffer + offset)` is wrong on a big-endian host and misaligned on any
host when the offset is odd.

**Guard.** Archive words and longs (`resourceManager::ReadWord`, `ReadLong`)
and all records are decoded byte by byte. The build refuses big-endian targets
outright (`CMakeLists.txt`), since the in-memory resource formats (icons,
tiles, fonts) are still read in place.

## 5. Untrusted lengths and counts in data files

**Mechanism.** Counts and sizes inside the files drive allocations and copies:
the archive's entry count and offsets, the number and length of a map's extra
records, the map header's text fields. The original trusted them, and also
ignored short reads, so a truncated or crafted file reads past buffers.

**Guard.**

- The archive directory must fit in the file and every entry must lie inside
  it, or the archive is refused (`resourceManager::LoadAggregateHeader`). A
  resource id that is not found is reported before the directory is indexed
  past its end (`PointToFile`, `GetFileSize`).
- A map's extra-record count must be within the record table and each
  record's length within the file; each block is allocated at least as large
  as the largest record type, so reading a short block through its structure
  stays inside the allocation (`game::LoadMap`).
- `RecordReader` fails on any read past the end and returns zeros instead of
  stale memory; loaders report a short file as a file error instead of
  continuing with partial state.
- Fixed-width text from files is copied with `CopyTextField`, which stops at
  the field width and always terminates; record codecs terminate name fields.
- The help book is read by the port's own converter (`src/PLATFORM/Help.cpp`)
  and is equally untrusted: every read is bounds-checked, a reader that failed
  stays failed (one that silently returned zeros after an error produced
  plausible garbage), LZ77 and RLE output is capped, B+ tree walks refuse
  cycles. `help_test` damages a synthetic book thousands of ways under the
  sanitizers.

## 6. Retail out-of-bounds and overlap that "worked"

**Mechanism.** Some original code reads one element past an array, copies a
string onto itself, or reads the table after the one it means, and works only
because of how the original linker laid out memory. A different compiler,
layout or allocator turns it into a crash or a different result; sanitizers
report it at once.

The scenario editor has its own: its terrain blending tested the map edges
against the grid size instead of the last index and read cells beyond the
grid (fixed where it happens, listed in the divergences).

The assembly routines replaced by C++ have their own: the bit routines
touch a 32-bit word around the addressed byte, zero or negative counts make
loops run for billions of iterations, and the decoder reads past the end of
its input and before its tree table. The C++ replacements (`src/PORT/BASE`)
were checked against the original assembly, assembled and linked into one
32-bit test program, on about 130,000 generated inputs; they keep every
result on valid input and bound the out-of-range accesses (the remaining
edge cases are listed in [divergences.md](divergences.md)).
`tests/port/blit_test.cpp` draws into exactly-sized buffers under the
sanitizers.

**Guard.** The AddressSanitizer/UBSan build (section 12) runs the start-up,
new game and load paths, and the survey (`tools/port/survey.py`, README)
plays every shipped map, the campaign openings, random battles and random
editor maps with computer players, and both programs under random input.
Most of what it found was one class: an index one step outside a grid or a
table at its edge, or a -1 meaning "none" used as an index (map cells beyond
the edge in the path search, mine flags and object shadows above the top
row, hex -1 beside the battlefield, tavern slots and hero owners of -1, a
63-entry name table read up to 127). Each is fixed where it happens; where
the original's result was fixed by the linker's layout (the quick info texts
read from the next tables, the masked shift of a 32-bit site mask) the same
result is produced without leaving the table, otherwise the evident intent
is kept, and each is listed in [divergences.md](divergences.md). Map
contents are checked once when a game starts on a map (`game::MapDataValid`)
instead of at each use.

## 7. Compiler semantics the code relies on

**Mechanism.** The original compiler made plain `char` signed, wrapped signed
overflow and did no type-based alias analysis. Game code compares `char`
values against negative sentinels, lets random-number and checksum arithmetic
overflow, and reinterprets buffers. On ARM `char` is unsigned by default;
modern optimizers assume signed overflow and aliasing never happen.

**Guard.** Every native target compiles with `-fsigned-char -fwrapv
-fno-strict-aliasing` (`HOMM1_SEMANTICS` in `CMakeLists.txt`): the port keeps
the semantics the game was written against instead of auditing every
expression.

## 8. Case, separators and roots of file names

**Mechanism.** The game names files like `.\DATA\heroes.agg` and
`MAPS\CAMP1.CMP`; installed copies spell them `Data/HEROES.AGG` or any other
case. Backslashes and case-sensitive lookups fail on POSIX hosts. Naive
fixes resolve `..` out of the game folder or pick an arbitrary file among
several case variants.

**Guard.** One resolver (`src/PLATFORM/File.cpp`): game paths are split on
either separator, `.` is dropped, `..` and drive or absolute game paths are
refused, and each component is matched exactly first, then case-insensitively
in sorted order, so the choice is deterministic. Directory listings are
sorted. `tests/port/file_test.cpp` covers case folding, containment, missing
files, creation of new folders on write, wildcards and ordering.

On Windows the file system already ignores case, but the port resolves the
same way there, so `..` and drive-relative game paths (`C:FILE`) stay
refused, listings stay sorted and resolved names carry the spelling on disk.
Host paths (the game folder, the settings folder) are UTF-8 inside the port
and go through the wide-character API: the ANSI API mangles any folder
name outside the system code page.

## 9. Interrupted and non-atomic saves

**Mechanism.** The original truncated the save file and then wrote it; a
crash or full disk in between destroys the previous save.

**Guard.** `FileReplace` writes the whole file to a sibling `.partial` file,
flushes it with `fsync`, then renames it over the old one; on failure the old
file is untouched. Saved games, high scores and settings use it. A save that
fails (a full disk, a read-only folder) is reported and play goes on; the
original ended the program when the file could not be created
(`save_diskfull` tests it with `/dev/full`).

On Windows, `rename` refuses to replace an existing file, so the replace is
`FlushFileBuffers` and `MoveFileExW(MOVEFILE_REPLACE_EXISTING |
MOVEFILE_WRITE_THROUGH)`. The settings were first written with `std::rename`
over the old file, which on Windows silently stopped updating them after
the first start; they now go through `FileReplace` too. In a browser the file
system is in memory: a replaced file reaches IndexedDB when it is closed
(IDBFS `autoPersist`), and only a completely written file is ever renamed
into place there too.

## 10. Format strings and fixed buffers

**Mechanism.** `sprintf(buffer, text)` treats `text` as a format; when the
text is a player-entered save name, a `%` in it reads arbitrary stack.
Fixed-size buffers sized for the original text overflow when text gets longer.

**Guard.** The two places where user text was a format string
(`game::SaveGame`, `game::LoadGame`) copy it instead. The remaining
`sprintf(buffer, gameText)` calls take the game's own catalog text, which has
no `%`; the build reports them (`-Wformat-security`) so they stay visible. The
map requester copies map names and descriptions bounded to their fields.

## 11. Time

**Mechanism.** The game compares millisecond tick counts as signed 32-bit
values (`deadline > KBTickCount()`) and asserts that deadlines exceed 10000,
assuming a Windows tick count that starts at boot. A clock that starts at 0
trips the assertion; one that wraps breaks every comparison.

**Guard.** `platform::Ticks()` counts from 1,000,000 at start-up, which
satisfies the assertion and keeps the signed comparisons valid for 24 days of
continuous play. After that the game ends with the assertion in `DelayTil`
(`HOMM1_TICK_START` shows it); the original did the same after 24.8 days of
Windows uptime.

## 12. Sanitizers, warnings and tests in the build

- `-DHOMM1_SANITIZERS=ON` builds everything with AddressSanitizer and
  UndefinedBehaviorSanitizer, undefined behaviour fatal;
  `-DHOMM1_SANITIZERS_RECOVER=ON` keeps running to survey all findings.
- Port and platform code compile with `-Wall -Wextra -Wpedantic -Wconversion
  -Wsign-conversion -Wshadow -Wold-style-cast -Werror`; game code with the
  defect-class errors (`return-type`, `int-to-pointer-cast`,
  `mismatched-new-delete`) and visible format-security warnings.
- `ctest` runs `records_test`, `file_test`, `blit_test` (the drawing
  routines in exactly-sized buffers) and the LZHUF round trip. With
  `HOMM1_DATA` set it also checks every shipped data file, and
  `save_roundtrip` loads the shipped saved game in the real program under
  Xvfb and saves it again: the new file must equal the original byte for byte
  outside its name field, which also proves that no uninitialized memory
  reaches a save.
- `nix flake check` builds the native program and the sanitizer build and
  runs their tests.
- Scripted input (`HOMM1_INPUT_REPLAY`) drives the real binary headless, so a
  sanitizer build can be run through menus, a new game and a loaded save
  under Xvfb (`docs/port/README.md`).

## 13. Replaced host subsystems

**Mechanism.** Replacing the window system, sound library and movie player is
where a port loses behaviour without noticing: a stubbed cursor call leaves the
desktop arrow instead of the game's cursors, a music backend inherits a
"which tracks exist" table from the old one, a movie stub returns nothing and
the caller dereferences it, input arrives with logical instead of physical key
codes.

**Guard.** Each replaced subsystem keeps the original's contract, written
down where it is implemented:

- **Cursors** (`src/PORT/SOURCE/wingraph.cpp`, `src/PLATFORM/SDL3/Video.cpp`):
  the game still builds both the colour and the monochrome cursors from its
  own art, with the Windows AND/XOR mask rules; the display draws them over
  the picture with the current palette, so they fade and cycle as they did
  on the 8-bit display.
- **Keys** (`src/PLATFORM/SDL3/Input.cpp`): physical keys become PC Set 1
  scan codes exactly as the Windows message carried them; arrows and the
  keypad share codes whatever the Num Lock state, so keypad movement and
  arrow keys reach the game's own scan-code table, and the game applies its
  own keyboard layout table (the language's `keyboard` descriptor).
- **Presentation** (`src/PLATFORM/SDL3/Video.cpp`): the display keeps an
  8-bit image and a palette like DirectDraw's primary surface; palette changes
  show without a copy; many small copies are coalesced into one frame; the
  adventure map's scrolling copy takes its source from the scrolled position
  as the original paint did.
- **Music** (`src/PORT/BASE/Audio.cpp`): the original host's track policy (CD
  track map, which tracks repeat, where a track resumes) is kept; only the
  decoder changes.
- **Sound effects** (`src/PLATFORM/SDL3/Audio.cpp`): a looping voice keeps its
  own copy of the sample, because the audio thread refills it while the game
  may free the sample (`StopSample` does nothing while samples are suspended,
  as during a computer player's turn).
- **Movies** (`src/PORT/SOURCE/Smacker.cpp`): the Smacker calls the game makes
  are implemented, including the decode-ahead palette and the frame counter
  that wraps to 0 at the end, which is how the game detects a movie's end. A
  movie that cannot be opened returns no handle, which the game already
  handles.
- **Menus** (`src/PORT/SOURCE/Menu.cpp`): built from the same `.rc` scripts
  the Windows build compiles, so the items, their identifiers and their text
  cannot drift; the game's own calls keep check marks and greyed items; and
  an open menu runs a modal loop, because the game polls the pointer and
  would otherwise scroll the map under an open menu, which Windows' modal
  menu loop prevented.
- **Networking** (`src/PORT/SOURCE/netwin.cpp`, `comwin.cpp`): reports itself
  unavailable through the original's own error path.

## 14. One program, two builds, two programs

**Mechanism.** The Windows build lists each program's units in its manifest;
a second, hand-written list for the native build drifts as soon as the
regenerated source adds or moves a unit, and the two programs (game and
editor) share units compiled differently (`HOMM1_EDITOR`). Code moved out of
a Windows unit into a shared one can break the other program's link.

**Guard.** `tools/port/units.py` derives each native program's units from
`build.json` with fixed substitutions for the Windows-bound units;
`build.py --target all` and both native programs are built for every change,
and the editor's map writer is checked against every shipped map
(`editor_maps_test`), whose header lists what the writer legitimately
derives instead of copying.

## 15. Hosts that do not let a program block

**Mechanism.** The game is written as a set of blocking loops: each screen
polls for input until it closes, and waits by spinning on the tick count. A
browser page that never returns to the browser shows nothing and receives no
input. Rewriting every loop as a callback is a rewrite of the game.

**Guard.** The browser build compiles with ASYNCIFY, which turns any call
into a point where the program can return to the browser and later resume.
The return points are where the program already waits: SDL presenting a
frame and `SDL_Delay`, plus the event poll, which yields at least once a
frame even in a loop that neither draws nor sleeps
(`platform::sdl::YieldToBrowser`). The game code is untouched; the cost is a
larger WebAssembly file. JSPI is the lighter successor once every browser
has it. Anything else that waits on the browser (message boxes, the audio
unlock, opening a tab) must happen inside these returns or inside the
user's click: the audio context is created in the click on **Play**, a
context created later stays suspended in some browsers.

## 16. Toolchains that accept wrong input silently

**Mechanism.** Cross builds meet tools that do not fail on mistakes: FFmpeg's
`configure` only warns about a component name it does not know (the
Smacker video decoder is `smacker`, not its runtime name `smackvid`) and
builds without it, after which every movie "cannot be opened"; SDL marks its
headers as non-system, so strict warnings apply to its macros wherever the
compiler is not Nix's native wrapper; MinGW's `printf` is Microsoft's unless
asked otherwise, so `%zu` prints garbage.

**Guard.** `nix/ffmpeg-minimal.nix` fails when any component the port needs
is missing from `config_components.h`; `CMakeLists.txt` marks
`SDL3::Headers` as system headers; the Windows build defines
`__USE_MINGW_ANSI_STDIO` and checks `Log`'s format as `gnu_printf`; the
Windows install fails when a program imports a DLL that is neither shipped
nor part of Windows. Headless test runs of the help must also keep the
desktop out of reach: `SDL_OpenURL` tries the D-Bus portal before `xdg-open`
and opens the user's real browser, so such runs set
`DBUS_SESSION_BUS_ADDRESS` to nothing and put a recording `xdg-open` first
on the `PATH`.

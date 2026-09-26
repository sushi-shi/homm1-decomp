# HoMM1 resource-manager layout

The HoMM1 retail constructors and the surrounding `RESMGR` methods establish
the packed class layout used by `SavePosition` and `RestorePosition`.

`baseManager::baseManager` at VA `0x00473D90` initializes a vptr followed by
two manager links, two 16-bit manager values, a 30-byte name, and a final
16-bit active value. The last store is at offset `0x2E`, proving a base size of
`0x30` bytes.

The `resourceManager` constructor at VA `0x00475830` then initializes:

| Offset | Field |
| ---: | :--- |
| `0x30` | loaded-resource list head |
| `0x34` | aggregate file descriptor |
| `0x38` | aggregate directory pointer |
| `0x3C` | 16-bit directory-entry count |
| `0x3E` | expunging flag |
| `0x42` | saved file position |
| `0x46` | 60-byte last-file name |
| `0x82` | last file id |

The final field ends at `0x86`, which fixes the derived-class size. Accesses in
`AddResource`, `Query`, and `PointToFile` independently confirm the fields at
offsets `0x30` through `0x3E`.

The linked `resource` nodes are packed records. `Dispose` and `GetSample`
access the reference count at `0x06`; `Query` sign-extends the resource ID at
`0x08`; and `AddResource` and `Query` access the next pointer at `0x0A`.
Together with the vptr and category at the start, these accesses establish a
record size of `0x0E` and a 16-bit ID parameter for HoMM1's `Query`.

`Expunge` at VA `0x00475ED0` follows the donor list walk exactly. Its two
traversal pointers occupy one two-element cursor array: element zero holds the
next node at `[ebp-8]`, and element one holds the current node at `[ebp-4]`.
That layout accounts for every retail stack access while retaining the donor's
remove-then-delete order.

HoMM1 differs from both HoMM2 donors here. Buka 2.1 and PoL 2.0 save positions
in global stacks and select among aggregate descriptors. HoMM1 saves one
position in the object. Its functions at VAs `0x00476470` and `0x004764A0`
call the CRT routines at VAs `0x0048B050` and `0x00482500`, respectively.
The donor identities and the VC4 CRT implementations identify those targets
as `_tell` and `_lseek`; their call arguments also match the SDK declarations.
The function bodies end at their `ret` instructions after `0x2B` and `0x2E`
bytes. The following `INT3` bytes align the next linker partitions and are not
part of either source function.

`GetBackdrop` at VA `0x004758D0` is the raw branch of the later Buka donor:
HoMM1 has no `useIcon` argument. After reading the bitmap header and pixels it
calls the one-byte cdecl stub at VA `0x004738C0` with the pixel pointer, width,
and height. The same target follows pixel reads in the bitmap constructor at
VA `0x0047A770` and the three-dimensional image constructor at VA
`0x0047F970`, where the last argument is the product of two dimensions. The
provisional semantic identity `PostprocessBitmap` records that strong calling
evidence without claiming an original-source spelling that neither donor
preserves. Its empty retail body explains why HoMM2 could remove the call.

`GetBackdropAtLoc` at VA `0x00475960` is Buka's raw row-copy loop without its
`useIcon` branch. Its `ret 16` proves four arguments. The retail stack slots
and VC4 output establish nested local lifetimes: the row index is outermost,
then image height, then width. Keeping those scopes emits the exact offsets
without synthetic padding.

`GetSample` at VA `0x00475D40` retains the Buka cache lookup and reference
count increment, but constructs HoMM1's earlier sample type with arguments
`(name, 0, 127, 1)`. The allocation operand proves a `0x2E`-byte object. The
constructor at VA `0x0047FA60` accesses eight 32-bit playback fields after the
`0x0E`-byte `resource` base, independently proving that size and inheritance.
The allocation target at VA `0x004806E0` is the VC4 `operator new(unsigned
int)` wrapper: it forwards the requested size and allocation flag `1` to the
adjacent runtime allocator. `GetSample` itself ends after `0x9C` bytes; its
four following `INT3` alignment bytes are excluded from the source claim.

`ReadWord` at VA `0x00476530` refers to the packed assertion record at VAs
`0x004A0E0C` and `0x004A0E10`. The record contains a 16-bit value `619`
followed by `D:\\Heroes\\Base\\RESMGR.CPP`; the function increments the value
before passing line `620` to `ProcessAssert`. Its final call target at VA
`0x00482990` is `_read`, as shown independently by the donor CRT identity and
the matching three-argument file-read body.

The adjacent read helpers repeat the same record shape. `ReadByte` uses line
599 at VAs `0x004A0DEC`/`0x004A0DF0`, `ReadLong` uses line 640 at
`0x004A0E2C`/`0x004A0E30`, and `ReadBlock` uses line 680 at
`0x004A0E4C`/`0x004A0E50`. `ReadBlock` calls the C-linkage `_PollSound` body at
VA `0x0044F640` before and after `_read`, matching the Buka 2.1 donor design.

`Open` at VA `0x00475FD0` loads the pointer at VA `0x004C79DC` and passes it
to `LoadAggregateHeader`. The other reference to that pointer, in the command
line setup function at VA `0x00450FBC`, assigns it the address of the buffer
filled from `".\\DATA\\"` and `"heroes.agg"`. This is the HoMM1 counterpart
of HoMM2's `DEFAULT_AGGREGATE_NAME`; unlike the HoMM2 donor, HoMM1 loads only
that one aggregate. Both `Open` and `LoadAggregateHeader` return their status
through `AX`, establishing their 16-bit return types.

`LoadAggregateHeader` at VA `0x00476180` reads the entry count directly into
the 16-bit field at object offset `0x3C`. Its multiply-by-14 sequence and the
later field accesses in `PointToFile` and `GetFileSize` establish a packed
14-byte `aggEntry`. The failure branch formats `"Can't open file: %s"` into
the shared `gText` buffer at VA `0x004C6750` before calling `ShutDown`.
The called CRT bodies are `_sprintf` at VA `0x004806F0`, `_free` at
`0x00480880`, `_malloc` at `0x004809F0`, `_close` at `0x00482890`, `_read` at
`0x00482990`, and `_open` at `0x00482C40`. The first three are exact VC4
library members in the executable census; the remaining identities also agree
with the donor source and their argument/result use in this body.

`PointToFile` at VA `0x00476280` uses a signed 16-bit resource ID and walks
the one packed directory until the count is exhausted or the ID matches. Its
entry stride is 14 bytes, independently confirming the `aggEntry` layout. A
null directory reports `"File Error: .AGG File not valid"`; a missing ID
formats the longer diagnostic beginning `"ResMgr::PointToFile failure!"`.
Both string starts, at VAs `0x004A0D24` and `0x004A0D44`, are direct relocated
operands in the retail body. The final `_lseek` uses the matched entry's
32-bit offset at `entry + 2` and the single descriptor at object offset
`0x34`. The source function ends after `0xF2` bytes at its `ret 4`; the
following 14 `INT3` bytes are linker alignment before `GetFileSize` and are
excluded from the function claim.

The previously unnamed cache getter family at RVAs `0x759F0`, `0x75A90`,
`0x75C00`, and `0x75CA0` follows Buka 2.1's `GetPalette`, `GetBitmap`,
`GetTileset`, and `GetFont` order. Each calls `MakeId` and `Query`, increments
the cached resource's 16-bit reference count at offset `+6`, or allocates and
constructs a derived resource before `AddResource`. The allocation sizes are
`0x12`, `0x18`, `0x18`, and `0x16` respectively. Their constructor targets at
RVAs `0x7CF70`, `0x7A770`, `0x7F970`, and `0x7B2C0` each call the known
`resource::resource` body with category `2`, `0`, `3`, and `5` respectively,
then write a distinct vtable. The getter's argument is a 16-bit ID and the
constructor reads only its low word; the source uses `short` for the VC4
constructor signature. All four constructor bodies now have source claims.

The retail constructor stores establish the cache classes' inherited
`resource` prefix: palette's data pointer is at `+0x0E`; tileset's count,
width, height and data pointer are at `+0x0E`, `+0x10`, `+0x12`, and `+0x14`;
font's two 16-bit properties and icon pointer are at `+0x0E`, `+0x10`, and
`+0x12`. Together with the allocation sizes, these determine the packed
class layouts used by the getters. The bitmap already has its resource base
and 0x18-byte layout in its owner header.

The name overload of `GetIcon` at RVA `0x75B30` calls `MakeId` and forwards
its signed short result to the cache overload at `0x75B70`. The latter's
`Query`/reference-count/new-icon path matches the adjacent donor family.
The icon allocation is `0x1C` bytes. Its retail drawing bodies read and write
four signed 16-bit bounds at offsets `+0x14`, `+0x16`, `+0x18`, and `+0x1A`,
after the frame count at `+0x0E` and data pointer at `+0x10`. That completes
the class layout needed by the exact cache body and constructor.

Retail's bitmap ID constructor at RVA `0x7A770` has the Buka load sequence
plus `PollSound` before and after data read and `PostprocessBitmap` between
them. The palette constructor at `0x7CF70` allocates 768 bytes and reads that
many palette bytes. The tileset constructor at `0x7F970` zero-extends its
three 16-bit dimensions, reads their product, and passes the decoded tile
height and width to `PostprocessBitmap`.

The font constructor at RVA `0x7B2C0` stores two 16-bit header words, reads a
13-byte icon name, and brackets `GetIcon(name)` with 1/0 writes to the global
at RVA `0x92E00`. The same global is bracketed around icon loads by
`combatManager::DoLoseWindow` and `advManager::Open`, matching Buka 2.1's
`gbLoadingMonoIcon` identity. The icon constructor at `0x79B20` reads its
frame count and data length, allocates and reads the data, then calls RVA
`0x738D0`. That retail callee is a `ret` and sits after the analogous bitmap
hook at `0x738C0`; `PostprocessIcon` is a provisional name for this reviewed
call identity, with no claimed source body.

The previously unnamed `resourceManager::Close` body at RVA `0x760F0` is the
single-aggregate form of Buka 2.1's `Close`: it checks active state at `+0x2E`,
calls `Expunge`, clears the list head at `+0x30`, frees the directory at
`+0x38`, closes the file descriptor at `+0x34`, and clears active state.
The previously unnamed `GetFileSize` body at RVA `0x76380` repeats the
single-aggregate directory search of `PointToFile`, returns zero if the
directory is absent, reports a missing ID using its distinct retail string,
and returns the matched entry's 32-bit size at offset `+6`.

The retail vtables at RVAs `0x8C5FC`, `0x8C624`, `0x8C634`, and `0x8C684`
point to the icon, font, palette, and tileset virtual deleting destructors at
`0x79B90`, `0x7B370`, `0x7CF30`, and `0x7FA20`. The first, third, and fourth
inline a free of their owned data pointer followed by `resource::~resource`;
their source destructor is correspondingly `inline`, as in the Buka 2.1
headers. The font deleting destructor calls its separate ordinary destructor
at `0x7B3B0`, which disposes the glyph icon through `resourceManager::Dispose`
before calling the resource base destructor. Each deleting destructor tests
its flag and conditionally calls RVA `0x805E0`. The callee is the VC4 operator
delete wrapper: its body passes its pointer to the reviewed `_free` at
`0x80880`. The three newly split census boundaries are function entries,
not padding; the vtable pointers and complete prologue-to-`ret 4` disassembly
support them. These destructor claims cover only code; they do not assert the
vtable initializer bytes.

The bitmap vtable at RVA `0x8C600` points to its deleting destructor at
`0x7A6E0`, an entry embedded in the previous coarse census span. Retail
first sets that vtable, frees the non-null pixel pointer, clears the pointer,
calls `resource::~resource`, and conditionally calls operator delete. The
ordinary destructor already reconstructs those operations; making it inline
exposes them to VC4's deleting-destructor generator and matches the retail
code. The preceding default constructor at `0x7A6B0` zeroes its dimensions
and pixel pointer. Palette's default constructor at `0x7CF00` allocates the
same 768-byte buffer as the ID constructor after passing ID `-1` and an
initial reference to `resource::resource`. Its `Data` accessor at `0x7CFD0`
returns the data pointer at `+0x0E`. These four source bodies match exactly.

## Constructor source boundaries

The base constructor returns at VA 0x00473DD9, giving a 0x4A-byte body;
the following six INT3 bytes align the next retail function. Buka BASEMGR
supplies the corresponding initializer list and priority/mask/active/name
statements. The retail packed base fields and existing cpp_o2 profile
reproduce all constructor instructions and ordered code referents.

The resource constructor returns at VA 0x004758CA, giving a 0x9B-byte body;
the following five INT3 bytes are padding. InitMainClasses allocates 0x86
bytes at VA 0x004F806 and calls this constructor at VA 0x004F82A. The
constructor calls the base constructor at VA 0x0047583F and initializes the
single aggregate fields in retail order before copying the empty last-file
name and clearing the last-file ID. The saved position at offset 0x42 is
written by SavePosition, rather than by this constructor. The existing
cpp_carcass_oi profile reproduces this constructor exactly. Code matching
does not admit initializer/data-byte coverage for either name string.

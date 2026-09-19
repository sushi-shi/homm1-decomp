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

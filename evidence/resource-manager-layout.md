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

HoMM1 differs from both HoMM2 donors here. Buka 2.1 and PoL 2.0 save positions
in global stacks and select among aggregate descriptors. HoMM1 saves one
position in the object. Its functions at VAs `0x00476470` and `0x004764A0`
call the CRT routines at VAs `0x0048B050` and `0x00482500`, respectively.
The donor identities and the VC4 CRT implementations identify those targets
as `_tell` and `_lseek`; their call arguments also match the SDK declarations.
The function bodies end at their `ret` instructions after `0x2B` and `0x2E`
bytes. The following `INT3` bytes align the next linker partitions and are not
part of either source function.

`ReadWord` at VA `0x00476530` refers to the packed assertion record at VAs
`0x004A0E0C` and `0x004A0E10`. The record contains a 16-bit value `619`
followed by `D:\\Heroes\\Base\\RESMGR.CPP`; the function increments the value
before passing line `620` to `ProcessAssert`. Its final call target at VA
`0x00482990` is `_read`, as shown independently by the donor CRT identity and
the matching three-argument file-read body.

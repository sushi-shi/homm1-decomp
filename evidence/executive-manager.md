# HoMM1 executive manager list and call loop

The packed `executive` object is 16 bytes: list head, list tail, active
manager, and result are 32-bit fields at offsets `0`, `4`, `8`, and `12`.
The default constructor at RVA `0x7A170` clears all four and matches VC4
exactly. Buka 2.1's `BASE/EXEC.cpp` has the corresponding class and method
family, but HoMM1's `AddManager` has a 16-bit result and priority argument,
as shown by the retail `AX` return tests, `ret 8`, and word accesses to
manager priority. Its body at `0x7A3D0` returns 3 for null/open failure,
inserts by priority into the doubly linked manager list, and returns zero
on success. That source matches exactly.

`RemoveManager` at `0x7A4B0` calls the manager's virtual `Close`, splices the
head or an interior node from the list, clears its two links, and returns.
`CallManager` at `0x7A530` saves the active manager, removes it, adds the
requested manager at default priority `-1`, runs the main loop, removes the
requested manager, restores the saved manager, and writes the active pointer.
Both failure branches pass `"Can't add manager!"` to `ShutDown`, but their
relocated operands point to distinct 20-byte slots at RVAs `0xA1A00` and
`0xA1A14`. These slots are recorded as code-required identities without
initializer claims; distinct extern declarations preserve their separate
retail referents. The former's source is near exact; the latter's body and call
set agree with retail but its VC4 register allocation differs. `MainLoop`
at `0x7A5A0` is a reviewed call identity from this body and the donor; it
remains unclaimed.

## Recovered dialog and dispatch family

Retail DoDialog at RVA 0x7A2C0 allocates 0x100 stack bytes. Its three link
arrays begin at stack offsets 0x1C, 0x6C and 0xBC after callee-save pushes:
the 0x50 spacing proves twenty four-byte manager pointers per array. The
remaining sixteen bytes hold the local executive. The return at 0x7A3BB
loads AX from its result field, proving a short DoDialog return. Four calls
to ShutDown use distinct 20-byte error slots at 0xA1920, 0xA1934, 0xA1948
and 0xA195C; these are code-required identities only, without data-byte
initializer claims. Buka BASE/EXEC.cpp supplies corresponding link save
and restoration source.

MainLoop at RVA 0x7A5A0 ends with RET at 0x7A6A7; eight following INT3
bytes are padding. Its message type and executive command are WORD fields
at message offsets 0 and 2; command values 1, 2 and 4 terminate, remove
the active manager, and store the DWORD result at message offset 12. The
loop termination flag uses BL and INC BL; dispatch uses a BYTE stack home.

DoDialog retail expands the four stores of the executive constructor.
An inline declaration control reproduced that expansion but removed the
standalone constructor object in the incomplete family, so that control
was restored. Retail xrefs identify InitMainClasses as the actual caller
of the standalone constructor; recovering that caller is the remaining
inline-ownership investigation. No emission-forcing device was retained.

## Lifecycle entry and shutdown

InitSystem at RVA 0x7A180 returns zero in AX and ends at 0x7A228;
seven following INT3 bytes are alignment padding. It opens resource, input
and sound managers with priority -1, then adds mouse and window managers.
Unlike Buka, retail does not guard sound initialization with an editor-mode
test. Its five distinct error slots start at 0xA1820, 0xA1858, 0xA18A8,
0xA18C4 and 0xA18E0. These are code-required identities without initializer
claims.

ShutDownSystem at RVA 0x7A230 ends at 0x7A2B3, before twelve padding bytes.
It calls EarlyShutDownSystem, closes sound, preserves the next link while
removing non-window/non-mouse managers, removes active window and mouse
managers, then closes resource and input managers in that order. The final
two closes are reversed relative to Buka. EarlyShutDownSystem at RVA
0x55C94 is a complete empty hook: the sixteen retail bytes contain only an
ordinary frame, jump to epilogue and return, agreeing with both donors.

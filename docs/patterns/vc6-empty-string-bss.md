# VC6 empty narrow strings can live in BSS

The pinned Buka VC6 SP5 profile emits `""` as a one-byte `$SG` member in
`.bss`, with no raw COFF payload. Its address is still an ordinary DIR32
reference. Treating every compiler string as initialized `.data` loses this
identity even when all of the surrounding function instructions match.

A measured example is `resourceManager::resourceManager` in `BASE/RESMGR`:

```cpp
strcpy(m_lastFileName, "");
```

The retail review pairs the operand at Buka RVA `0x6c133` with the compiler's empty-string member.
It points to RVA `0xcfb48`, in the PE's loader-zero tail. The compiled member is
in `.bss`; its semantic extent is one NUL byte, not its alignment padding or
the distance to the next symbol. Compiler `$SG` ordinals are not stable names.

The ordinary literal exporter now accepts this case only when its existing
reference pairing establishes one address, the PE read proves the complete
zero payload, and retail storage agrees with the candidate's BSS section.
Inside ambiguous PE FileAlignment slack, the candidate section resolves the
storage class by the same rule already used for named source DATA claims.
Initialized retail storage is never silently changed to BSS.

Zero-filled bytes do not establish an address. Unreferenced empty literals
are withheld; neither neighboring allocations nor a zero-byte search can
place them. Real-COFF tests in `homm1.delink.test_bss_literals` cover these
negative cases, invalid payloads/storage, and the positive referenced case.

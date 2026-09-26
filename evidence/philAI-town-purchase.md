# CreaturesToBuy(town *, int) (retail RVA 0x1e3fe)

Buka 2.1 SOURCE/PHILAI.cpp lines 3158–3160 supply the surviving overload:
read town garrison at the requested level, then call the creature/count
overload with gDwellingType indexed by town type and level. Retail confirms
the signed type byte at town+3, signed short stock at town+0x1a+level*2,
six-byte table row stride, and direct call to RVA 0x1e446. The 0x48-byte
candidate matches 100% in code mode.

The root's retail-backed packed town prefix ends at 0x1a; the six stock
shorts follow it. Pinned retail table VA 0x4913a0 contains four six-byte
faction rows (24 bytes), distinct from the following unrelated integer
storage. The four factions contain the 24 town creatures in the expected
donor-correspondent order; these bytes establish identity and row layout,
not admitted initializer coverage. Only table storage and code-use identity
are modeled in this code-first batch.

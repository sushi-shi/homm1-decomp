// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <SOURCE/PATH.h>

#include <H1/All.h>

// donor PoL RVA 0x000bdd3a; preferred Buka symbol ?ValidPath@army@@QAEHHH@Z
// donor Buka TU SOURCE/PATH; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.547636;margin=0.507012;shape=0.439;size=0.810;calls=1.000;alternate=pol20:int army::ValidPath(int, int)@0x000bdd3a
VA(0x00418242, 0x9e)
int army::ValidPath(int, int) { return 0; }

// HoMM2 donor behavior; HoMM1's WORD parameter/return prove the narrower API.
// donor Buka TU SOURCE/PATH; HoMM1 owner inferred from contiguous order
// evidence: retail body uses signed WORD loads and returns through AX;
// alternate=pol20:int OppositeDirection(int)@0x000be9e7
VA(0x00418fb0, 0x58)
H1_ENUM_RETURN(CombatHexDirection, short)
OppositeDirection(H1_ENUM_PARAM(CombatHexDirection, short) direction)
{
    if (static_cast<int>(direction) < COMBAT_DIRECTION_ADJACENT_COUNT)
        return H1_ENUM_CAST(CombatHexDirection, short,
            (static_cast<int>(direction) + COMBAT_DIRECTION_OPPOSITE_OFFSET)
                % COMBAT_DIRECTION_ADJACENT_COUNT);
    else {
        if (direction == COMBAT_DIRECTION_WIDE_WEST)
            return COMBAT_DIRECTION_WIDE_EAST;
        else
            return COMBAT_DIRECTION_WIDE_WEST;
    }
}

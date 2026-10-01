// HoMM1's CURSOR object (0x00405950-0x00407d8f): Buka SOURCE/CURSOR
// advManager movement routines, aligned apart from wingraph and TOWNMGR.

#include <match.h>

#include <BASE/INPUTMGR_TYPES.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/kbwin.h>

// donor PoL RVA 0x0000e198; preferred Buka symbol ?GetCursorBaseFrame@advManager@@QAEHH@Z
// donor Buka TU SOURCE/CURSOR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.375377;margin=0.466673;shape=0.186;size=0.574;calls=1.000;alternate=pol20:int advManager::GetCursorBaseFrame(int)@0x0000e198
VA(0x004061ed, 0x88)
short advManager::GetCursorBaseFrame(H1_ENUM_PARAM(MapDirection, short) direction)
{
    if (static_cast<int>(direction) > static_cast<int>(MAP_DIRECTION_SOUTH)) {
        switch (direction) {
            case MAP_DIRECTION_SOUTH_WEST:
                return static_cast<short>(CURSOR_BOAT_BASE_FRAME_5);
            case MAP_DIRECTION_WEST:
                return static_cast<short>(CURSOR_BOAT_BASE_FRAME_6);
            case MAP_DIRECTION_NORTH_WEST:
                return static_cast<short>(CURSOR_BOAT_BASE_FRAME_7);
            default:
                return 0;
        }
    } else {
        return static_cast<int>(direction) * static_cast<int>(CURSOR_FRAMES_PER_DIRECTION);
    }
}

// donor PoL RVA 0x0000e21d; preferred Buka symbol ?TurnTo@advManager@@QAEXH@Z
// donor Buka TU SOURCE/CURSOR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.461800;margin=0.347232;shape=0.223;size=0.872;calls=1.000;alternate=pol20:void advManager::TurnTo(int)@0x0000e21d
VA(0x00406275, 0x261)
void advManager::TurnTo(int) {}

// donor PoL RVA 0x0000e51f; preferred Buka symbol ?MoveHero@advManager@@QAEPAVmapCell@@HHPAH00H0H@Z
// donor Buka TU SOURCE/CURSOR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.461867;margin=0.353958;shape=0.281;size=0.831;calls=0.879;alternate=pol20:class mapCell * advManager::MoveHero(int, int, int *, int *, int *, int, int *, int)@0x0000e51f
VA(0x0040660c, 0xe1e)
class mapCell * advManager::MoveHero(int, int, int *, int *, int *, int, int *, int) { return 0; }

// donor PoL RVA 0x0000f753; preferred Buka symbol ?CheckAdjacentMon@advManager@@QAEXPAH@Z
// donor Buka TU SOURCE/CURSOR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.530646;margin=0.751795;shape=0.360;size=0.888;calls=1.000;alternate=pol20:void advManager::CheckAdjacentMon(int *)@0x0000f753
VA(0x0040742a, 0x181)
void advManager::CheckAdjacentMon(int *) {}

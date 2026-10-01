// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/INPUTMGR_TYPES.h>
#include <H1/All.h>
#include <H1/KB.h>

#include <string.h>

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

// donor PoL RVA 0x00013900; preferred Buka symbol ??0townObject@@QAE@HHPAD@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.615649;margin=0.314990;shape=0.431;size=0.799;calls=0.333;strings=%s.icn;alternate=pol20:void townObject::constructor(int, int, char *)@0x00013900
VA(0x00407d90, 0x1f1)
townObject::townObject(int, int, char *) {}

// donor PoL RVA 0x00013a6a; preferred Buka symbol ??1townObject@@QAE@XZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.564007;margin=0.293257;shape=0.438;size=0.896;calls=1.000;alternate=pol20:void townObject::~destructor(void)@0x00013a6a
VA(0x00407f81, 0x60)
townObject::~townObject() {
    if (m_border != 0)
        delete m_border;
    gpResourceManager->Dispose(m_icon);
}

// Buka TOWNMGR.cpp:537-625; HoMM1 draws the base frame, then the castle's
// mage-guild levels and the animation frame.
VA(0x00407fe1, 0x117)
void townObject::Draw(signed char advanceAnimation)
{
    short level;

    if (!m_visible)
        return;
    m_icon->DrawToBuffer(0, 0, 0, 0, 0);
    if (m_buildingId == 0) {
        for (level = 0; level < gpTownManager->m_town->m_buildState; level++)
            m_icon->DrawToBuffer(0, 0, (level + 1) * 2, 0, 0);
        m_icon->DrawToBuffer(0, 0, gpTownManager->m_town->m_buildState * 2 + 1, 0, 0);
    }
    if (m_animationFrameCount) {
        m_icon->DrawToBuffer(0, 0, m_animationFrame + 1, 0, 0);
        if (advanceAnimation == 1) {
            m_animationFrame++;
            if (m_animationFrame == m_animationFrameCount)
                m_animationFrame = 0;
        }
    }
}

// Buka TOWNMGR.cpp:627-633; HoMM1 also clears the object count and adds
// its dispatch mask.
VA(0x004080f8, 0x74)
townManager::townManager(void)
{
    m_town = 0;
    m_townObjectCount = 0;
    m_heroWindow0 = 0;
    m_unknown79 = 0;
    m_selectedBuilding = -1;
    m_castleDialogActive = 0;
    m_dispatchMask = TOWN_MANAGER_DISPATCH_MASK;
}

// donor PoL RVA 0x0001436f; preferred Buka symbol ?SetupTown@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.644440;margin=0.052173;shape=0.362;size=0.803;calls=0.887;strings=%s%s|port%04d.icn|strip.icn;alternate=pol20:void townManager::SetupTown(void)@0x0001436f
// Retail vtable slot 0 (0x0048c068): HoMM1's Open performs Buka's SetupTown work.
VA(0x0040816c, 0x7ec)
short townManager::Open(short) { return 0; }

// donor PoL RVA 0x00014cc9; preferred Buka symbol ?UnloadTown@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.470224;margin=0.176996;shape=0.284;size=0.924;calls=0.667;alternate=pol20:void townManager::UnloadTown(void)@0x00014cc9
// Retail vtable slot 1: HoMM1's Close performs Buka's UnloadTown work.
VA(0x00408958, 0x1c4)
void townManager::Close(void)
{
    short index;

    delete m_bankBox;
    if (m_heroStrip)
        delete m_heroStrip;
    delete m_garrisonStrip;
    for (index = 0; index < m_townObjectCount; index++) {
        m_townWindow->RemoveWidget(m_townObjects[index]->m_border);
        delete m_townObjects[index];
    }
    gpResourceManager->Dispose(m_backgroundIcon);
    gpWindowManager->RemoveWindow(m_townWindow);
    delete m_townWindow;
    gpSoundManager->SwitchAmbientMusic(-1);
    gpWindowManager->FadeScreen(1, 8, 0);
    gpMouseManager->SetPointer(-1);
    m_active = 0;
}

// donor PoL RVA 0x000158e0; preferred Buka symbol ?ShowText@townManager@@QAEXPAD@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.613333;margin=0.109874;shape=0.519;size=1.000;calls=1.000;alternate=pol20:void townManager::ShowText(char *)@0x000158e0
VA(0x0040933a, 0x74)
void townManager::ShowText(char *)
{
    tag_message message;

    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = TOWN_STATUS_TEXT_CONTROL;
    message.payload.widget.data.text = m_statusText;
    m_townWindow->BroadcastMessage(message);
    m_townWindow->DrawWindow(0, TOWN_STATUS_TEXT_CONTROL - 2, TOWN_STATUS_TEXT_CONTROL);
    gpWindowManager->UpdateScreenRegion(0, TOWN_STATUS_REGION_Y, TOWN_STATUS_REGION_WIDTH,
                                        TOWN_STATUS_REGION_HEIGHT);
}

// donor PoL RVA 0x0001595d; preferred Buka symbol ?Main@townManager@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.513026;margin=0.490356;shape=0.272;size=0.823;calls=0.467;strings=caslwind.bin|magewind.bin|thiefwin.bin;alternate=pol20:int townManager::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x0001595d
VA(0x004093ae, 0x131f)
short townManager::Main(struct tag_message &) { return 0; }

// donor PoL RVA 0x0001718d; preferred Buka symbol ?DoCommand@townManager@@QAEXH@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.443170;margin=0.301918;shape=0.326;size=0.721;calls=0.800;alternate=pol20:void townManager::DoCommand(int)@0x0001718d
VA(0x0040a6cd, 0x65f)
void townManager::DoCommand(int) {}

// Buka TOWNMGR.cpp:1905-1921; HoMM1 redraws strips before the status text.
VA(0x0040ad2c, 0xa5)
void townManager::RedrawTownScreen(void)
{
    tag_message message;

    DrawTown(1, 1);
    m_garrisonStrip->DrawIcons(1);
    m_heroStrip->DrawIcons(1);
    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = TOWN_STATUS_TEXT_CONTROL;
    message.payload.widget.data.text = m_statusText;
    m_townWindow->BroadcastMessage(message);
    m_townWindow->DrawWindow(0);
    gpWindowManager->UpdateScreenRegion(0, 0x100, 0x280, 0x1e0);
    m_bankBox->Update();
}

// donor PoL RVA 0x0001771d; preferred Buka symbol ?SplitArmy@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.732616;margin=0.021648;shape=0.430;size=0.991;calls=1.000;strings=splitwin.bin;alternate=pol20:void townManager::SplitArmy(void)@0x0001771d
VA(0x0040add1, 0x37e)
void townManager::SplitArmy(void) {}

// donor PoL RVA 0x00017ab2; preferred Buka symbol ?ResetStrips@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.515580;margin=0.398432;shape=0.375;size=0.860;calls=1.000;alternate=pol20:void townManager::ResetStrips(void)@0x00017ab2
VA(0x0040b21d, 0xab)
void townManager::ResetStrips(void)
{
    if (m_swapStrip)
        m_swapStrip->m_selectedSlot = -1;
    if (m_pendingStrip)
        m_pendingStrip->m_selectedSlot = -1;
    m_heroStrip->Draw();
    m_garrisonStrip->Draw();
    m_swapStrip = m_pendingStrip = 0;
    m_swapArmySlot = m_pendingArmySlot = -1;
}

// Buka TOWNMGR.cpp:1993-2003.
VA(0x0040b2c8, 0x95)
void townManager::Toggle(signed char building)
{
    short index;

    if (m_town->m_buildings & (1 << building)) {
        for (index = 0; index < m_townObjectCount; index++) {
            if (m_townObjects[index]->m_buildingId == building)
                m_townObjects[index]->m_visible ^= 1;
        }
    }
}

// donor PoL RVA 0x00017c9d; preferred Buka symbol ?BuyBuild@townManager@@QAEHHHH@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.668468;margin=0.054253;shape=0.329;size=0.894;calls=0.897;strings=bigfont.fnt|buybuil%d.bin|resource.icn;alternate=pol20:int townManager::BuyBuild(int, int, int)@0x00017c9d
VA(0x0040b455, 0x1023)
int townManager::BuyBuild(int, int, int) { return 0; }

// donor PoL RVA 0x00018bd2; preferred Buka symbol ?BuildObj@townManager@@QAEXH@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.630156;margin=0.342681;shape=0.206;size=0.999;calls=0.933;strings=buildtwn.82M;alternate=pol20:void townManager::BuildObj(int)@0x00018bd2
VA(0x0040c478, 0x3a0)
void townManager::BuildObj(int) {}

// donor PoL RVA 0x0001d040; preferred Buka symbol ?SetupCastle@townManager@@QAEXPAVheroWindow@@H@Z
// donor Buka TU SOURCE/Castle; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.228280;margin=0.655471;shape=0.234;size=0.312;calls=0.264;alternate=pol20:void townManager::SetupCastle(class heroWindow *, int)@0x0001d040
VA(0x0040c818, 0x4b5)
void townManager::SetupCastle(class heroWindow *, int) {}

// donor PoL RVA 0x00018fbb; preferred Buka symbol ?SetupMage@townManager@@QAEXPAVheroWindow@@@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.426387;margin=0.115621;shape=0.211;size=0.919;calls=0.625;alternate=pol20:void townManager::SetupMage(class heroWindow *)@0x00018fbb
VA(0x0040cf1a, 0x331)
void townManager::SetupMage(class heroWindow *) {}

// donor PoL RVA 0x0001a783; preferred Buka symbol ?SetupThievesGuild@townManager@@QAEXPAVheroWindow@@H@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.350822;margin=0.362412;shape=0.263;size=0.210;calls=0.163;strings=townwind.icn;alternate=pol20:void townManager::SetupThievesGuild(class heroWindow *, int)@0x0001a783
VA(0x0040d3d1, 0x2ec)
void townManager::SetupThievesGuild(class heroWindow *, int) {}

// donor PoL RVA 0x0001b692; preferred Buka symbol ?GetCategoryStats@@YIXHQAJQAC@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.459458;margin=0.479043;shape=0.193;size=0.976;calls=0.800;alternate=pol20:void GetCategoryStats(int, long int * const, signed char * const)@0x0001b692
VA(0x0040d6bd, 0x484)
void GetCategoryStats(int, long int * const, signed char * const) {}

// HoMM1 town-type wrapper over the global building-name table lookup.
VA(0x0040dc2b, 0x2f)
char *townManager::GetBuildingName(int building)
{
    return ::GetBuildingName(m_town->m_type, building);
}

// donor PoL RVA 0x00019523; preferred Buka symbol ?RecruitHero@townManager@@QAEHHH@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.555715;margin=0.218228;shape=0.276;size=0.741;calls=0.578;strings=port%04d.icn|rcrthero.bin;alternate=pol20:int townManager::RecruitHero(int, int)@0x00019523
VA(0x0040dc5a, 0x981)
int townManager::RecruitHero(int, int) { return 0; }

// donor PoL RVA 0x00019c29; preferred Buka symbol ?TavernHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.461090;margin=0.267847;shape=0.244;size=0.845;calls=1.000;alternate=pol20:int TavernHandler(struct tag_message &)@0x00019c29
VA(0x0040e5db, 0x155)
int TavernHandler(struct tag_message &) { return 0; }

// donor PoL RVA 0x00019d7c; preferred Buka symbol ?DoTavern@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.728216;margin=0.164549;shape=0.467;size=0.965;calls=0.889;strings=tavwin.bin;alternate=pol20:void townManager::DoTavern(void)@0x00019d7c
VA(0x0040e730, 0x136)
void townManager::DoTavern(void) {}

// donor PoL RVA 0x0001e0fb; preferred Buka symbol ?CastleHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/Castle; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.424517;margin=0.478912;shape=0.381;size=0.626;calls=0.675;alternate=pol20:int CastleHandler(struct tag_message &)@0x0001e0fb
VA(0x0040e866, 0x726)
int CastleHandler(struct tag_message &) { return 0; }

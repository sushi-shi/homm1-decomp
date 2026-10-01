// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/kbwin.h>

#include <stdlib.h>
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
// Buka TOWNMGR.cpp townObject ctor; HoMM1 reads frame count, rectangle and
// building id from the .tod resource instead of sBuildingInfo.
VA(0x00407d90, 0x1f1)
townObject::townObject(char *name)
{
    char fileName[16];
    short w;
    short tmp;
    short id;
    short x;
    short h;
    short y;

    m_animationFrame = 0;
    m_icon = 0;
    m_border = 0;
    m_visible = 1;
    sprintf(fileName, "%s.tod", name);
    id = gpResourceManager->MakeId(fileName);
    gpResourceManager->PointToFile(id);
    m_animationFrameCount = gpResourceManager->ReadByte();
    x = gpResourceManager->ReadWord();
    y = gpResourceManager->ReadWord();
    w = gpResourceManager->ReadWord();
    h = gpResourceManager->ReadWord();
    id = gpResourceManager->ReadWord();
    m_buildingId = id;
    sprintf(fileName, "%s.icn", name);
    m_icon = gpResourceManager->GetIcon(fileName);
    if (id == 0) {
        h = gpTownManager->m_town->m_buildState * 20 + 0x61;
        y = 0x99 - h;
    }
    if (id != -1) {
        m_border = new border(x, y, w, h, id, 1, 0, 0);
        if (m_border == 0)
            MemError();
    }
}

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
    gpResourceManager->Dispose(m_backgroundBitmap);
    gpWindowManager->RemoveWindow(m_townWindow);
    delete m_townWindow;
    gpSoundManager->SwitchAmbientMusic(-1);
    gpWindowManager->FadeScreen(1, 8, 0);
    gpMouseManager->SetPointer(-1);
    m_active = 0;
}

// Buka TOWNMGR.cpp:944-1020; HoMM1 matches the dragged creature against
// every slot of the target army and keeps word-sized flags.
VA(0x00408b1c, 0x3b6)
void townManager::SetArmyCommand(short qualifier)
{
    short lastArmy;
    short i;
    short sameType;

    m_command = TOWN_ARMY_COMMAND_NONE;
    lastArmy = 0;
    if (m_swapStrip->m_army->GetNumArmies() == 1 && m_swapStrip == m_heroStrip
        && m_swapStrip != m_pendingStrip)
        lastArmy = 1;

    if (m_swapStrip != m_pendingStrip) {
        sameType = 0;
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            if (m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]
                == m_pendingStrip->m_army->m_creatureTypes[i])
                sameType = 1;
        }
        if (sameType) {
            if (qualifier) {
                sprintf(m_statusText, cTownCommand[TOWN_TEXT_REDISTRIBUTE_ARMY],
                        gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]);
                m_command = TOWN_ARMY_COMMAND_SPLIT;
            } else if (lastArmy) {
                strcpy(m_statusText, cTownCommand[TOWN_TEXT_CANNOT_COMBINE_LAST_ARMY]);
                return;
            } else {
                sprintf(m_statusText, cTownCommand[TOWN_TEXT_COMBINE_ARMIES],
                        gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]);
                m_command = TOWN_ARMY_COMMAND_MERGE;
            }
        } else if (qualifier && m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] == -1) {
            sprintf(m_statusText, cTownCommand[TOWN_TEXT_REDISTRIBUTE_TO_EMPTY_SLOT],
                    gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]);
            m_command = TOWN_ARMY_COMMAND_SPLIT;
        }
    } else if (m_swapArmySlot == m_pendingArmySlot) {
        sprintf(m_statusText, cTownCommand[TOWN_TEXT_VIEW_ARMY],
                gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]);
        m_command = TOWN_ARMY_COMMAND_VIEW;
    }

    if (m_command != TOWN_ARMY_COMMAND_NONE)
        return;
    if (m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] == -1) {
        if (lastArmy) {
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_CANNOT_MOVE_LAST_ARMY]);
            return;
        } else {
            sprintf(m_statusText, cTownCommand[TOWN_TEXT_MOVE_ARMY],
                    gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]]);
            m_command = TOWN_ARMY_COMMAND_SWAP;
        }
    } else {
        sprintf(m_statusText, cTownCommand[TOWN_TEXT_EXCHANGE_ARMIES],
                gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]],
                gArmyNames[m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot]]);
        m_command = TOWN_ARMY_COMMAND_SWAP;
    }
}

// Buka TOWNMGR.cpp:1022-1176; HoMM1 has no calendar entry and names the
// six dwellings through gDwellingType.
VA(0x00408ed2, 0x468)
void townManager::SetCommandAndText(struct tag_message &message)
{
    short id;

    id = message.payload.widget.id;
    m_command = TOWN_ARMY_COMMAND_NONE;
    switch (id) {
        case TOWN_CLOSE_CONTROL:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_EXIT]);
            break;
        case -1:
        case TOWN_EMPTY_STATUS_CONTROL_FIRST:
        case TOWN_EMPTY_STATUS_CONTROL_LAST:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_EMPTY_STATUS]);
            break;
        case TOWN_GARRISON_FIRST_CONTROL:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_GARRISON]);
            m_command = TOWN_ARMY_COMMAND_GARRISON;
            break;
        case TOWN_GARRISON_SLOT_FIRST:
        case TOWN_GARRISON_SLOT_FIRST + 1:
        case TOWN_GARRISON_SLOT_FIRST + 2:
        case TOWN_GARRISON_SLOT_FIRST + 3:
        case TOWN_GARRISON_SLOT_FIRST + 4:
            if (m_swapArmySlot != -1) {
                m_pendingStrip = m_garrisonStrip;
                m_pendingArmySlot = id - TOWN_GARRISON_SLOT_FIRST;
                SetArmyCommand(message.payload.mouse.modifiers & TOWN_SHIFT_QUALIFIER_MASK);
            } else {
                m_selectedStrip = m_garrisonStrip;
                m_selectedArmySlot = id - TOWN_GARRISON_SLOT_FIRST;
                if (m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot] == -1)
                    strcpy(m_statusText, cTownCommand[TOWN_TEXT_EMPTY_SLOT]);
                else {
                    sprintf(m_statusText, cTownCommand[TOWN_TEXT_SELECT_ARMY],
                            gArmyNames[m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot]]);
                    m_command = TOWN_ARMY_COMMAND_SELECT;
                }
            }
            break;
        case TOWN_HERO_FIRST_CONTROL:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_VIEW_HERO]);
            m_command = TOWN_ARMY_COMMAND_VIEW_HERO;
            break;
        case TOWN_HERO_SLOT_FIRST:
        case TOWN_HERO_SLOT_FIRST + 1:
        case TOWN_HERO_SLOT_FIRST + 2:
        case TOWN_HERO_SLOT_FIRST + 3:
        case TOWN_HERO_SLOT_FIRST + 4:
            if (m_swapArmySlot != -1) {
                m_pendingStrip = m_heroStrip;
                m_pendingArmySlot = id - TOWN_HERO_SLOT_FIRST;
                SetArmyCommand(message.payload.mouse.modifiers & TOWN_SHIFT_QUALIFIER_MASK);
            } else {
                m_selectedStrip = m_heroStrip;
                m_selectedArmySlot = id - TOWN_HERO_SLOT_FIRST;
                if (m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot] == -1) {
                    strcpy(m_statusText, cTownCommand[TOWN_TEXT_EMPTY_SLOT]);
                    m_command = TOWN_ARMY_COMMAND_NONE;
                } else {
                    sprintf(m_statusText, cTownCommand[TOWN_TEXT_SELECT_ARMY],
                            gArmyNames[m_selectedStrip->m_army->m_creatureTypes[m_selectedArmySlot]]);
                    m_command = TOWN_ARMY_COMMAND_SELECT;
                }
            }
            break;
        case 0:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_BUILDING_0]);
            break;
        case 1:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_BUILDING_0 + 1]);
            break;
        case 2:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_BUILDING_0 + 2]);
            break;
        case 3:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_BUILDING_0 + 3]);
            break;
        case 4:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_BUILDING_0 + 4]);
            break;
        case 5:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_BUILDING_0 + 5]);
            break;
        case 6:
            strcpy(m_statusText, cTownCommand[TOWN_TEXT_BUILDING_0 + 6]);
            break;
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            sprintf(m_statusText, cTownCommand[TOWN_TEXT_DWELLING],
                    gArmyNames[gDwellingType[m_town->m_type][id - 7]]);
            break;
    }
    ShowText(m_statusText);
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
// Buka TOWNMGR.cpp:1923-1970; HoMM1 always names both armies and merges
// into the first matching slot of the target army.
VA(0x0040add1, 0x37e)
void townManager::SplitArmy(void)
{
    short messageId = 1;
    tag_message message;
    short merge;
    short numberId = TOWN_SPLIT_SETUP_AMOUNT_CONTROL;
    short n;

    m_heroWindow1 = new heroWindow(TOWN_SPLIT_WINDOW_X, TOWN_SPLIT_WINDOW_Y, "splitwin.bin");
    if (m_heroWindow1 == 0)
        MemError();
    m_splitAmount = 0;
    m_splitMaximum = m_swapStrip->m_army->m_creatureCounts[m_swapArmySlot];
    message.type = MESSAGE_WIDGET;
    sprintf(gText, "Move how many %s troops from %s to %s?",
            gArmyNames[m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]],
            m_swapStrip == m_heroStrip ? "Hero's Army" : "Garrison",
            m_pendingStrip == m_heroStrip ? "Hero's Army" : "Garrison");
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = 1;
    message.payload.widget.data.text = gText;
    m_heroWindow1->BroadcastMessage(message);
    sprintf(gText, "%d", m_splitAmount);
    message.payload.widget.id = TOWN_SPLIT_SETUP_AMOUNT_CONTROL;
    message.payload.widget.data.text = gText;
    m_heroWindow1->BroadcastMessage(message);
    gpWindowManager->DoDialog(m_heroWindow1, SplitArmyHandler, 0);
    delete m_heroWindow1;
    if (gpWindowManager->m_dialogResult == TOWN_DIALOG_BUTTON_2) {
        merge = 0;
        for (n = 0; n < ARMY_GROUP_SLOT_COUNT; n++) {
            if (m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot]
                == m_pendingStrip->m_army->m_creatureTypes[n]) {
                merge = 1;
                break;
            }
        }
        if (merge)
            m_pendingStrip->m_army->m_creatureCounts[n] += m_splitAmount;
        else {
            m_pendingStrip->m_army->m_creatureTypes[m_pendingArmySlot] =
                m_swapStrip->m_army->m_creatureTypes[m_swapArmySlot];
            m_pendingStrip->m_army->m_creatureCounts[m_pendingArmySlot] = m_splitAmount;
        }
        m_swapStrip->m_army->m_creatureCounts[m_swapArmySlot] -= m_splitAmount;
    }
}

// HoMM1 re-evaluates the pending strip command when the shift qualifier
// changes, then refreshes the status line.
VA(0x0040b14f, 0xce)
void townManager::ShiftQualChange(void)
{
    tag_message message;

    if (m_swapStrip != m_pendingStrip
        && (m_command == TOWN_ARMY_COMMAND_NONE || m_command == TOWN_ARMY_COMMAND_SPLIT
            || m_command == TOWN_ARMY_COMMAND_MERGE || m_command == TOWN_ARMY_COMMAND_SWAP))
        SetArmyCommand(gpInputManager->GetModifiers() & TOWN_SHIFT_QUALIFIER_MASK);
    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = TOWN_STATUS_TEXT_CONTROL;
    message.payload.widget.data.text = m_statusText;
    m_townWindow->BroadcastMessage(message);
    m_townWindow->DrawWindow();
}

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

// Buka TOWNMGR.cpp:2005-2029; HoMM1 draws a bitmap background and folds
// the mouse pointer into the screen buffer around the viewport blit.
VA(0x0040b35d, 0xf8)
void townManager::DrawTown(signed char updateScreen, int drawFlags)
{
    short index;
    short x;
    short y;

    m_backgroundBitmap->DrawToBuffer(0, 0);
    for (index = 0; index < m_townObjectCount; index++)
        m_townObjects[index]->Draw(drawFlags);
    m_townWindow->DrawWindow(0, TOWN_REDRAW_FIRST_CONTROL, TOWN_REDRAW_LAST_CONTROL);
    gpMouseManager->MouseCoords(x, y);
    if (y < TOWN_VIEWPORT_HEIGHT)
        gpMouseManager->SaveAndDraw(gpWindowManager->m_screen, 0, 0, 1);
    if (updateScreen)
        BlitBitmapToScreen(gpWindowManager->m_screen, 0, 0, TOWN_VIEWPORT_WIDTH,
                           TOWN_VIEWPORT_HEIGHT, 0, 0);
    if (y < TOWN_VIEWPORT_HEIGHT)
        gpMouseManager->RestoreUnderlying();
}

// donor PoL RVA 0x00017c9d; preferred Buka symbol ?BuyBuild@townManager@@QAEHHHH@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.668468;margin=0.054253;shape=0.329;size=0.894;calls=0.897;strings=bigfont.fnt|buybuil%d.bin|resource.icn;alternate=pol20:int townManager::BuyBuild(int, int, int)@0x00017c9d
VA(0x0040b455, 0x1023)
int townManager::BuyBuild(int, int, int) { return 0; }

// donor PoL RVA 0x00018bd2; preferred Buka symbol ?BuildObj@townManager@@QAEXH@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.630156;margin=0.342681;shape=0.206;size=0.999;calls=0.933;strings=buildtwn.82M;alternate=pol20:void townManager::BuildObj(int)@0x00018bd2
// Buka TOWNMGR.cpp:2475; HoMM1 fizzles a fixed per-building rectangle
// instead of computing the drawn extent.
VA(0x0040c478, 0x3a0)
void townManager::BuildObj(short building)
{
    short index;
    SAMPLE2 buildSample;

    gpMouseManager->ReallyHidePointer();
    DrawTown(1, 1);
    if (building == TOWN_BUILDING_MAGE_GUILD) {
        if (m_town->m_buildings & 1)
            m_town->m_buildState++;
        if (m_town->m_occupyingHeroId != -1)
            m_town->GiveSpells();
    }
    m_town->m_buildings |= 1 << building;
    if (building >= TOWN_BUILDING_FIRST_DWELLING && building <= TOWN_BUILDING_LAST_DWELLING)
        m_town->m_garrison[building - TOWN_BUILDING_FIRST_DWELLING] =
            gMonsterDatabase[gDwellingType[m_town->m_type][building - TOWN_BUILDING_FIRST_DWELLING]]
                .growth;
    for (index = 0; index < m_townObjectCount; index++) {
        if (m_townObjects[index]->m_buildingId == building) {
            m_townObjects[index]->m_visible = 1;
            m_townObjects[index]->m_border->m_flags |= TOWN_OBJECT_ENABLED_FLAG;
        }
    }
    if (building == TOWN_BUILDING_CASTLE) {
        m_town->m_buildings &= ~(1 << TOWN_BUILDING_TENT);
        for (index = 0; index < m_townObjectCount; index++) {
            if (m_townObjects[index]->m_buildingId == TOWN_BUILDING_TENT) {
                m_townObjects[index]->m_visible = 0;
                m_townObjects[index]->m_border->m_flags &= ~TOWN_OBJECT_ENABLED_FLAG;
            }
        }
    }
    gpWindowManager->SaveFizzleSource(gTownBuildingExtents[m_town->m_type][building].x,
                                      gTownBuildingExtents[m_town->m_type][building].y,
                                      gTownBuildingExtents[m_town->m_type][building].width,
                                      gTownBuildingExtents[m_town->m_type][building].height);
    DrawTown(0, 1);
    buildSample = NULL_SAMPLE2;
    buildSample = LoadPlaySample("buildtwn.82M");
    gpWindowManager->FizzleForward(gTownBuildingExtents[m_town->m_type][building].x,
                                   gTownBuildingExtents[m_town->m_type][building].y,
                                   gTownBuildingExtents[m_town->m_type][building].width,
                                   gTownBuildingExtents[m_town->m_type][building].height, -1);
    WaitEndSample(buildSample, -1);
    m_selectedBuilding = -1;
    m_bankBox->Update();
    m_townWindow->DrawWindow();
    gpMouseManager->ReallyShowPointer();
    gpWindowManager->BroadcastMessage(MESSAGE_WIDGET, WIDGET_COMMAND_CLEAR_FLAGS, TOWN_DIALOG_BUTTON_0,
                                      0x4008);
    BitSet(gpGame->m_townBuiltToday, m_town->m_id);
    m_town->GiveSpells();
}

// donor PoL RVA 0x0001d040; preferred Buka symbol ?SetupCastle@townManager@@QAEXPAVheroWindow@@H@Z
// donor Buka TU SOURCE/Castle; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.228280;margin=0.655471;shape=0.234;size=0.312;calls=0.264;alternate=pol20:void townManager::SetupCastle(class heroWindow *, int)@0x0001d040
VA(0x0040c818, 0x4b5)
void townManager::SetupCastle(class heroWindow *, int) {}

// Buka TOWNMGR.cpp:3131 SetupWell; HoMM1 has six fixed dwellings and
// capitalises the creature name in gText.
VA(0x0040cccd, 0x24d)
void townManager::SetupWell(class heroWindow *window)
{
    short iconBase = 1;
    short buildingName = 7;
    short growthRate;
    short firstMonsterIcon = 0xd;
    short creatureId = 0x13;
    short firstAvailable = 0x19;
    tag_message message;
    short i;

    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_FRAME;
    for (i = 0; i < TOWN_WELL_DWELLING_COUNT; i++) {
        message.payload.widget.id = i + TOWN_WELL_FIRST_ICON_CONTROL;
        message.payload.widget.data.value =
            (m_town->m_type + 1) * TOWN_WELL_FRAMES_PER_TYPE + i + 1;
        window->BroadcastMessage(message);
        message.payload.widget.id = i + TOWN_WELL_FIRST_MONSTER_ICON_CONTROL;
        message.payload.widget.data.value = gDwellingType[m_town->m_type][i];
        window->BroadcastMessage(message);
    }
    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    for (i = 0; i < TOWN_WELL_DWELLING_COUNT; i++) {
        message.payload.widget.id = i + TOWN_WELL_FIRST_NAME_CONTROL;
        message.payload.widget.data.text = GetBuildingName(i + TOWN_WELL_FIRST_DWELLING_BUILDING);
        window->BroadcastMessage(message);
        message.payload.widget.id = i + TOWN_WELL_FIRST_CREATURE_CONTROL;
        strcpy(gText, gArmyNames[gDwellingType[m_town->m_type][i]]);
        gText[0] -= 'a' - 'A';
        message.payload.widget.data.text = gText;
        window->BroadcastMessage(message);
    }
    for (i = 0; i < TOWN_WELL_DWELLING_COUNT; i++) {
        message.payload.widget.id = i + TOWN_WELL_FIRST_AVAILABLE_CONTROL;
        if (!(m_town->m_buildings & (1 << (i + TOWN_WELL_FIRST_DWELLING_BUILDING))))
            strcpy(gText, "Available:\nNONE\nGrowth Rate:\nN/A");
        else {
            growthRate = gMonsterDatabase[gDwellingType[m_town->m_type][i]].growth;
            growthRate += 2;
            sprintf(gText, "Available:\n%d\nGrowth Rate:\n%d/week", m_town->m_garrison[i],
                    growthRate);
        }
        message.payload.widget.data.text = gText;
        window->BroadcastMessage(message);
    }
}

// Buka TOWNMGR.cpp:2597 SetupMage; HoMM1 shows nine guild spells, hiding
// the levels above the guild and stacking tower frames by level.
VA(0x0040cf1a, 0x331)
void townManager::SetupMage(class heroWindow *window)
{
    short off = 0;
    short shown = 1;
    short iconFrame = 2;
    short messageId = TOWN_MAGE_DESCRIPTION_CONTROL;
    short slotBase = TOWN_MAGE_FIRST_SPELL_CONTROL;
    short iconOffset = TOWN_MAGE_FIRST_ICON_CONTROL;
    short nameBase = TOWN_MAGE_FIRST_NAME_CONTROL;
    short firstTower = TOWN_MAGE_FIRST_TOWER_CONTROL;
    tag_message message;
    short spellIndex;
    int spellState;

    message.type = MESSAGE_WIDGET;
    if (m_town->m_occupyingHeroId == -1) {
        strcpy(gText, "The above spells are available here.");
        message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
        message.payload.widget.id = TOWN_MAGE_DESCRIPTION_CONTROL;
        message.payload.widget.data.text = gText;
        window->BroadcastMessage(message);
    }
    for (spellIndex = 0; spellIndex < TOWN_MAGE_SPELL_COUNT; spellIndex++) {
        switch (spellIndex) {
            case 0:
            case 1:
            case 2:
                if (m_town->m_buildState >= 0)
                    spellState = 0;
                else
                    spellState = 1;
                break;
            case 3:
            case 4:
                if (m_town->m_buildState >= 1)
                    spellState = 0;
                else
                    spellState = 1;
                break;
            case 5:
            case 6:
                if (m_town->m_buildState >= 2)
                    spellState = 0;
                else
                    spellState = 1;
                break;
            case 8:
                ;
            default:
                if (m_town->m_buildState >= 3)
                    spellState = 0;
                else
                    spellState = 1;
                break;
        }
        message.payload.widget.command = WIDGET_COMMAND_SET_FRAME;
        message.payload.widget.id = spellIndex + TOWN_MAGE_FIRST_SPELL_CONTROL;
        message.payload.widget.data.value = spellState;
        window->BroadcastMessage(message);
        if (spellState == 1) {
            message.payload.widget.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.payload.widget.data.value = TOWN_MAGE_WIDGET_VISIBLE_FLAG;
            message.payload.widget.id = spellIndex + TOWN_MAGE_FIRST_ICON_CONTROL;
            window->BroadcastMessage(message);
            message.payload.widget.id = spellIndex + TOWN_MAGE_FIRST_NAME_CONTROL;
            window->BroadcastMessage(message);
        } else {
            message.payload.widget.command = WIDGET_COMMAND_SET_FRAME;
            message.payload.widget.id = spellIndex + TOWN_MAGE_FIRST_ICON_CONTROL;
            message.payload.widget.data.value = m_town->m_spells[spellIndex];
            window->BroadcastMessage(message);
            message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
            message.payload.widget.id = spellIndex + TOWN_MAGE_FIRST_NAME_CONTROL;
            message.payload.widget.data.text = gSpellNames[m_town->m_spells[spellIndex]];
            window->BroadcastMessage(message);
        }
    }
    message.payload.widget.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.payload.widget.data.value = TOWN_MAGE_WIDGET_VISIBLE_FLAG;
    for (spellIndex = 1; spellIndex < TOWN_MAGE_TOWER_FRAME_COUNT; spellIndex++) {
        message.payload.widget.id = spellIndex + TOWN_MAGE_FIRST_TOWER_CONTROL;
        window->BroadcastMessage(message);
    }
    message.payload.widget.command = WIDGET_COMMAND_SET_FLAGS;
    message.payload.widget.data.value = TOWN_MAGE_WIDGET_VISIBLE_FLAG;
    for (spellIndex = 0; spellIndex < m_town->m_buildState; spellIndex++) {
        message.payload.widget.id = (spellIndex + 1) * 2 + TOWN_MAGE_FIRST_TOWER_CONTROL;
        window->BroadcastMessage(message);
    }
    message.payload.widget.id = m_town->m_buildState * 2 + TOWN_MAGE_FIRST_TOWER_CONTROL + 1;
    window->BroadcastMessage(message);
}

// Buka TOWNMGR.cpp:2735 MageGuildHandler; HoMM1 numbers spells 1-9 and
// icons 10-18 and bounds them by the guild level.
VA(0x0040d24b, 0x186)
short MageGuildHandler(struct tag_message &message)
{
    short firstSpell = TOWN_MAGE_FIRST_SPELL_CONTROL;
    short iconBase = TOWN_MAGE_FIRST_ICON_CONTROL;
    int quickView;
    int spellId;
    int mageLevel;
    int spellPos;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.payload.widget.command) {
            case WIDGET_NOTIFY_SELECT:
                quickView = message.payload.widget.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON;
                switch (message.payload.widget.id) {
                    case 1:
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                    case 9:
                        spellPos = message.payload.widget.id - TOWN_MAGE_FIRST_SPELL_CONTROL;
                        goto showSpell;
                    case 10:
                    case 11:
                    case 12:
                    case 13:
                    case 14:
                    case 15:
                    case 16:
                    case 17:
                    case 18:
                        spellPos = message.payload.widget.id - TOWN_MAGE_FIRST_ICON_CONTROL;
                    showSpell:
                        mageLevel = gpTownManager->m_town->m_buildState;
                        if ((mageLevel == 0 && spellPos > 2) || (mageLevel == 1 && spellPos > 4)
                            || (mageLevel == 2 && spellPos > 6))
                            return MESSAGE_DISPATCH_CONSUME;
                        spellId = gpTownManager->m_town->m_spells[spellPos];
                        NormalDialog(gSpellDesc[spellId], quickView ? 4 : 1, -1, -1, 8, spellId, -1, 0, -1);
                        return MESSAGE_DISPATCH_CONSUME;
                }
        }
    }
    return EventWindowHandler(message);
}

// donor PoL RVA 0x0001a783; preferred Buka symbol ?SetupThievesGuild@townManager@@QAEXPAVheroWindow@@H@Z
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.350822;margin=0.362412;shape=0.263;size=0.210;calls=0.163;strings=townwind.icn;alternate=pol20:void townManager::SetupThievesGuild(class heroWindow *, int)@0x0001a783
// Buka TOWNMGR.cpp:3328 SetupThievesGuild; HoMM1 only draws the ranking
// flags, with the category count taken from the number of guilds owned.
VA(0x0040d3d1, 0x2ec)
void townManager::SetupThievesGuild(class heroWindow *window, short categories)
{
    iconWidget *marker;
    short firstPlayer;
    int numThieves;
    short wUnusedRankX = THIEVES_RANK_FIRST_X;
    short iUnusedRankWidth = THIEVES_PLAYER_COLUMN_WIDTH;
    short top = THIEVES_FIRST_CATEGORY_Y;
    short rowSpacing = THIEVES_CATEGORY_ROW_HEIGHT;
    short frameBase = THIEVES_FLAG_FRAME_BASE;
    short pos;
    short lMarkWidth = THIEVES_RANK_ICON_WIDTH;
    short bIconHeight = THIEVES_RANK_ICON_HEIGHT;
    short bColWidth = THIEVES_PLAYER_WIDTH;
    signed char ranking[GAME_PLAYER_COUNT];
    short rank;
    short categoryIndex;
    long totals[GAME_PLAYER_COUNT];
    short startPos;
    short hi;
    short tied;

    if (categories == -1) {
        numThieves = gpGame->GetNumThievesGuilds(giCurPlayer);
        if (numThieves >= 4)
            categories = 8;
        else if (numThieves == 3)
            categories = 7;
        else if (numThieves == 2)
            categories = 5;
        else
            categories = 3;
    }
    if (categories > THIEVES_CATEGORY_COUNT)
        categories = THIEVES_CATEGORY_COUNT;
    for (categoryIndex = 0; categoryIndex < categories; categoryIndex++) {
        GetCategoryStats(categoryIndex, totals, ranking);
        SortStats(totals, ranking);
        firstPlayer = 0;
        hi = 0;
        for (rank = 0; rank < THIEVES_RANK_COUNT; rank++) {
            if (firstPlayer == gpGame->m_playerCount - gpGame->m_deadPlayerCount)
                break;
            tied = 1;
            while (hi + 1 < gpGame->m_playerCount
                   && totals[hi + 1] == totals[hi]) {
                tied++;
                hi++;
            }
            startPos = rank * THIEVES_PLAYER_COLUMN_WIDTH + THIEVES_RANK_FIRST_X
                    - (tied - 1) * THIEVES_TIE_CENTERING_STEP;
            for (pos = firstPlayer; !(pos > hi); pos++) {
                marker = new iconWidget(
                    (pos - firstPlayer) * THIEVES_RANK_ICON_WIDTH + startPos,
                    categoryIndex * THIEVES_CATEGORY_ROW_HEIGHT + THIEVES_FIRST_CATEGORY_Y,
                    THIEVES_RANK_ICON_WIDTH, THIEVES_RANK_ICON_HEIGHT, "townwind.icn",
                    gpGame->m_players[ranking[pos]].m_unknown11 + THIEVES_FLAG_FRAME_BASE,
                    0, -1, 0x10, 1);
                if (marker == 0)
                    MemError();
                window->AddWidget(marker, -1);
            }
            hi++;
            firstPlayer = hi;
        }
    }
}

// Buka TOWNMGR.cpp:3727-3833; HoMM1 has eight categories, sums three
// resources per row and counts obelisks through playerData.
VA(0x0040d6bd, 0x484)
void townManager::GetCategoryStats(signed char category, long *const stats,
                                   signed char *const order)
{
    short player;
    short townIndex;
    short index;
    long strength;
    short numTowns;
    short numCastles;
    hero *playerHero;
    town *theTown;

    for (player = 0; player < gpGame->m_playerCount; player++) {
        numTowns = 0;
        numCastles = 0;
        order[player] = player;
        if (gpGame->m_playerDead[player]) {
            stats[player] = TOWN_THIEVES_DEAD_PLAYER_STAT;
        } else {
            switch (category) {
                case THIEVES_CATEGORY_TOWNS:
                    for (townIndex = 0; townIndex < GAME_TOWN_COUNT; townIndex++) {
                        if (gpGame->m_castleRecs[townIndex].m_owner == player
                            && (gpGame->m_castleRecs[townIndex].m_buildings & TOWN_BUILDING_TENT_FLAG))
                            numTowns++;
                    }
                    stats[player] = numTowns;
                    break;
                case THIEVES_CATEGORY_CASTLES:
                    for (townIndex = 0; townIndex < GAME_TOWN_COUNT; townIndex++) {
                        if (gpGame->m_castleRecs[townIndex].m_owner == player
                            && (gpGame->m_castleRecs[townIndex].m_buildings & TOWN_BUILDING_CASTLE_FLAG))
                            numCastles++;
                    }
                    stats[player] = numCastles;
                    break;
                case THIEVES_CATEGORY_HEROES:
                    stats[player] = gpGame->m_players[player].m_heroCount;
                    break;
                case THIEVES_CATEGORY_GOLD:
                    stats[player] = gpGame->m_players[player].m_resources[RESOURCE_GOLD];
                    break;
                case THIEVES_CATEGORY_WOOD_AND_ORE:
                    stats[player] = gpGame->m_players[player].m_resources[RESOURCE_ORE]
                                    + gpGame->m_players[player].m_resources[RESOURCE_CRYSTAL]
                                    + gpGame->m_players[player].m_resources[RESOURCE_WOOD];
                    break;
                case THIEVES_CATEGORY_RARE_RESOURCES:
                    stats[player] = gpGame->m_players[player].m_resources[RESOURCE_MERCURY]
                                    + gpGame->m_players[player].m_resources[RESOURCE_SULFUR]
                                    + gpGame->m_players[player].m_resources[RESOURCE_GEMS];
                    break;
                case THIEVES_CATEGORY_OBELISKS:
                    stats[player] = gpGame->m_players[player].CountVisitedObelisks();
                    break;
                case THIEVES_CATEGORY_ARMY_STRENGTH:
                    strength = 0;
                    for (index = 0; index < gpGame->m_players[player].m_heroCount;
                         index++) {
                        playerHero = gpGame->GetHero(gpGame->m_players[player].m_heroIds[index]);
                        strength +=
                            gpPhilAI->FightValueOfStack(&playerHero->m_army, playerHero, 0, 0, 0);
                    }
                    for (index = 0; index < gpGame->m_players[player].m_townCount;
                         index++) {
                        theTown = gpGame->GetTown(gpGame->m_players[player].m_townIds[index]);
                        if (theTown->HasGarrison())
                            strength +=
                                gpPhilAI->FightValueOfStack(&theTown->m_army, 0, 0, 0, 0);
                    }
                    stats[player] = strength;
                    break;
            }
        }
    }
}

// Buka TOWNMGR.cpp:3843-3862 SortStats, a townManager member in HoMM1.
VA(0x0040db41, 0xea)
void townManager::SortStats(long *const stats, signed char *const order)
{
    long temp;
    short firstPlayer;
    short secondPlayer;
    signed char tempColor;

    for (firstPlayer = 0; firstPlayer < gpGame->m_playerCount - 1; firstPlayer++) {
        for (secondPlayer = firstPlayer + 1; secondPlayer < gpGame->m_playerCount; secondPlayer++) {
            if (stats[firstPlayer] < stats[secondPlayer]) {
                temp = stats[firstPlayer];
                stats[firstPlayer] = stats[secondPlayer];
                stats[secondPlayer] = temp;
                tempColor = order[firstPlayer];
                order[firstPlayer] = order[secondPlayer];
                order[secondPlayer] = tempColor;
            }
        }
    }
}

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
// Buka TOWNMGR.cpp:2968-3000; HoMM1 animates frames 1-8 of control 2.
VA(0x0040e5db, 0x155)
short TavernHandler(struct tag_message &message)
{
    int unusedDelay = TOWN_TAVERN_ANIMATION_DELAY;
    short unusedFrame = TOWN_TAVERN_UNUSED_FRAME;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.payload.widget.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.payload.widget.id) {
                    case TOWN_DIALOG_BUTTON_0:
                    case TOWN_DIALOG_BUTTON_1:
                    case TOWN_DIALOG_BUTTON_2:
                        gpWindowManager->m_dialogResult = message.payload.widget.id;
                        message.payload.widget.command = message.payload.widget.id =
                            WIDGET_COMMAND_DIALOG_SELECT;
                        return MESSAGE_DISPATCH_FORWARD;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    if (glTimers[0] < KBTickCount()) {
        message.type = MESSAGE_WIDGET;
        message.payload.widget.command = WIDGET_COMMAND_SET_FRAME;
        message.payload.widget.id = TOWN_TAVERN_ANIMATION_CONTROL;
        ++gpGame->m_viewArmyResult;
        message.payload.widget.data.value =
            gpGame->m_viewArmyResult % TOWN_TAVERN_ANIMATION_FRAME_COUNT
            + TOWN_TAVERN_FIRST_ANIMATION_FRAME;
        gpTownManager->m_heroWindow0->BroadcastMessage(message);
        gpTownManager->m_heroWindow0->MoveWindow(0, 0);
        glTimers[0] = KBTickCount() + TOWN_TAVERN_ANIMATION_DELAY;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// donor PoL RVA 0x00019d7c; preferred Buka symbol ?DoTavern@townManager@@QAEXXZ
// donor Buka TU SOURCE/TOWNMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.728216;margin=0.164549;shape=0.467;size=0.965;calls=0.889;strings=tavwin.bin;alternate=pol20:void townManager::DoTavern(void)@0x00019d7c
// Buka TOWNMGR.cpp:3003-3032; HoMM1 plays the tavern theme instead of a
// rumour and restores the town theme afterwards.
VA(0x0040e730, 0x136)
void townManager::DoTavern(void)
{
    int unusedValue = 0;

    m_heroWindow0 = new heroWindow(TOWN_TAVERN_WINDOW_X, TOWN_TAVERN_WINDOW_Y, "tavwin.bin");
    if (m_heroWindow0 == 0)
        MemError();
    SetWinText(m_heroWindow0, TOWN_TAVERN_WINDOW_TEXT);
    gpSoundManager->SwitchAmbientMusic(TOWN_TAVERN_MUSIC);
    gpWindowManager->DoDialog(m_heroWindow0, TavernHandler, 0);
    delete m_heroWindow0;
    gpSoundManager->SwitchAmbientMusic(townTheme[m_town->m_type] + TOWN_THEME_MUSIC_BASE);
}

// Buka TOWNMGR.cpp:3034 SplitArmyHandler; HoMM1 handles the amount
// buttons on selection and redraws the whole split window.
VA(0x0040ef8c, 0x32d)
short SplitArmyHandler(struct tag_message &message)
{
    short plusControl = TOWN_SPLIT_INCREASE_CONTROL;
    int unusedAction;
    short minusButton = TOWN_SPLIT_DECREASE_CONTROL;
    short amountText = TOWN_SPLIT_AMOUNT_CONTROL;
    int handled = 0;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.payload.widget.command) {
            case WIDGET_NOTIFY_SELECT:
                switch (message.payload.widget.id) {
                    case TOWN_SPLIT_INCREASE_CONTROL:
                        ++gpTownManager->m_splitAmount;
                        if (gpTownManager->m_splitAmount >= gpTownManager->m_splitMaximum)
                            gpTownManager->m_splitAmount = gpTownManager->m_splitMaximum - 1;
                        goto update_amount;
                    case TOWN_SPLIT_DECREASE_CONTROL:
                        --gpTownManager->m_splitAmount;
                        if (gpTownManager->m_splitAmount < 0)
                            gpTownManager->m_splitAmount = 0;
                        goto update_amount;
                    case TOWN_SPLIT_AMOUNT_CONTROL:
                        message.payload.widget.command = WIDGET_COMMAND_GET_TEXT;
                        gpTownManager->m_heroWindow1->BroadcastMessage(message);
                        gpTownManager->m_splitAmount = atoi(message.payload.widget.data.text);
                        if (gpTownManager->m_splitAmount < 0)
                            gpTownManager->m_splitAmount = 0;
                        if (gpTownManager->m_splitAmount >= gpTownManager->m_splitMaximum)
                            gpTownManager->m_splitAmount = gpTownManager->m_splitMaximum - 1;
                        goto update_amount;
                }
                break;
            case WIDGET_NOTIFY_DESELECT:
                switch (message.payload.widget.id) {
                    case TOWN_DIALOG_BUTTON_0:
                    case TOWN_DIALOG_BUTTON_1:
                        gpTownManager->m_splitAmount = 0;
                        gpWindowManager->m_dialogResult = message.payload.widget.id;
                        handled = 1;
                        break;
                    case TOWN_DIALOG_BUTTON_2:
                        if (gpTownManager->m_splitAmount == 0)
                            gpWindowManager->m_dialogResult = TOWN_DIALOG_BUTTON_1;
                        else
                            gpWindowManager->m_dialogResult = TOWN_DIALOG_BUTTON_2;
                        handled = 1;
                        break;
                }
                break;
            default:
                break;
        }
    }

    if (handled == 1) {
        message.payload.widget.command = message.payload.widget.id = WIDGET_COMMAND_DIALOG_SELECT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;

update_amount:
    sprintf(gText, "%d", gpTownManager->m_splitAmount);
    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = TOWN_SPLIT_AMOUNT_CONTROL;
    message.payload.widget.data.text = gText;
    gpTownManager->m_heroWindow1->BroadcastMessage(message);
    gpTownManager->m_heroWindow1->DrawWindow();
    return MESSAGE_DISPATCH_CONSUME;
}

// donor PoL RVA 0x0001e0fb; preferred Buka symbol ?CastleHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/Castle; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.424517;margin=0.478912;shape=0.381;size=0.626;calls=0.675;alternate=pol20:int CastleHandler(struct tag_message &)@0x0001e0fb
VA(0x0040e866, 0x726)
int CastleHandler(struct tag_message &) { return 0; }

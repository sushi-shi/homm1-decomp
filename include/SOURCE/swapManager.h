#ifndef HOMM1_SOURCE_SWAPMANAGER_H
#define HOMM1_SOURCE_SWAPMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/icon.h>
#include <BASE/message.h>
#include <Domains.h>
#include <H1/Macros.h>

// forward declarations:
class hero;
class heroWindow;
class icon;
struct tag_message;

// m_selectedSide/m_targetSide: the m_heroes index. DrawSelector draws side 1
// at the left army/artifact columns, so the left hero is index 1.
H1_ENUM_BEGIN(SwapManagerSide)
    SWAP_SIDE_NONE = -1,
    SWAP_SIDE_RIGHT = 0,
    SWAP_SIDE_LEFT = 1,
    SWAP_SIDE_COUNT = 2
H1_ENUM_END(SwapManagerSide)

// m_itemType: what the selection holds.
H1_ENUM_BEGIN(SwapManagerItemType)
    SWAP_ITEM_NONE = -1,
    SWAP_ITEM_ARMY = 0,
    SWAP_ITEM_ARTIFACT = 1
H1_ENUM_END(SwapManagerItemType)

#pragma pack(push, 1)
class swapManager : public baseManager {
public:
    heroWindow* m_window;
    icon* m_selectorIcon;
    // Main indexes the pair by side byte: [1] is the constructor's first
    // (left) hero, [0] the second.
    H1_ENUM_ARRAY(hero*, m_heroes, SwapManagerSide, SWAP_SIDE_COUNT);
    H1_ENUM_STORAGE(SwapManagerSide, i8) m_selectedSide;
    H1_ENUM_STORAGE(SwapManagerSide, i8) m_targetSide;
    i8 m_selectedSlot;
    i8 m_targetSlot;
    H1_ENUM_STORAGE(SwapManagerItemType, i8) m_itemType;
    i16 m_messageFilter;
    // --- constructors ---
    swapManager(void);
    swapManager(class hero* leftHero, class hero* rightHero);
    // --- virtual methods (vtable order) ---
    virtual H1_ENUM_RETURN(BaseManagerStatus, i16) Open(i16 id) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual H1_ENUM_RETURN(MessageDispatchResult, i16) Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void Reset(void);
    void DrawSelector(void);
    void ViewMon(void);
    void SwapArtifacts(void);
    void SwapMons(void);
    void Update(void);
    void SplitMons(void);
};
#pragma pack(pop)
// swapwin.bin widget ids. LEFT is the constructor's first hero,
// m_heroes[SWAP_SIDE_LEFT].
H1_ENUM_ID_BEGIN(SwapManagerControl)
CONTROL_LEFT_HERO = 65, CONTROL_RIGHT_HERO = 66, CONTROL_LEFT_PRIMARY_SKILL_FIRST = 67,
                        CONTROL_RIGHT_PRIMARY_SKILL_FIRST = 72, CONTROL_TITLE = 77,
                        CONTROL_LEFT_ARMY_FIRST = 78, CONTROL_LEFT_ARMY_LAST = 82,
                        CONTROL_RIGHT_ARMY_FIRST = 83, CONTROL_RIGHT_ARMY_LAST = 87,
                        CONTROL_LEFT_ARTIFACT_FIRST = 88, CONTROL_LEFT_ARTIFACT_LAST = 101,
                        CONTROL_RIGHT_ARTIFACT_FIRST = 102, CONTROL_RIGHT_ARTIFACT_LAST = 115,
                        CONTROL_LEFT_ARMY_COUNT_FIRST = 116,
                        CONTROL_RIGHT_ARMY_COUNT_FIRST = 121 H1_ENUM_ID_END(SwapManagerControl)

                        // m_selectedSlot/m_targetSlot with nothing picked; DrawSelector lays a
                        // hero's fourteen artifacts out in two columns of seven.
                        H1_ENUM_CONST_BEGIN(SwapManagerConstant)
    SWAP_SLOT_NONE = -1,
    SWAP_ARTIFACTS_PER_COLUMN = 7
H1_ENUM_CONST_END(SwapManagerConstant)

#endif // HOMM1_SOURCE_SWAPMANAGER_H

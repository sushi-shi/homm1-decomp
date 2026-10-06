#ifndef HOMM1_SOURCE_SWAPMANAGER_H
#define HOMM1_SOURCE_SWAPMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/icon.h>
#include <BASE/message.h>

class hero;
class heroWindow;
class icon;
struct tag_message;

enum SwapManagerSide {
    SWAP_SIDE_NONE = -1,
    SWAP_SIDE_RIGHT = 0,
    SWAP_SIDE_LEFT = 1,
    SWAP_SIDE_COUNT = 2
};

enum SwapManagerItemType {
    SWAP_ITEM_NONE = -1,
    SWAP_ITEM_ARMY = 0,
    SWAP_ITEM_ARTIFACT = 1
};

class swapManager : public baseManager {
public:
    heroWindow* m_window;
    icon* m_selectorIcon;
    hero* m_heroes[SWAP_SIDE_COUNT];
    i8 m_selectedSide;
    i8 m_targetSide;
    i8 m_selectedSlot;
    i8 m_targetSlot;
    i8 m_itemType;
    i16 m_messageFilter;
    swapManager(void);
    swapManager(class hero* leftHero, class hero* rightHero);
    virtual i16 Open(i16 id) ;
    virtual void Close(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void Reset(void);
    void DrawSelector(void);
    void ViewMon(void);
    void SwapArtifacts(void);
    void SwapMons(void);
    void Update(void);
    void SplitMons(void);
};
enum SwapManagerControl {
CONTROL_LEFT_HERO = 65, CONTROL_RIGHT_HERO = 66, CONTROL_LEFT_PRIMARY_SKILL_FIRST = 67,
                        CONTROL_RIGHT_PRIMARY_SKILL_FIRST = 72, CONTROL_TITLE = 77,
                        CONTROL_LEFT_ARMY_FIRST = 78, CONTROL_LEFT_ARMY_LAST = 82,
                        CONTROL_RIGHT_ARMY_FIRST = 83, CONTROL_RIGHT_ARMY_LAST = 87,
                        CONTROL_LEFT_ARTIFACT_FIRST = 88, CONTROL_LEFT_ARTIFACT_LAST = 101,
                        CONTROL_RIGHT_ARTIFACT_FIRST = 102, CONTROL_RIGHT_ARTIFACT_LAST = 115,
                        CONTROL_LEFT_ARMY_COUNT_FIRST = 116,
                        CONTROL_RIGHT_ARMY_COUNT_FIRST = 121 };

                        enum SwapManagerConstant {
    SWAP_SLOT_NONE = -1,
    SWAP_ARTIFACTS_PER_COLUMN = 7
};

#endif

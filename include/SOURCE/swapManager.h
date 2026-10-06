#ifndef HOMM1_SOURCE_SWAPMANAGER_H
#define HOMM1_SOURCE_SWAPMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/icon.h>

class hero;
class heroWindow;
class icon;
struct tag_message;

#pragma pack(push, 1)
class swapManager : public baseManager {
public:
    heroWindow* m_window;
    icon* m_selectorIcon;
    hero* m_heroes[2];
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
    i32 DrawSwapWin(void);
    void DrawSelector(void);
    void ViewMon(void);
    void SwapArtifacts(void);
    void SwapMons(void);
    void Update(void);
    void SplitMons(void);
};
#pragma pack(pop)
#endif

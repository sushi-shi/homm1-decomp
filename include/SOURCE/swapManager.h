#ifndef HOMM1_SOURCE_SWAPMANAGER_H
#define HOMM1_SOURCE_SWAPMANAGER_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 13 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <H1/Macros.h>

// forward declarations:
class hero;
class heroWindow;
class icon;
struct tag_message;

// The constructors store the vtable over baseManager and fill this packed
// tail; Reset chains the five selection bytes.
#pragma pack(push, 1)
class swapManager : public baseManager {
public:
    heroWindow* m_window;
    icon* m_selectorIcon;
    hero* m_rightHero;
    hero* m_leftHero;
    signed char m_selectedSide;
    signed char m_targetSide;
    signed char m_selectedSlot;
    signed char m_targetSlot;
    signed char m_itemType;
    short m_messageFilter;
    // --- constructors ---
    swapManager(void);
    swapManager(class hero*, class hero*);
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void Reset(void);
    int DrawSwapWin(void);
    void DrawSelector(void);
    void ViewMon(void);
    void SwapArtifacts(void);
    void SwapMons(void);
    void Update(void);
    void SplitMons(void);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_SWAPMANAGER_H

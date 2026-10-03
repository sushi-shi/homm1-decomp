#ifndef HOMM1_SOURCE_SWAPMANAGER_H
#define HOMM1_SOURCE_SWAPMANAGER_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 13 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <BASE/icon.h>
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
    // Main indexes the pair by side byte: [1] is the constructor's first
    // (left) hero, [0] the second.
    hero* m_heroes[2];
    i8 m_selectedSide;
    i8 m_targetSide;
    i8 m_selectedSlot;
    i8 m_targetSlot;
    i8 m_itemType;
    i16 m_messageFilter;
    // --- constructors ---
    swapManager(void);
    swapManager(class hero* leftHero, class hero* rightHero);
    // --- virtual methods (vtable order) ---
    virtual i16 Open(i16 id) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
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
#endif // HOMM1_SOURCE_SWAPMANAGER_H

#ifndef HOMM1_SOURCE_RECRUITUNIT_H
#define HOMM1_SOURCE_RECRUITUNIT_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 6 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <Domains.h>
#include <H1/Macros.h>

// forward declarations:
class armyGroup;
class heroWindow;
class town;
struct tag_message;

// m_sourceType: an event recruit (RecruitEvent) or a town dwelling; Close
// refreshes the town strips only for TOWN.
H1_ENUM_BEGIN(RecruitSourceType)
    RECRUIT_SOURCE_EVENT = -1,
    RECRUIT_SOURCE_TOWN = 0x28
H1_ENUM_END(RecruitSourceType)

// Both retail constructors, Open, Update and Main fix this packed layout.
#pragma pack(push, 1)
class recruitUnit : public baseManager {
public:
    H1_ENUM_STORAGE(RecruitSourceType, i8) m_sourceType;
    i8 m_creatureType;
    char m_padding32[4];
    i32 m_goldCost;
    i8 m_resourceType;
    i16 m_resourceCost;
    heroWindow* m_window;
    char m_padding41[4];
    armyGroup* m_army;
    i8 m_padding49;
    i8 m_recruited;
    i8 m_noRoom;
    i16* m_available;
    i16 m_maximum;
    i32 m_goldTotal;
    i16 m_resourceTotal;
    i16 m_quantity;
    // RecruitEvent allocates 0x5c bytes.
    char m_padding5a[2];
    // --- constructors ---
    recruitUnit(class armyGroup* army, i8 creatureType, i16* available);
    // HoMM1 has no refresh-town argument (retail ret 8).
    recruitUnit(class town* townData, i8 dwelling);
    // --- virtual methods (vtable order) ---
    virtual i16 Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void Update(void);
};
#pragma pack(pop)

void SetupRecruitWin(
    class heroWindow* window,
    i32 creatureType,
    i32 goldCost,
    i32 resourceType,
    i32 resourceCost,
    i32 available
);
void QuickViewRecruit(class town* townData, i8 dwelling);
// Moved from RECRUIT.cpp.
// Buka RecruitConstant (HoMM1 values): window and dialog positions and
// SetupRecruitWin's text buffers.
H1_ENUM_CONST_BEGIN(RecruitConstant)
    RECRUIT_WINDOW_X = 0xa0,
    RECRUIT_WINDOW_Y = 0x10,
    RECRUIT_VIEW_ARMY_X = 0x77,
    RECRUIT_VIEW_ARMY_Y = 0x20,
    RECRUIT_NO_ROOM_DIALOG_X = 0xb1,
    RECRUIT_NO_ROOM_DIALOG_Y = 0x64,
    RECRUIT_NAME_SIZE = 20,
    RECRUIT_LABEL_SIZE = 40
H1_ENUM_CONST_END(RecruitConstant)

// recruit0/1.bin and recruiq0/1.bin control ids (Buka RecruitControl).
H1_ENUM_BEGIN(RecruitControl)
    RECRUIT_CLOSE_CONTROL = DIALOG_BUTTON_0,
    RECRUIT_CANCEL_CONTROL = DIALOG_BUTTON_1,
    RECRUIT_CONFIRM_CONTROL = DIALOG_BUTTON_2,
    RECRUIT_TITLE_CONTROL = 0x40,
    RECRUIT_CREATURE_CONTROL = 0x42,
    RECRUIT_AVAILABLE_CONTROL = 0x43,
    RECRUIT_QUANTITY_CONTROL = 0x44,
    RECRUIT_INCREASE_CONTROL = 0x45,
    RECRUIT_DECREASE_CONTROL = 0x46,
    RECRUIT_MAXIMUM_CONTROL = 0x47,
    RECRUIT_GOLD_COST_CONTROL = 0x49,
    RECRUIT_RESOURCE_ICON_CONTROL = 0x4a,
    RECRUIT_RESOURCE_COST_CONTROL = 0x4b,
    RECRUIT_GOLD_TOTAL_CONTROL = 0x4d,
    RECRUIT_RESOURCE_IMAGE_CONTROL = 0x4e,
    RECRUIT_RESOURCE_TOTAL_CONTROL = 0x4f
H1_ENUM_END(RecruitControl)

#endif // HOMM1_SOURCE_RECRUITUNIT_H

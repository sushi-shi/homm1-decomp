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
    char m_unknown32[4];
    i32 m_goldCost;
    i8 m_resourceType;
    i16 m_resourceCost;
    heroWindow* m_window;
    char m_unknown41[4];
    armyGroup* m_army;
    i8 m_unknown49;
    i8 m_recruited;
    i8 m_noRoom;
    i16* m_available;
    i16 m_maximum;
    i32 m_goldTotal;
    i16 m_resourceTotal;
    i16 m_quantity;
    // RecruitEvent allocates 0x5c bytes.
    char m_unknown5a[2];
    // --- constructors ---
    recruitUnit(class armyGroup*, i32, i16*);
    // HoMM1 has no refresh-town argument (retail ret 8).
    recruitUnit(class town*, i8);
    // --- virtual methods (vtable order) ---
    virtual i16 Open(i16) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void Update(void);
};
#pragma pack(pop)

void SetupRecruitWin(class heroWindow*, i32, i32, i32, i32, i32);
void QuickViewRecruit(class town*, i8);
#endif // HOMM1_SOURCE_RECRUITUNIT_H

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
    H1_ENUM_STORAGE(RecruitSourceType, signed char) m_sourceType;
    signed char m_creatureType;
    char m_unknown32[4];
    int m_goldCost;
    signed char m_resourceType;
    short m_resourceCost;
    heroWindow* m_window;
    char m_unknown41[4];
    armyGroup* m_army;
    char m_unknown49;
    signed char m_recruited;
    signed char m_noRoom;
    short* m_available;
    short m_maximum;
    int m_goldTotal;
    short m_resourceTotal;
    short m_quantity;
    // RecruitEvent allocates 0x5c bytes.
    char m_unknown5a[2];
    // --- constructors ---
    recruitUnit(class armyGroup*, int, short int*);
    // HoMM1 has no refresh-town argument (retail ret 8).
    recruitUnit(class town*, signed char);
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void Update(void);
};
#pragma pack(pop)

void SetupRecruitWin(class heroWindow*, int, int, int, int, int);
void QuickViewRecruit(class town*, signed char);
#endif // HOMM1_SOURCE_RECRUITUNIT_H

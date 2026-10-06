#ifndef HOMM1_SOURCE_RECRUITUNIT_H
#define HOMM1_SOURCE_RECRUITUNIT_H

#include <BASE/baseManager.h>

class armyGroup;
class heroWindow;
class town;
struct tag_message;

enum RecruitSourceType {
    RECRUIT_SOURCE_EVENT = -1,
    RECRUIT_SOURCE_TOWN = 0x28
};

#pragma pack(push, 1)
class recruitUnit : public baseManager {
public:
    i8 m_sourceType;
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
    char m_unknown5a[2];
    recruitUnit(class armyGroup* army, i32 creatureType, i16* available);
    recruitUnit(class town* townData, i8 dwelling);
    virtual i16 Open(i16 priority) ;
    virtual void Close(void) ;
    virtual i16 Main(struct tag_message& message) ;
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
#endif

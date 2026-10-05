#ifndef HOMM1_SOURCE_BANKBOX_H
#define HOMM1_SOURCE_BANKBOX_H

#include <Domains.h>
#include <H1/Macros.h>

// bankbox.bin text ids: resource r's count at RESOURCE_FIRST + r, gold last.
H1_ENUM_BEGIN(BankBoxControl)
    BANK_BOX_RESOURCE_FIRST = 30,
    BANK_BOX_GOLD = 36
H1_ENUM_END(BankBoxControl)

// forward declarations:
class playerData;
class heroWindow;

class bankBox {
public:
    playerData* m_player;
    i16 m_x;
    i16 m_y;
    heroWindow* m_window;
    // --- constructors ---
    bankBox(i16 x, i16 y, class playerData* player);
    ~bankBox();
    // --- methods ---
    void Update(void);
};
#endif // HOMM1_SOURCE_BANKBOX_H

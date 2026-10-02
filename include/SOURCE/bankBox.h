#ifndef HOMM1_SOURCE_BANKBOX_H
#define HOMM1_SOURCE_BANKBOX_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 3 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>

// clang-format off
// bankbox.bin text ids: resource r's count at RESOURCE_FIRST + r, gold last.
H1_ENUM_BEGIN(BankBoxControl)
    BANK_BOX_RESOURCE_FIRST = 30,
    BANK_BOX_GOLD = 36
H1_ENUM_END(BankBoxControl)
    // clang-format on

// forward declarations:
class playerData;
class heroWindow;

class bankBox {
public:
    playerData* m_player;
    short m_x;
    short m_y;
    heroWindow* m_window;
    // --- constructors ---
    bankBox(short, short, class playerData*);
    ~bankBox();
    // --- methods ---
    // HoMM1 callers pass no argument (retail 0x00463e48).
    void Update(void);
};
#endif // HOMM1_SOURCE_BANKBOX_H

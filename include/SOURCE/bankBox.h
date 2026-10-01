#ifndef HOMM1_SOURCE_BANKBOX_H
#define HOMM1_SOURCE_BANKBOX_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 3 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>

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
    bankBox(int, int, class playerData*);
    ~bankBox();
    // --- methods ---
    // HoMM1 callers pass no argument (retail 0x00463e48).
    void Update(void);
};
#endif // HOMM1_SOURCE_BANKBOX_H

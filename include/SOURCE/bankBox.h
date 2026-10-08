#ifndef HOMM1_SOURCE_BANKBOX_H
#define HOMM1_SOURCE_BANKBOX_H

enum BankBoxControl {
    BANK_BOX_RESOURCE_FIRST = 30,
    BANK_BOX_GOLD = 36
};

class playerData;
class heroWindow;

class bankBox {
public:
    playerData* m_player;
    i16 m_x;
    i16 m_y;
    heroWindow* m_window;
    bankBox(i16 x, i16 y, class playerData* player);
    ~bankBox();
    void Update(void);
};
#endif

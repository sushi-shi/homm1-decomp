#ifndef HOMM1_BASE_DIALOG_H
#define HOMM1_BASE_DIALOG_H

#include <Domains.h>

// Reserved window-record button slots, as in HoMM2 Buka's BASE/dialog.h.
// heroWindowManager::DoDialog leaves the slot that closed a dialog in
// m_dialogResult; each dialog assigns its own meaning (EventWindowHandler
// closes on slots 0..3, 5 and 6, NormalDialog shows 1, 2, 5 and 6).
H1_ENUM_BEGIN(DialogButtonId)
    DIALOG_BUTTON_0 = 0x7800,
    DIALOG_BUTTON_1 = 0x7801,
    DIALOG_BUTTON_2 = 0x7802,
    DIALOG_BUTTON_3 = 0x7803,
    DIALOG_BUTTON_5 = 0x7805,
    DIALOG_BUTTON_6 = 0x7806
H1_ENUM_END(DialogButtonId)

#endif // HOMM1_BASE_DIALOG_H

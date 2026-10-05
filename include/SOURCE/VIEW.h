#ifndef HOMM1_SOURCE_VIEW_H
#define HOMM1_SOURCE_VIEW_H

#include <Domains.h>

struct tag_message;

// The combat general's stats window handler.
i16 HandleViewGeneral(struct tag_message& message);

// vgenwin.bin widget ids: name, portrait, colour and stats boxes, the Cast
// Spell / Retreat / Surrender buttons ViewGeneral disables and
// HandleViewGeneral returns, and the frame widgets the retail block names
// without using.
H1_ENUM_BEGIN(ViewGeneralControl)
    GENERAL_CONTROL_NONE = 0,
    GENERAL_NAME_WIDGET = 1,
    GENERAL_PORTRAIT_WIDGET = 2,
    GENERAL_COLOR_WIDGET = 3,
    GENERAL_STATS_WIDGET = 4,
    GENERAL_CONTROL_SEVEN = 7,
    GENERAL_CONTROL_EIGHT = 8,
    GENERAL_CONTROL_NINE = 9,
    GENERAL_CAST_SPELL = 10,
    GENERAL_RETREAT = 11,
    GENERAL_SURRENDER = 12,
    GENERAL_CONTROL_THIRTEEN = 13,
    GENERAL_CONTROL_FOURTEEN = 14
H1_ENUM_END(ViewGeneralControl)

// HandleViewGeneral's hover line: the gViewGeneralHelp row.
H1_ENUM_BEGIN(ViewGeneralHoverHelp)
    GENERAL_HOVER_HELP_CAST_SPELL = 1,
    GENERAL_HOVER_HELP_RETREAT = 2,
    GENERAL_HOVER_HELP_SURRENDER = 3,
    GENERAL_HOVER_HELP_CLOSE = 4,
    GENERAL_HOVER_HELP_HERO = 5
H1_ENUM_END(ViewGeneralHoverHelp)

#endif // HOMM1_SOURCE_VIEW_H

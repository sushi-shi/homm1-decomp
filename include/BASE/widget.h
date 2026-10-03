#ifndef HOMM1_BASE_WIDGET_H
#define HOMM1_BASE_WIDGET_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 7 methods, 3 own-virtual (all pure), 0 static data.
// Abstract root of the BASE UI-widget hierarchy. Verified from ??_7widget@@6B@: the
// vtable is 3 all-__purecall slots in order [Draw, ~widget, Main]. Draw is pure with
// NO body (emits no symbol); ~widget (??1widget@@UAE, 0x7) and Main (?Main@widget@@UAE,
// 0x2f4) are pure-virtual-WITH-body. Declaration order == vtable slot order; derived
// classes (border, iconWidget, textWidget, dimmerWidget, ...) override these 3 slots.

#include <Domains.h>
#include <H1/Macros.h>

class heroWindow;
struct tag_message;

// clang-format off
H1_ENUM_FLAGS_BEGIN(WidgetFlag, short)
    WIDGET_FLAG_SELECTED = 1,
    WIDGET_FLAG_ENABLED = 2,
    WIDGET_FLAG_DRAW = 4,
    WIDGET_FLAG_DIMMED = 8,
    WIDGET_FLAG_UPDATE = 0x4000
H1_ENUM_FLAGS_END(WidgetFlag)

H1_ENUM_BEGIN(WidgetKind)
    WIDGET_KIND_NONE = 0,
    // border kind 1: drawn without its background (Buka widgetKind.h).
    WIDGET_KIND_TRANSPARENT = 1,
    WIDGET_KIND_TEXT = 0x200,
    WIDGET_KIND_AUTO_REPEAT = 0x1000,
    WIDGET_KIND_TRACK_PRESS = 0x2000,
    WIDGET_KIND_TEXT_ENTRY = 0x4000
H1_ENUM_END(WidgetKind)

// widget::m_id of a widget that issues no command (decorative icons and text
// built into a window; Buka TOWN_WIDGET_ID_NONE).
H1_ENUM_CONST_BEGIN(WidgetIdConstant)
    WIDGET_ID_NONE = -1
H1_ENUM_CONST_END(WidgetIdConstant)
// clang-format on

#pragma pack(push, 1)
class widget /* abstract */ {
public:
    heroWindow* m_owner;
    widget* m_next;
    widget* m_prev;
    short m_id;
    short m_zOrder;
    short m_kind;
    short m_flags;
    short m_x;
    short m_y;
    short m_width;
    short m_height;

    // --- constructors ---
    widget(short int, short int, short int, short int, short int, short int);
    widget(void);
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) = 0;
    virtual ~widget(void) = 0;
    virtual short Main(struct tag_message&) = 0;
    // --- methods ---
    short Open(short, class heroWindow*);
    void Close(void);
    void Dim(void);
};
#pragma pack(pop)
#endif // HOMM1_BASE_WIDGET_H

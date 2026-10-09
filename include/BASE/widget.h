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

H1_ENUM_FLAGS_BEGIN(WidgetFlag, i16)
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

// (x, y), in the owner window's coordinates, lies inside widget w (Buka 2.1
// widget.h).
#define WIDGET_CONTAINS_LOCAL_POINT(w, x, y)                                                       \
    ((x) >= (w).m_x && (y) >= (w).m_y && (x) < (w).m_x + (w).m_width                               \
     && (y) < (w).m_y + (w).m_height)

// A widget record's four geometry words, read in order from the open resource
// (Buka 2.1 widget.h). VC4 rejects an assignment through (*this).member, so
// the widget is passed by pointer.
#define READ_WIDGET_GEOMETRY(w, resources)                                                         \
    do {                                                                                           \
        (w)->m_x = (resources)->ReadWord();                                                        \
        (w)->m_y = (resources)->ReadWord();                                                        \
        (w)->m_width = (resources)->ReadWord();                                                    \
        (w)->m_height = (resources)->ReadWord();                                                   \
    } while (0)

#pragma pack(push, 1)
class widget /* abstract */ {
public:
    heroWindow* m_owner;
    widget* m_next;
    widget* m_prev;
    i16 m_id;
    i16 m_zOrder;
    i16 m_kind;
    i16 m_flags;
    i16 m_x;
    i16 m_y;
    i16 m_width;
    i16 m_height;

    // --- constructors ---
    widget(i16 x, i16 y, i16 width, i16 height, i16 id, i16 kind);
    widget(void);
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) = 0;
    virtual ~widget(void) = 0;
    virtual i16 Main(struct tag_message& message) = 0;
    // --- methods ---
    i16 Open(i16 zOrder, class heroWindow* owner);
    void Close(void);
    void Dim(void);
};
#pragma pack(pop)
#endif // HOMM1_BASE_WIDGET_H

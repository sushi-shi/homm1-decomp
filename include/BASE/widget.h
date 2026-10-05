#ifndef HOMM1_BASE_WIDGET_H
#define HOMM1_BASE_WIDGET_H
// Abstract root of the BASE UI-widget hierarchy. The vtable is three __purecall
// slots in order [Draw, ~widget, Main]. Draw has no body; ~widget and Main are
// pure virtual with bodies. Derived classes (border, iconWidget, textWidget,
// dimmerWidget, ...) override these three slots.

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

H1_ENUM_CONST_BEGIN(WidgetFlagConstant)
    WIDGET_FLAG_MASK = 0xffff,
    // widget::widget(void)'s width and height.
    WIDGET_DEFAULT_EXTENT = 16
H1_ENUM_CONST_END(WidgetFlagConstant)

H1_ENUM_BEGIN(WidgetKind)
    WIDGET_KIND_NONE = 0,
    // border kind 1: drawn without its background.
    WIDGET_KIND_TRANSPARENT = 1,
    // widget::widget(void)'s kind.
    WIDGET_KIND_DEFAULT = 2,
    WIDGET_KIND_TEXT = 0x200,
    WIDGET_KIND_AUTO_REPEAT = 0x1000,
    WIDGET_KIND_TRACK_PRESS = 0x2000,
    WIDGET_KIND_TEXT_ENTRY = 0x4000
H1_ENUM_END(WidgetKind)

// widget::m_id of a widget that issues no command (decorative icons and text
// built into a window).
H1_ENUM_CONST_BEGIN(WidgetIdConstant)
    WIDGET_ID_NONE = -1
H1_ENUM_CONST_END(WidgetIdConstant)

// (x, y), in the owner window's coordinates, lies inside widget w.
#define WIDGET_CONTAINS_LOCAL_POINT(w, x, y)                                                       \
    ((x) >= (w).m_x && (y) >= (w).m_y && (x) < (w).m_x + (w).m_width                               \
     && (y) < (w).m_y + (w).m_height)

// A widget record's four geometry words, read in order from the open resource
// into the widget pointed to by w.
#define READ_WIDGET_GEOMETRY(w, resources)                                                         \
    ((w)->m_x = (resources)->ReadWord(),                                                           \
     (w)->m_y = (resources)->ReadWord(),                                                           \
     (w)->m_width = (resources)->ReadWord(),                                                       \
     (w)->m_height = (resources)->ReadWord())

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

#ifndef HOMM1_BASE_WIDGET_H
#define HOMM1_BASE_WIDGET_H

#include <BASE/heroWindow.h>
#include <BASE/message.h>

class heroWindow;
struct tag_message;

enum WidgetFlag {
    WIDGET_FLAG_SELECTED = 1,
    WIDGET_FLAG_ENABLED = 2,
    WIDGET_FLAG_DRAW = 4,
    WIDGET_FLAG_DIMMED = 8,
    WIDGET_FLAG_DIM_REQUEST = 0x1000,
    WIDGET_FLAG_UPDATE = 0x4000
};

enum WidgetFlagConstant {
    WIDGET_FLAG_MASK = 0xffff,
    WIDGET_DEFAULT_EXTENT = 16
};

enum WidgetKind {
    WIDGET_KIND_NONE = 0,
    WIDGET_KIND_TRANSPARENT = 1,
    WIDGET_KIND_DEFAULT = 2,
    WIDGET_KIND_TEXT = 0x200,
    WIDGET_KIND_AUTO_REPEAT = 0x1000,
    WIDGET_KIND_TRACK_PRESS = 0x2000,
    WIDGET_KIND_TEXT_ENTRY = 0x4000
};

enum WidgetIdConstant {
    WIDGET_ID_NONE = -1
};

#define WIDGET_CONTAINS_LOCAL_POINT(w, x, y)                                                       \
    ((x) >= (w).m_x && (y) >= (w).m_y && (x) < (w).m_x + (w).m_width                               \
     && (y) < (w).m_y + (w).m_height)

#define READ_WIDGET_GEOMETRY(w, resources)                                                         \
    ((w)->m_x = (resources)->ReadWord(),                                                           \
     (w)->m_y = (resources)->ReadWord(),                                                           \
     (w)->m_width = (resources)->ReadWord(),                                                       \
     (w)->m_height = (resources)->ReadWord())

class widget  {
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

    widget(i16 x, i16 y, i16 width, i16 height, i16 id, i16 kind);
    widget(void);
    virtual void Draw(void) = 0;
    virtual ~widget(void) = 0;
    virtual i16 Main(struct tag_message& message) = 0;
    i16 Open(i16 zOrder, class heroWindow* owner);
    void Close(void);
    void Dim(void);
};
#endif

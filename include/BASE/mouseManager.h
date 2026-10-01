#ifndef HOMM1_BASE_MOUSEMANAGER_H
#define HOMM1_BASE_MOUSEMANAGER_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 17 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <H1/Macros.h>

// forward declarations:
struct tag_message;
class bitmap;

#pragma pack(push, 1)
class mouseManager : public baseManager {
public:
    void* m_cursorResource;
    bitmap* m_savedUnderlying;
    void* m_cursorImage;
    short m_cursorFrame;
    short m_cursorReady;
    // Constructor and UpdateScreenRegion establish the packed tail.
    signed char m_pointerFlags;
    // CheckDoMain compares the pointer position less this offset with the
    // last drawn position at +0x5b/+0x5d.
    short m_hotspotX;
    short m_hotspotY;
    short m_mouseX;
    short m_mouseY;
    int m_unknown49;
    int m_unknown4d;
    char m_unknown51;
    char m_unknown52[9];
    short m_drawnX;
    short m_drawnY;

    // --- constructors ---
    mouseManager(void);
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void SetPointer(char*, int);
    void SetPointer(short);
    void NewUpdate(int);
    void MouseCoords(short&, short&);
    void SaveAndDraw(void);
    void RestoreUnderlying(void);
    void ReallyHidePointer(void);
    void ReallyShowPointer(void);
    void HideColorPointer(void);
    void ShowColorPointer(void);
    int IsVis(void) {
        return m_pointerFlags & 1;
    }
    void CheckUpdateMousePos(void);
    // Empty in the Windows build (retail 0x00476e20, `ret 8`).
    void MovePointer(short, short);
    // Empty Windows-build hooks around advManager::UpdateScreen's blit
    // (retail 0x00476e10 `ret 0x10` and 0x00476e00 `ret`).
    void BeginScreenUpdate(class bitmap*, short, short, short);
    void EndScreenUpdate(void);
    // Empty in the Windows build (retail 0x00476ec0, `ret 4`); the locator
    // knob drag passes 4 on entry and 6 on release.
    void SetCursorShape(int);
    void SetColorMice(int);
};
#pragma pack(pop)
#endif // HOMM1_BASE_MOUSEMANAGER_H

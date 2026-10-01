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
    unsigned char m_pointerFlags;
    int m_unknown41;
    short m_mouseX;
    short m_mouseY;
    int m_unknown49;
    int m_unknown4d;
    char m_unknown51;

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
    // HoMM1 Windows keeps the DOS buffer-pointer hooks as empty stubs.
    void SaveAndDraw(class bitmap*, int, int, int);
    void RestoreUnderlying(void);
    void ReallyHidePointer(void);
    void ReallyShowPointer(void);
    void HideColorPointer(void);
    void ShowColorPointer(void);
    int IsVis(void) {
        return m_pointerFlags & 1;
    }
    void CheckUpdateMousePos(void);
    void SetColorMice(int);
};
#pragma pack(pop)
#endif // HOMM1_BASE_MOUSEMANAGER_H

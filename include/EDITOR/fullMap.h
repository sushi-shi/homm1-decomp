#ifndef HOMM1_EDITOR_FULLMAP_H
#define HOMM1_EDITOR_FULLMAP_H

#include <EDITOR/mapcell.h>

// forward declarations:
class mapCell;
struct mapCellExtra;

class fullMap {
public:
    // --- members (offsets recovered from this+off accesses) ---
    mapCell* cells;       // +0x00
    mapCellExtra* extras; // +0x04
    i32 width;            // +0x08
    i32 height;           // +0x0c
    i32 extraCount;       // +0x10

    // --- constructors ---
    fullMap(void);
    ~fullMap();
    // --- methods ---
    void Close(void);
    void Init(i32, i32);
    void ClearCellExtra(i32);
    i32 GetNewCellExtraIndex(void);
    struct mapCellExtra* GetNewCellExtraOverlay(i32, i32);
    struct mapCellExtra* GetNewCellExtraObject(i32, i32);
    void Write(i32);
    void Read(i32, i32);
    void ChangeTilesetIndex(class mapCell*, i32, i32, i32, i32, i32, i32);

    // Inline accessors.
    mapCell* Row(i32 y) {
        return cells + width * y;
    } // row base ptr; caller does [x]
    mapCellExtra* Extra(i32 i) {
        return &extras[i];
    } // &extras[i] (stride 7)
};
#endif // HOMM1_EDITOR_FULLMAP_H

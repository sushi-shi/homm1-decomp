#ifndef HOMM1_EDITOR_OVERLAYMANAGER_H
#define HOMM1_EDITOR_OVERLAYMANAGER_H

// The object-placement tool (src/EDITOR/OVERLAY.cpp) and the placeable
// object table it shares with the random map generator.

#include <Domains.h>

H1_ENUM_CONST_BEGIN(EditObjectConstant)
// The generator scans this many gEditObjects records.
    EDIT_OBJECT_SCAN_COUNT = 295,
    // gEditObjects[].terrainMask: placeable on every terrain.
    EDIT_OBJECT_ANY_TERRAIN = 0xfe,
    EDIT_OBJECT_NAME_SIZE = 9
H1_ENUM_CONST_END(EditObjectConstant)

#pragma pack(push, 1)
// One placeable object: a padded eight-character name (an obelisk's digit at
// name[7]), its adventure tileset (MapTileset), how often the generator
// scatters it and a bit per terrain (1 << TerrainType) it may stand on.
struct editObject {
    char name[EDIT_OBJECT_NAME_SIZE];
    i8 tileset;
    u8 unknown0a;
    u16 frequency;
    u8 unknown0d[2];
    u8 terrainMask;
    u8 unknown10[29];
};
#pragma pack(pop)

extern editObject gEditObjects[];

// Whether `object` fits with its top-left cell at (x, y).
i16 CanPlaceObject(editObject* object, i16 x, i16 y);
// Writes `object` into the map at (x, y).
i32 PlaceObject(editObject* object, i16 x, i16 y);
// Writes `object` as overlays at (x, y).
i32 PlaceObjectOverlay(editObject* object, i16 x, i16 y, i32 checkFlags);

#endif // HOMM1_EDITOR_OVERLAYMANAGER_H

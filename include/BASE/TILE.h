#ifndef HOMM1_BASE_TILE_H
#define HOMM1_BASE_TILE_H

class bitmap;
class tileset;

// BASE/TILE.asm cdecl tile blitter (HoMM2 Buka TILE.h signature).
extern "C" void __cdecl TileToBitmap(tileset*, u32, bitmap*, i32, i32);

#endif

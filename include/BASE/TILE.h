#ifndef HOMM1_BASE_TILE_H
#define HOMM1_BASE_TILE_H

class bitmap;
class tileset;

// BASE/TILE.asm cdecl tile blitter.
extern "C" void __cdecl TileToBitmap(tileset* tiles, u32 tile, bitmap* dest, i32 x, i32 y);

#endif

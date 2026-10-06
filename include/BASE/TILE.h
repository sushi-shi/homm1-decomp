#ifndef HOMM1_BASE_TILE_H
#define HOMM1_BASE_TILE_H

class bitmap;
class tileset;

extern "C" void __cdecl TileToBitmap(tileset* tiles, u32 tile, bitmap* dest, i32 x, i32 y);

#endif

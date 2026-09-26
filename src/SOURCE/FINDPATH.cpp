#include <match.h>

#include <SOURCE/FINDPATH.h>

#include <H1/KB.h>
#include <SOURCE/searchArray.h>

#include <stdlib.h>
#include <string.h>

// Donor Clear; retail retains inline cells instead of HoMM2 heap storage.
VA(0x00424820, 0x23)
void searchArray::Clear(void) {
    memset(m_queue, 0, sizeof(m_queue));
    memset(m_cells, 0, sizeof(m_cells));
    m_pathLength = 0;
    m_queueCount = 0;
}

// Buka FINDPATH.cpp:83-88; retail retains short parameters/locals/return.
VA(0x00424850, 0x4c)
short searchArray::QuickDistance(short x1, short y1, short x2, short y2) {
    short xDistance = abs(x1 - x2);
    short yDistance = abs(y1 - y2);
    return ApproximateGridDistance(xDistance, yDistance);
}

// Donor CalcTerrainCost family; HoMM1 lacks pathfinding-skill/road dimensions.
VA(0x00424900, 0x4f)
int CalcTerrainCost(int terrain, int diagonal, int mobility, int waterMode) {
    int baseCost;
    if (waterMode == FINDPATH_WATER_MODE)
        terrain = FINDPATH_WATER_TERRAIN;
    if (diagonal == 0)
        return giTerrainCost[terrain][diagonal];
    if (giTerrainCost[terrain][1] <= mobility)
        return giTerrainCost[terrain][diagonal];
    baseCost = giTerrainCost[terrain][0];
    if (baseCost > mobility)
        baseCost = giTerrainCost[terrain][1];
    return baseCost;
}

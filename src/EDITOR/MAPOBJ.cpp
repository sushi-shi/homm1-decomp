// The random map generator: editManager methods that paint terrain, lay
// mountain and tree chains and place towns, objects and treasure. The unit
// name MAPOBJ is descriptive: no retail assertion names it.
// Descriptive names: every function and datum of this unit.

#include <match.h>

#include <BASE/miscwin.h>
#include <EDITOR/editManager.h>
#include <EDITOR/EDITOR.h>
#include <EDITOR/eventsManager.h>
#include <EDITOR/overlayManager.h>
#include <EDITOR/randomMap.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/KB.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/terrainTypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The cell after a chain link in each of the eight directions.
DATA(0x0044c0e4)
static i32 gChainSteps[CHAIN_DIRECTION_COUNT][MAP_STEP_AXES] =
    {{1, -2}, {1, -1}, {1, 2}, {1, 1}, {-1, 2}, {-1, 1}, {-1, -2}, {-1, -1}};

// A chain's sideways shift when it turns right ([0]) or left ([1]).
DATA(0x0044c124)
static i32 gChainTurns[CHAIN_DIRECTION_COUNT][2][MAP_STEP_AXES] = {
    {{1, 3}, {-1, 0}},
    {{1, 0}, {0, 0}},
    {{-1, 0}, {-3, 1}},
    {{0, 0}, {1, 0}},
    {{-1, -3}, {-1, 0}},
    {{0, 0}, {0, 0}},
    {{1, 0}, {3, -1}},
    {{0, 0}, {0, 0}}
};

DATA(0x0044c1a4)
static i32 gMineSiteKinds[5] = {2, 3, 4, 5, 6};

// No retail code reads this; it holds its retail .bss place, before
// GenerateRandomMap's empty status text.
DATA(0x00452ef4)
i32 gUnusedData452ef4;

VA(0x00410fa0, 0x3c5)
void editManager::GenerateRandomMap(void) {
    i32 attempt;
    i32 type;
    i32 done;
    i32 unusedTries;
    double unusedPercent;
    i32 canvas;
    double unusedRatio;

    if (!NewMapDialog()) {
        DrawMap();
        UpdateMapView();
        DrawRadar(1);
        return;
    }
    if (gSaveUnseen)
        gGeneratingMaps = 1;
    NewMap(1);
    done = 0;
    attempt = 0;
    while (!done && attempt < RANDOM_MAP_ATTEMPTS) {
        attempt++;
        ResetArea(0, 0, MAP_CELL_GRID_SIZE, MAP_CELL_GRID_SIZE);
        DrawMap();
        UpdateMapView();
        DrawRadar(1);
        unusedPercent = 100.0;
        canvas = 0;
        for (type = 0; type <= TERRAIN_LAST; type++) {
            if (gTerrainPercent[type] > 0.0) {
                PaintRandomTerrain(type, 100, 0);
                canvas = type + 1;
                type = 99;
            }
        }
        for (type = canvas; type <= TERRAIN_LAST; type++) {
            if (gTerrainPercent[type] > 0.0) {
                sprintf(
                    gText,
                    localization::Tr("editor.random.status.terrain"),
                    gGeneratorTerrainNames[type]
                );
                ShowStatusText(gText);
                PaintRandomTerrain(type, static_cast<i32>(gTerrainPercent[type]), canvas - 1);
            }
        }
        ShowStatusText(localization::Tr("editor.random.status.smoothing"));
        RemoveSmallRegions();
        for (type = 0; type <= TERRAIN_LAST; type++)
            BlendTerrain(type, 1, 0, 1, 0);
        BlendTerrain(TERRAIN_WATER, 1, 0, 0, 1);
        ShowStatusText(localization::Tr("editor.random.status.mountains"));
        PlaceObstacleChains(
            static_cast<i32>(gDensityPercent[GENERATOR_DENSITY_MOUNTAINS]),
            TILESET_MTN32
        );
        ShowStatusText(localization::Tr("editor.random.status.trees"));
        PlaceObstacleChains(
            static_cast<i32>(gDensityPercent[GENERATOR_DENSITY_TREES]),
            TILESET_TREE32
        );
        ShowStatusText(localization::Tr("editor.random.status.objects"));
        PlaceRandomObjects(
            static_cast<i32>(gDensityPercent[GENERATOR_DENSITY_OBJECTS]),
            static_cast<i32>(gDensityPercent[GENERATOR_DENSITY_MONSTERS])
        );
        ShowStatusText(localization::Tr("editor.random.status.land"));
        PlaceTowns();
        for (type = 0; type <= TERRAIN_LAST; type++)
            BlendTerrain(type, 1, 0, 1, 0);
        gVaryTiles = 1;
        BlendTerrain(TERRAIN_WATER, 1, 0, 0, 1);
        gVaryTiles = 0;
        ShowStatusText(localization::Tr("editor.random.status.treasure"));
        PlaceTreasures(
            static_cast<i32>(gDensityPercent[GENERATOR_DENSITY_TREASURE]),
            static_cast<i32>(gDensityPercent[GENERATOR_DENSITY_MONSTERS])
        );
        done = HasEnoughCastles();
        if (!done)
            continue;
        if (gGeneratingMaps) {
            ShowStatusText(localization::Tr("editor.random.status.save_prompt"));
            if (MapDetailsDialog(1) && !SaveMap(m_mapFileName)) {
                sprintf(gText, localization::Tr("editor.random.saved"), gMapHeader->name[0]);
                NormalDialog(gText, NORMAL_DIALOG_TYPE_OK);
            }
            ResetArea(0, 0, MAP_CELL_GRID_SIZE, MAP_CELL_GRID_SIZE);
            gGeneratingMaps = 0;
        }
    }
    DrawMap();
    ShowStatusText("");
    ClearStatusText();
    UpdateMapView();
    DrawRadar(1);
}

VA(0x00411365, 0xd5)
i32 editManager::HasEnoughCastles(void) {
    i32 count;
    i32 y;
    i32 x;
    mapCell* cell;

    count = 0;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map.cells[x][y];
            if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_CASTLE)
                || cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)
                       && (cell->m_objectIndex == RANDOM_MAP_KNIGHT_CASTLE_FRAME
                           || cell->m_objectIndex == RANDOM_MAP_BARBARIAN_CASTLE_FRAME
                           || cell->m_objectIndex == RANDOM_MAP_SORCERESS_CASTLE_FRAME
                           || cell->m_objectIndex == RANDOM_MAP_WARLOCK_CASTLE_FRAME))
                count++;
        }
    }
    return count >= RANDOM_MAP_MIN_CASTLES;
}

VA(0x0041143a, 0x601)
void editManager::PaintRandomTerrain(i32 terrain, i32 percent, i32 baseTerrain) {
    i32 perSeed;
    i32 walkX;
    i32 walkY;
    i32 sourceY;
    i32 guard;
    i32 total;
    i32 xBias;
    i32 reserve;
    i32 looking;
    i32 stepCount;
    i32 upperWeight;
    i32 yAxis;
    i32 balance;
    i32 escapes;
    i32 patches;
    i32 turnWeight;
    i32 lowerCap;
    i32 startX;
    i32 placed;
    i32 cluster;

    if (percent == RANDOM_MAP_FULL_PERCENT) {
        for (walkX = 0; walkX < MAP_CELL_GRID_SIZE; walkX++)
            for (walkY = 0; walkY < MAP_CELL_GRID_SIZE; walkY++)
                m_map.cells[walkX][walkY].m_tileIndex = terrain * MAP_CELL_TILES_PER_TERRAIN;
    } else {
        total = percent * (MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE) / 100;
        patches = Random(0, percent + 51) / 30 + 1;
        balance = total;
        escapes = 0;
        lowerCap = gScatterTowns ? 2 : 3;
        upperWeight = (gScatterTowns != 0) + 6;
        for (cluster = 0; cluster < patches; cluster++) {
            perSeed = balance / (patches - cluster);
            looking = 1;
            guard = 0;
            while (guard < 200 && looking) {
                guard++;
                if (gScatterTowns)
                    startX = Random(0, MAP_CELL_GRID_SIZE - 1);
                else
                    startX =
                        (Random(0, MAP_CELL_GRID_SIZE - 1) + Random(0, MAP_CELL_GRID_SIZE - 1)
                         + Random(0, MAP_CELL_GRID_SIZE - 1) + Random(0, MAP_CELL_GRID_SIZE - 1))
                        / 4;
                if (gScatterTowns) {
                    if (terrain == TERRAIN_DESERT || terrain == TERRAIN_LAVA)
                        sourceY =
                            (Random(0, MAP_CELL_GRID_SIZE - 1) + Random(0, MAP_CELL_GRID_SIZE - 1)
                             + Random(0, MAP_CELL_GRID_SIZE - 1))
                            / 3;
                    else if (terrain == TERRAIN_SNOW)
                        sourceY = Random(4, 20) + (Random(0, 1) ? 48 : 0);
                    else
                        sourceY = Random(0, MAP_CELL_GRID_SIZE - 1);
                } else {
                    sourceY =
                        (Random(0, MAP_CELL_GRID_SIZE - 1) + Random(0, MAP_CELL_GRID_SIZE - 1)
                         + Random(0, MAP_CELL_GRID_SIZE - 1) + Random(0, MAP_CELL_GRID_SIZE - 1))
                        / 4;
                }
                if (m_map.cells[startX][sourceY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                    == baseTerrain)
                    looking = 0;
            }
            xBias = Random(3, 7);
            turnWeight = Random(3, 7);
            yAxis = Random(3, 7);
            reserve = static_cast<i32>(perSeed * 1.5);
            stepCount = 0;
            for (placed = 0; placed < perSeed; placed++) {
                stepCount++;
                if ((stepCount & RANDOM_MAP_SEED_DRIFT_MASK) == RANDOM_MAP_SEED_DRIFT_MASK) {
                    startX = startX + Random(0, 2) - 1;
                    sourceY = sourceY + Random(0, 2) - 1;
                    if (startX < 0)
                        startX = 0;
                    if (startX >= MAP_CELL_GRID_SIZE)
                        startX = MAP_CELL_GRID_SIZE - 1;
                    if (sourceY < 0)
                        sourceY = 0;
                    if (sourceY >= MAP_CELL_GRID_SIZE)
                        sourceY = MAP_CELL_GRID_SIZE - 1;
                    if ((stepCount & RANDOM_MAP_WEIGHT_DRIFT_MASK)
                        == RANDOM_MAP_WEIGHT_DRIFT_MASK) {
                        xBias = xBias + Random(0, 2) - 1;
                        turnWeight = turnWeight + Random(0, 2) - 1;
                        yAxis = yAxis + Random(0, 2) - 1;
                        if (xBias < lowerCap)
                            xBias = lowerCap;
                        if (xBias > upperWeight)
                            xBias = upperWeight;
                        if (turnWeight < lowerCap)
                            turnWeight = lowerCap;
                        if (turnWeight > upperWeight)
                            turnWeight = upperWeight;
                        if (yAxis < lowerCap)
                            yAxis = lowerCap;
                        if (yAxis > upperWeight)
                            yAxis = upperWeight;
                    }
                }
                walkX = startX;
                walkY = sourceY;
                guard = 0;
                while (m_map.cells[walkX][walkY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN == terrain
                       && guard++ < 1000) {
                    if (Random(0, 9) < xBias) {
                        if (walkX == 0) {
                            walkX++;
                            if (turnWeight > upperWeight)
                                turnWeight = upperWeight;
                        } else if (walkX == MAP_CELL_GRID_SIZE - 1) {
                            walkX--;
                            if (turnWeight < lowerCap)
                                turnWeight = lowerCap;
                        } else if (Random(0, 9) < turnWeight)
                            walkX--;
                        else
                            walkX++;
                    } else {
                        if (walkY == 0) {
                            walkY++;
                            if (yAxis > upperWeight)
                                yAxis = upperWeight;
                        } else if (walkY == MAP_CELL_GRID_SIZE - 1) {
                            walkY--;
                            if (yAxis < lowerCap)
                                yAxis = lowerCap;
                        } else if (Random(0, 9) < yAxis)
                            walkY--;
                        else
                            walkY++;
                    }
                }
                if (guard >= 1000)
                    escapes++;
                if (m_map.cells[walkX][walkY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                    == baseTerrain)
                    m_map.cells[walkX][walkY].m_tileIndex = terrain * MAP_CELL_TILES_PER_TERRAIN;
                else if (reserve) {
                    reserve--;
                    placed--;
                } else {
                    balance++;
                    if (cluster + 1 == patches && patches < 20)
                        patches++;
                }
            }
            balance -= perSeed;
        }
    }
}

VA(0x00411a3b, 0x4bc)
void editManager::RemoveSmallRegions(void) {
    i32 fromX;
    i32 spread;
    i32 other;
    i8* done;
    i32 tallyX;
    i32 j;
    i32 ground;
    i32 extent;
    i32 landY;
    i32 homeY;
    i32 maxX;
    i32 startX;
    i32 y1;
    i8* inRegion;
    i32 n;
    i32 searchTop;

    done = static_cast<i8*>(malloc(MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE));
    inRegion = static_cast<i8*>(malloc(MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE));
    memset(done, 0, MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE);
    for (homeY = 0; homeY < MAP_CELL_GRID_SIZE; homeY++) {
        for (startX = 0; startX < MAP_CELL_GRID_SIZE; startX++) {
            if (*(done + startX + homeY * MAP_CELL_GRID_SIZE))
                continue;
            memset(inRegion, 0, MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE);
            (*(inRegion + startX + homeY * MAP_CELL_GRID_SIZE))++;
            ground = giGroundToTerrain[m_map.cells[startX][homeY].m_tileIndex];
            spread = 1;
            extent = 1;
            fromX = startX - 1;
            maxX = startX + 1;
            searchTop = homeY - 1;
            y1 = homeY + 1;
            other = -1;
            while (spread) {
                spread = 0;
                if (fromX < 0)
                    fromX = 0;
                if (maxX >= MAP_CELL_GRID_SIZE)
                    maxX = MAP_CELL_GRID_SIZE - 1;
                if (searchTop < 0)
                    searchTop = 0;
                if (y1 >= MAP_CELL_GRID_SIZE)
                    y1 = MAP_CELL_GRID_SIZE - 1;
                for (j = searchTop; j <= y1; j++) {
                    for (n = fromX; n <= maxX; n++) {
                        if (giGroundToTerrain[m_map.cells[n][j].m_tileIndex] != ground) {
                            if (other == -1)
                                other = giGroundToTerrain[m_map.cells[n][j].m_tileIndex];
                            continue;
                        }
                        if (*(inRegion + n + j * MAP_CELL_GRID_SIZE))
                            continue;
                        if (n < MAP_CELL_GRID_SIZE - 1
                            && *(inRegion + n + 1 + j * MAP_CELL_GRID_SIZE))
                            (*(inRegion + n + j * MAP_CELL_GRID_SIZE))++;
                        else if (n > 0 && *(inRegion + n - 1 + j * MAP_CELL_GRID_SIZE))
                            (*(inRegion + n + j * MAP_CELL_GRID_SIZE))++;
                        else if (j < MAP_CELL_GRID_SIZE - 1
                                 && *(inRegion + n + (j + 1) * MAP_CELL_GRID_SIZE))
                            (*(inRegion + n + j * MAP_CELL_GRID_SIZE))++;
                        else if (j > 0 && *(inRegion + n + (j - 1) * MAP_CELL_GRID_SIZE))
                            (*(inRegion + n + j * MAP_CELL_GRID_SIZE))++;
                        else
                            continue;
                        spread = 1;
                        extent++;
                        if (n == fromX)
                            fromX--;
                        if (n == maxX)
                            maxX++;
                        if (j == searchTop)
                            fromX--;
                        if (j == y1)
                            y1++;
                        if (fromX < 0)
                            fromX = 0;
                        if (maxX >= MAP_CELL_GRID_SIZE)
                            maxX = MAP_CELL_GRID_SIZE - 1;
                        if (searchTop < 0)
                            searchTop = 0;
                        if (y1 >= MAP_CELL_GRID_SIZE)
                            y1 = MAP_CELL_GRID_SIZE - 1;
                    }
                }
            }
            if (extent > 15)
                other = ground;
            for (j = 0; j < MAP_CELL_GRID_SIZE; j++) {
                for (n = 0; n < MAP_CELL_GRID_SIZE; n++) {
                    if (*(inRegion + n + j * MAP_CELL_GRID_SIZE)) {
                        *(done + n + j * MAP_CELL_GRID_SIZE) = 1;
                        m_map.cells[n][j].m_tileIndex = other * MAP_CELL_TILES_PER_TERRAIN;
                    }
                }
            }
        }
    }
    free(done);
    free(inRegion);
    gLandCellCount = 0;
    for (tallyX = 0; tallyX < MAP_CELL_GRID_SIZE; tallyX++)
        for (landY = 0; landY < MAP_CELL_GRID_SIZE; landY++)
            if (m_map.cells[tallyX][landY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN)
                gLandCellCount++;
}

VA(0x00411ef7, 0x62)
void ScaleByDensity(i32* count, i32 density) {
    i32 base;

    base = *count;
    *count = base * (base + 50) / 100;
    if (density < 50)
        *count = *count * (density + 50) / 100;
    else
        *count = *count * density / 50;
}

VA(0x00411f59, 0x3ce)
void editManager::PlaceObstacleChains(i32 density, i32 tileset) {
    i32 chance;
    i32 going;
    i32 direction;
    i32 unusedStep;
    i32 unusedMask;
    i32 ground;
    i32 budget;
    i32 rootX;
    char variety;
    i32 ridges;
    i32 placed;
    i32 landCells;
    i32 hunting;
    i32 rootY;

    placed = 0;
    variety = 0;
    ground = 0;
    landCells = 0;
    for (rootX = 0; rootX < MAP_CELL_GRID_SIZE; rootX++)
        for (rootY = 0; rootY < MAP_CELL_GRID_SIZE; rootY++)
            if (m_map.cells[rootX][rootY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN)
                landCells++;
    ridges = landCells / 40;
    ScaleByDensity(&ridges, density);
    budget = ridges * 13;
    while (placed < budget) {
        hunting = 1;
        while (hunting) {
            hunting = 0;
            rootX = Random(0, MAP_CELL_GRID_SIZE - 1);
            rootY = Random(0, MAP_CELL_GRID_SIZE - 1);
            ground = giGroundToTerrain[m_map.cells[rootX][rootY].m_tileIndex];
            if (tileset == TILESET_TREE32 && ground == TERRAIN_LAVA && Random(0, 100) < 80)
                hunting = 1;
            if (tileset == TILESET_TREE32 && ground == TERRAIN_DESERT && Random(0, 100) < 70)
                hunting = 1;
        }
        direction = Random(0, 3) * 2;
        if (Random(1, 100) <= 25)
            direction++;
        going = 1;
        variety = 0;
        if (tileset == TILESET_TREE32) {
            switch (Random(0, 2)) {
                case CHAIN_TREE_AUTUMN:
                    variety = 'a';
                    break;
                case CHAIN_TREE_PINE:
                    variety = 'p';
                    break;
                case CHAIN_TREE_DECIDUOUS:
                    variety = 't';
                    break;
            }
            if (ground == TERRAIN_DIRT) {
                if (rootY < MAP_CELL_GRID_SIZE / 2)
                    variety = 'p';
                else
                    variety = 'a';
            }
            if (ground == TERRAIN_GRASS) {
                if (rootY < MAP_CELL_GRID_SIZE / 2)
                    variety = 'p';
                else
                    variety = 't';
            }
            if (ground == TERRAIN_DESERT)
                variety = 'a';
            if (ground == TERRAIN_SNOW)
                variety = 'p';
            if (ground == TERRAIN_LAVA)
                variety = 't';
            if (ground == TERRAIN_SWAMP)
                variety = 't';
        }
        while (going) {
            if (PlaceChainLink(&rootX, &rootY, direction, tileset, variety)) {
                placed += 12;
                if (tileset == TILESET_TREE32) {
                    if (Random(1, 100) < (direction % 1 ? 30 : 10))
                        going = 0;
                } else {
                    if (Random(1, 100) < (direction % 1 ? 40 : 20))
                        going = 0;
                }
                if (tileset == TILESET_TREE32) {
                    if (Random(1, 100) < 35) {
                        if (direction < CHAIN_RIGHTWARD_END)
                            rootX += 2;
                        else
                            rootX -= 2;
                        chance = 0;
                    } else
                        chance = 50;
                } else
                    chance = 40;
                if (Random(1, 100) < chance) {
                    if (Random(0, 1)) {
                        rootX += gChainTurns[direction][0][MAP_STEP_X];
                        rootY += gChainTurns[direction][0][MAP_STEP_Y];
                        direction = (direction + 10) % 8;
                    } else {
                        rootX += gChainTurns[direction][1][MAP_STEP_X];
                        rootY += gChainTurns[direction][1][MAP_STEP_Y];
                        direction = (direction + 6) % 8;
                    }
                }
            } else {
                going = 0;
                placed++;
            }
        }
    }
}

VA(0x00412327, 0x303)
i32 editManager::PlaceChainLink(i32* x, i32* y, i32 direction, i32 tileset, char kind) {
    i32 frame;
    overlayType* generic;
    i32 ground;
    overlayType* specific;
    i32 n;
    i32 bit;

    if (*x < 0 || *x > MAP_CELL_GRID_SIZE - 1 || *y < 0 || *y > MAP_CELL_GRID_SIZE - 1)
        return 0;
    frame = 0;
    ground = giGroundToTerrain[m_map.cells[*x][*y].m_tileIndex];
    bit = -1;
    if (tileset == TILESET_MTN32) {
        switch (ground) {
            case TERRAIN_GRASS:
                bit = 2;
                break;
            case TERRAIN_SNOW:
                bit = 4;
                break;
            case TERRAIN_SWAMP:
                bit = 8;
                break;
            case TERRAIN_DESERT:
                bit = 0x20;
                break;
            case TERRAIN_DIRT:
                bit = 0x40;
                break;
        }
    }
    if (tileset == TILESET_TREE32) {
        switch (ground) {
            case TERRAIN_SNOW:
                bit = 4;
                break;
        }
    }
    if (direction == CHAIN_UP_RIGHT_STEEP)
        frame = 0;
    if (direction == CHAIN_UP_RIGHT)
        frame = 1;
    if (direction == CHAIN_DOWN_RIGHT_STEEP)
        frame = 2;
    if (direction == CHAIN_DOWN_RIGHT)
        frame = 3;
    if (direction == CHAIN_DOWN_LEFT_STEEP)
        frame = 0;
    if (direction == CHAIN_DOWN_LEFT)
        frame = 1;
    if (direction == CHAIN_UP_LEFT_STEEP)
        frame = 2;
    if (direction == CHAIN_UP_LEFT)
        frame = 3;
    specific = NULL;
    generic = NULL;
    for (n = 0; n < OVERLAY_TYPE_COUNT; n++) {
        if (!specific && gOverlayTypes[n].tileset == tileset && gOverlayTypes[n].terrainMask == bit)
            specific = &gOverlayTypes[n + frame];
        if (!generic && gOverlayTypes[n].tileset == tileset
            && gOverlayTypes[n].terrainMask == RANDOM_MAP_ANY_TERRAIN
            && (!kind || kind == gOverlayTypes[n].name[0]))
            generic = &gOverlayTypes[n + frame];
    }
    if (specific && CanPlaceOverlay(specific, *x, *y)) {
        PlaceOverlay(specific, *x, *y);
        *x += gChainSteps[direction][MAP_STEP_X];
        *y += gChainSteps[direction][MAP_STEP_Y];
        return 1;
    }
    if (CanPlaceOverlay(generic, *x, *y)) {
        PlaceOverlay(generic, *x, *y);
        *x += gChainSteps[direction][MAP_STEP_X];
        *y += gChainSteps[direction][MAP_STEP_Y];
        return 1;
    }
    return 0;
}

VA(0x0041262a, 0x1f4c)
void editManager::PlaceTowns(void) {
    i32 terrain;
    i32 cutOff[RANDOM_MAP_CASTLE_SLOTS];
    i32 extraRoads[RANDOM_MAP_CASTLE_SLOTS];
    i32 nearX;
    i32 belongs[RANDOM_MAP_CASTLE_SLOTS];
    i32 tileX;
    i32 reachable[RANDOM_MAP_CASTLE_SLOTS];
    i32 regionId;
    double shareValue[RANDOM_MAP_REGION_LIMIT];
    i32 endY;
    overlayType* anyGate;
    i32 coastAt;
    i32 destX;
    i32 continents;
    mapStep keeps[RANDOM_MAP_CASTLE_SLOTS];
    i32 tracing;
    i32 fromX;
    i32 roadMaskSet;
    overlayType* winterGate;
    i32 regionsUsed;
    i32 c;
    i32 round;
    i32 unusedValue;
    i16 rank[RANDOM_MAP_REGION_LIMIT + 1];
    i32 dist;
    i32 peerIndex;
    i32 steps;
    overlayType* castle;
    i32 t;
    i32 rating;
    i32 slot;
    i32 tileY;
    i32 unusedIndex;
    i32 filled;
    i32 stepX;
    i32 stepY;
    u8* regionGrid;
    i32 y0;
    i16 regionSizes[RANDOM_MAP_REGION_LIMIT];
    i32 meet;
    i32 foundY;
    i32 top;
    overlayType* desertGateLiths;
    i32 freeX;
    i32 scanY;
    i32 unusedTotal;
    u8* reachedGrids[RANDOM_MAP_CASTLE_SLOTS];

    for (slot = 0; slot < RANDOM_MAP_CASTLE_SLOTS; slot++) {
        cutOff[slot] = 0;
        extraRoads[slot] = 0;
    }
    castle = NULL;
    winterGate = NULL;
    desertGateLiths = NULL;
    anyGate = NULL;
    for (slot = 0; slot < OVERLAY_TYPE_COUNT; slot++) {
        if (!strcmpi(gOverlayTypes[slot].name, "xcast   "))
            castle = &gOverlayTypes[slot];
        if (!strcmpi(gOverlayTypes[slot].name, "stgate  "))
            winterGate = &gOverlayTypes[slot];
        if (!strcmpi(gOverlayTypes[slot].name, "dtgate  "))
            desertGateLiths = &gOverlayTypes[slot];
        if (!strcmpi(gOverlayTypes[slot].name, "xtgate  "))
            anyGate = &gOverlayTypes[slot];
    }
    regionGrid = static_cast<u8*>(malloc(MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE));
    memset(regionGrid, 0, MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE);
    for (slot = 0; slot < RANDOM_MAP_CASTLE_SLOTS; slot++) {
        reachedGrids[slot] = static_cast<u8*>(malloc(MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE));
        memset(reachedGrids[slot], 0, MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE);
    }
    continents = 0;
    for (regionId = 1; regionId < RANDOM_MAP_REGION_LIMIT; regionId++) {
        freeX = foundY = -1;
        for (tileX = 0; tileX < MAP_CELL_GRID_SIZE; tileX++) {
            for (tileY = 0; tileY < MAP_CELL_GRID_SIZE; tileY++) {
                if (m_map.cells[tileX][tileY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                    && !*(regionGrid + tileX + tileY * MAP_CELL_GRID_SIZE)) {
                    continents++;
                    freeX = tileX;
                    foundY = tileY;
                    *(regionGrid + tileX + tileY * MAP_CELL_GRID_SIZE) = regionId;
                    tileX = tileY = 999;
                }
            }
        }
        if (freeX >= 0) {
            filled = 1;
            while (filled) {
                filled = 0;
                for (tileX = 0; tileX < MAP_CELL_GRID_SIZE; tileX++) {
                    for (tileY = 0; tileY < MAP_CELL_GRID_SIZE; tileY++) {
                        if (m_map.cells[tileX][tileY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                            && !*(regionGrid + tileX + tileY * MAP_CELL_GRID_SIZE)) {
                            if (tileX > 0
                                && *(regionGrid + tileX - 1 + tileY * MAP_CELL_GRID_SIZE) > 0)
                                *(regionGrid + tileX + tileY * MAP_CELL_GRID_SIZE) =
                                    *(regionGrid + tileX - 1 + tileY * MAP_CELL_GRID_SIZE);
                            else if (tileX < MAP_CELL_GRID_SIZE - 1
                                     && *(regionGrid + tileX + 1 + tileY * MAP_CELL_GRID_SIZE) > 0)
                                *(regionGrid + tileX + tileY * MAP_CELL_GRID_SIZE) =
                                    *(regionGrid + tileX + 1 + tileY * MAP_CELL_GRID_SIZE);
                            else if (tileY > 0
                                     && *(regionGrid + tileX + (tileY - 1) * MAP_CELL_GRID_SIZE)
                                            > 0)
                                *(regionGrid + tileX + tileY * MAP_CELL_GRID_SIZE) =
                                    *(regionGrid + tileX + (tileY - 1) * MAP_CELL_GRID_SIZE);
                            else if (tileY < MAP_CELL_GRID_SIZE - 1
                                     && *(regionGrid + tileX + (tileY + 1) * MAP_CELL_GRID_SIZE)
                                            > 0)
                                *(regionGrid + tileX + tileY * MAP_CELL_GRID_SIZE) =
                                    *(regionGrid + tileX + (tileY + 1) * MAP_CELL_GRID_SIZE);
                            if (*(regionGrid + tileX + tileY * MAP_CELL_GRID_SIZE))
                                filled = 1;
                        }
                    }
                }
            }
        } else
            regionId = 999;
    }
    memset(regionSizes, 0, sizeof(regionSizes));
    for (tileX = 0; tileX < MAP_CELL_GRID_SIZE; tileX++)
        for (tileY = 0; tileY < MAP_CELL_GRID_SIZE; tileY++)
            regionSizes[*(regionGrid + tileX + tileY * MAP_CELL_GRID_SIZE)]++;
    for (slot = 0; slot < RANDOM_MAP_REGION_LIMIT; slot++)
        shareValue[slot] = 0.0;
    for (slot = 1; slot <= continents; slot++) {
        rank[slot] = slot;
        shareValue[slot] =
            static_cast<float>(regionSizes[slot]) / (static_cast<float>(gLandCellCount)) * 100.0f;
    }
    for (round = 1; round < continents; round++) {
        for (slot = round; slot < continents; slot++) {
            if (regionSizes[rank[slot]] < regionSizes[rank[slot + 1]]) {
                t = rank[slot];
                rank[slot] = rank[slot + 1];
                rank[slot + 1] = t;
            }
        }
    }
    if (shareValue[rank[1]] > 80.0) {
        belongs[0] = belongs[1] = belongs[2] = belongs[3] = rank[1];
        regionsUsed = 1;
    } else if (shareValue[rank[1]] > 40.0 && shareValue[rank[2]] < 15.0) {
        belongs[0] = belongs[1] = belongs[2] = belongs[3] = rank[1];
        regionsUsed = 1;
    } else if (shareValue[rank[1]] < 55.0 && shareValue[rank[2]] > 25.0) {
        belongs[0] = belongs[1] = rank[1];
        belongs[2] = belongs[3] = rank[2];
        regionsUsed = 2;
    } else if (shareValue[rank[1]] < 50.0 && shareValue[rank[2]] > 15.0
               && shareValue[rank[3]] > 15.0 && shareValue[rank[4]] > 15.0) {
        belongs[0] = rank[1];
        belongs[1] = rank[2];
        belongs[2] = rank[3];
        belongs[3] = rank[4];
        regionsUsed = 4;
    } else if (shareValue[rank[1]] < 30.0 && shareValue[rank[2]] > 8.0 && shareValue[rank[3]] > 8.0
               && shareValue[rank[4]] > 8.0) {
        belongs[0] = rank[1];
        belongs[1] = rank[2];
        belongs[2] = rank[3];
        belongs[3] = rank[4];
        regionsUsed = 4;
    } else {
        belongs[0] = belongs[1] = belongs[2] = belongs[3] = rank[1];
        regionsUsed = 1;
    }
    ShowStatusText(localization::Tr("editor.random.status.castles"));
    for (c = 0; c < RANDOM_MAP_CASTLE_SLOTS; c++) {
        top = 0;
        for (tileX = 2; tileX < MAP_CELL_GRID_SIZE - 3; tileX++) {
            for (tileY = 4; tileY < MAP_CELL_GRID_SIZE - 3; tileY++) {
                if (*(regionGrid + tileX + tileY * MAP_CELL_GRID_SIZE) == belongs[c]) {
                    rating = Random(1000, 1200);
                    if (m_map.cells[tileX - 1][tileY + 1].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)
                        rating += regionsUsed > 1 ? 1000 : continents * 50 + 100;
                    else if (m_map.cells[tileX - 2][tileY + 2].m_tileIndex
                             < MAP_CELL_TILES_PER_TERRAIN)
                        rating += regionsUsed > 1 ? 1000 : continents * 40 + 100;
                    for (nearX = tileX - 2; nearX <= tileX + 1; nearX++)
                        for (scanY = tileY - 2; scanY <= tileY; scanY++)
                            if (*(regionGrid + nearX + scanY * MAP_CELL_GRID_SIZE) == belongs[c])
                                rating += 50;
                    if (rating > top) {
                        for (nearX = 0; nearX < MAP_CELL_GRID_SIZE - 1; nearX++) {
                            for (scanY = 0; scanY < MAP_CELL_GRID_SIZE - 1; scanY++) {
                                if (m_map.cells[nearX][scanY].m_objectTileset == TILESET_TOWN32
                                    && m_map.cells[nearX][scanY].m_triggerType
                                           & MAP_TRIGGER_EVENT) {
                                    dist = abs(nearX - tileX) + abs(scanY - tileY);
                                    if (m_map.cells[nearX][scanY].m_triggerType
                                        == (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_CASTLE)) {
                                        if (dist < 10)
                                            rating -= 3000;
                                        else if (dist < 40)
                                            rating -= (40 - dist) * 15;
                                    } else {
                                        if (dist < 8)
                                            rating -= 3000;
                                        else if (dist < 40)
                                            rating -= (40 - dist) * 7;
                                    }
                                }
                            }
                        }
                    }
                    if (rating > top) {
                        keeps[c].x = tileX;
                        keeps[c].y = tileY;
                        top = rating;
                    }
                }
            }
        }
        tileX = keeps[c].x;
        tileY = keeps[c].y;
        if (top < 0)
            ShutDown(localization::Tr("editor.random.castles.failed"));
        terrain = m_map.cells[tileX][tileY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN;
        gEditManager->ClearArea(tileX - 2, tileY - 2, 5, 4, EDIT_CLEAR_ALL, 1);
        for (nearX = tileX - 2; nearX <= tileX + 2; nearX++)
            for (scanY = tileY - 2; scanY <= tileY + 1; scanY++)
                m_map.cells[nearX][scanY].m_tileIndex = terrain * MAP_CELL_TILES_PER_TERRAIN;
        PlaceOverlay(castle, tileX - 2, tileY);
        if (regionsUsed > 1)
            steps = 999;
        else if (m_map.cells[tileX - 1][tileY + 1].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)
            steps = 1;
        else if (m_map.cells[tileX - 2][tileY + 2].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)
            steps = 3;
        else
            steps = 0;
        if (steps) {
            coastAt = 0;
            nearX = tileX - 2;
            scanY = tileY + 1;
            ResetArea(tileX - 2, tileY + 1, 2, 2);
            while (!coastAt && steps) {
                steps--;
                if (m_map.cells[nearX][scanY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                        == TERRAIN_WATER
                    || m_map.cells[nearX + 1][scanY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                           == TERRAIN_WATER)
                    coastAt = 1;
                else {
                    m_map.cells[nearX][scanY].m_tileIndex = 0;
                    m_map.cells[nearX + 1][scanY].m_tileIndex = 0;
                    if (nearX > 0)
                        nearX--;
                    if (scanY < MAP_CELL_GRID_SIZE - 1)
                        scanY++;
                    if (nearX == 0 && scanY == MAP_CELL_GRID_SIZE - 1)
                        coastAt = 1;
                }
            }
        }
        for (peerIndex = 0; peerIndex < c; peerIndex++) {
            if (belongs[c] != belongs[c - 1])
                continue;
            if (keeps[peerIndex].x < keeps[c].x && keeps[peerIndex].y < keeps[c].y) {
                fromX = tileX - 2;
                y0 = tileY - 2;
                destX = keeps[peerIndex].x + 1;
                endY = keeps[peerIndex].x + 1;
            } else if (keeps[peerIndex].x < keeps[c].x && keeps[peerIndex].y >= keeps[c].y) {
                fromX = tileX + 1;
                y0 = tileY + 1;
                destX = keeps[peerIndex].x + 1;
                endY = keeps[peerIndex].x - 2;
            } else if (keeps[peerIndex].x >= keeps[c].x && keeps[peerIndex].y < keeps[c].y) {
                fromX = tileX + 1;
                y0 = tileY - 2;
                destX = keeps[peerIndex].x + 1;
                endY = keeps[peerIndex].x + 1;
            } else {
                fromX = tileX + 1;
                y0 = tileY + 1;
                destX = keeps[peerIndex].x - 1;
                endY = keeps[peerIndex].x - 2;
            }
            nearX = fromX;
            scanY = y0;
            tracing = 1;
            while (tracing) {
                if (nearX == destX && scanY == endY)
                    tracing = 0;
                if (destX > nearX)
                    stepX = 1;
                else if (destX < nearX)
                    stepX = -1;
                else
                    stepX = 0;
                if (endY > scanY)
                    stepY = 1;
                else if (endY < scanY)
                    stepY = -1;
                else
                    stepY = 0;
                if (stepX
                    && m_map.cells[nearX + stepX][scanY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN)
                    stepY = 0;
                else if (stepY
                         && m_map.cells[nearX][scanY + stepY].m_tileIndex
                                >= MAP_CELL_TILES_PER_TERRAIN)
                    stepX = 0;
                else {
                    tracing = 0;
                    cutOff[c] = 1;
                    cutOff[peerIndex] = 1;
                }
                if (tracing
                    && m_map.cells[nearX + stepX][scanY + stepY].m_tileIndex
                           >= MAP_CELL_TILES_PER_TERRAIN) {
                    nearX += stepX;
                    scanY += stepY;
                    roadMaskSet = RANDOM_MAP_ROAD_CLEAR_MASK;
                    gEditManager->ClearArea(nearX, scanY, 1, 1, roadMaskSet, 0);
                }
            }
        }
    }
    if (Random(0, 100) < 50) {
        extraRoads[0] = 1;
        extraRoads[1] = 1;
        if (Random(0, 100) < 50)
            extraRoads[2] = 1;
        if (Random(0, 100) < 50)
            extraRoads[3] = 1;
    }
    if (cutOff[0] || cutOff[1] || cutOff[2] || cutOff[3] || extraRoads[0]) {
        ShowStatusText(localization::Tr("editor.random.status.roads"));
        for (slot = 0; slot < RANDOM_MAP_CASTLE_SLOTS; slot++) {
            cutOff[slot] = 0;
            reachable[slot] = 0;
            for (tileX = keeps[slot].x - 2; tileX <= keeps[slot].x + 2; tileX++)
                for (tileY = keeps[slot].y - 2; tileY <= keeps[slot].y + 1; tileY++)
                    *(reachedGrids[slot] + tileX + tileY * MAP_CELL_GRID_SIZE) = 1;
            filled = 1;
            while (filled) {
                filled = 0;
                for (tileX = 0; tileX < MAP_CELL_GRID_SIZE; tileX++) {
                    for (tileY = 0; tileY < MAP_CELL_GRID_SIZE; tileY++) {
                        if (m_map.cells[tileX][tileY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                            && (!m_map.cells[tileX][tileY].m_objectTileset
                                || m_map.cells[tileX][tileY].m_objectTileset == TILESET_MONS32)
                            && !*(reachedGrids[slot] + tileX + tileY * MAP_CELL_GRID_SIZE)) {
                            if (tileX > 0
                                && *(reachedGrids[slot] + tileX - 1 + tileY * MAP_CELL_GRID_SIZE)
                                       > 0)
                                *(reachedGrids[slot] + tileX + tileY * MAP_CELL_GRID_SIZE) =
                                    *(reachedGrids[slot] + tileX - 1 + tileY * MAP_CELL_GRID_SIZE);
                            else if (tileX < MAP_CELL_GRID_SIZE - 1
                                     && *(reachedGrids[slot] + tileX + 1
                                          + tileY * MAP_CELL_GRID_SIZE)
                                            > 0)
                                *(reachedGrids[slot] + tileX + tileY * MAP_CELL_GRID_SIZE) =
                                    *(reachedGrids[slot] + tileX + 1 + tileY * MAP_CELL_GRID_SIZE);
                            else if (tileY > 0
                                     && *(reachedGrids[slot] + tileX
                                          + (tileY - 1) * MAP_CELL_GRID_SIZE)
                                            > 0)
                                *(reachedGrids[slot] + tileX + tileY * MAP_CELL_GRID_SIZE) =
                                    *(reachedGrids[slot] + tileX
                                      + (tileY - 1) * MAP_CELL_GRID_SIZE);
                            else if (tileY < MAP_CELL_GRID_SIZE - 1
                                     && *(reachedGrids[slot] + tileX
                                          + (tileY + 1) * MAP_CELL_GRID_SIZE)
                                            > 0)
                                *(reachedGrids[slot] + tileX + tileY * MAP_CELL_GRID_SIZE) =
                                    *(reachedGrids[slot] + tileX
                                      + (tileY + 1) * MAP_CELL_GRID_SIZE);
                            if (*(reachedGrids[slot] + tileX + tileY * MAP_CELL_GRID_SIZE)) {
                                filled = 1;
                                reachable[slot]++;
                            }
                        }
                    }
                }
            }
        }
        for (slot = 1; slot < RANDOM_MAP_CASTLE_SLOTS; slot++) {
            for (t = 0; t < slot; t++) {
                if (belongs[slot] == belongs[t]) {
                    meet = 0;
                    for (tileX = 0; tileX < MAP_CELL_GRID_SIZE; tileX++)
                        for (tileY = 0; tileY < MAP_CELL_GRID_SIZE; tileY++)
                            if (*(reachedGrids[slot] + tileX + tileY * MAP_CELL_GRID_SIZE)
                                && *(reachedGrids[t] + tileX + tileY * MAP_CELL_GRID_SIZE))
                                meet = 1;
                    if (!meet) {
                        cutOff[slot] = 1;
                        cutOff[t] = 1;
                    }
                }
            }
        }
        for (slot = 0; slot < RANDOM_MAP_CASTLE_SLOTS; slot++) {
            if (cutOff[slot] || extraRoads[slot]) {
                steps = 20000;
                tracing = 1;
                while (tracing) {
                    if (steps-- < 0)
                        tracing = 0;
                    tileX = Random(0, MAP_CELL_GRID_SIZE - 1);
                    tileY = Random(0, MAP_CELL_GRID_SIZE - 1);
                    if (*(reachedGrids[slot] + tileX + tileY * MAP_CELL_GRID_SIZE)) {
                        dist = abs(tileX - keeps[slot].x) + abs(tileY - keeps[slot].y);
                        if (steps < 10000 || Random(0, 100) < dist) {
                            tracing = 0;
                            for (nearX = 0; nearX < MAP_CELL_GRID_SIZE; nearX++) {
                                for (scanY = 0; scanY < MAP_CELL_GRID_SIZE; scanY++) {
                                    if (m_map.cells[nearX][scanY].m_triggerType
                                        == (MAP_TRIGGER_EVENT | MAP_OBJECT_STONE_LITHS)) {
                                        dist = abs(tileX - nearX) + abs(tileY - scanY);
                                        if (steps > 10000 && dist < 40 && Random(0, 100) > dist)
                                            tracing = 1;
                                    }
                                }
                            }
                        }
                    }
                }
                gEditManager->ClearArea(tileX, tileY, 1, 1, EDIT_CLEAR_ALL, 1);
                if (m_map.cells[tileX][tileY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                    == TERRAIN_DESERT)
                    PlaceOverlay(desertGateLiths, tileX, tileY);
                if (m_map.cells[tileX][tileY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                    == TERRAIN_SNOW)
                    PlaceOverlay(winterGate, tileX, tileY);
                else
                    PlaceOverlay(anyGate, tileX, tileY);
            }
        }
    }
    free(regionGrid);
    for (slot = 0; slot < RANDOM_MAP_CASTLE_SLOTS; slot++)
        free(reachedGrids[slot]);
}

VA(0x00414576, 0x21f)
void editManager::PlaceResourceSite(i32 x, i32 y, i32 kind) {
    overlayType* river;
    overlayType* site;
    i32 ground;
    i32 index;
    char name[12];

    site = NULL;
    river = NULL;
    ground = m_map.cells[x][y].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN;
    if (kind == 0) {
        for (index = 0; index < OVERLAY_TYPE_COUNT; index++)
            if (!strcmpi(gOverlayTypes[index].name, "sawmill "))
                site = &gOverlayTypes[index];
        PlaceOverlay(site, x, y);
    } else if (kind == 1) {
        for (index = 0; index < OVERLAY_TYPE_COUNT; index++)
            if (!strnicmp(gOverlayTypes[index].name, "alch-0", 6)
                && gOverlayTypes[index].terrainMask & 1 << ground)
                site = &gOverlayTypes[index];
        PlaceOverlay(site, x, y);
    } else {
        sprintf(name, "rovr-0%d ", kind);
        for (index = 0; index < OVERLAY_TYPE_COUNT; index++) {
            if (!strnicmp(gOverlayTypes[index].name, "mine-0", 6)
                && gOverlayTypes[index].terrainMask & 1 << ground)
                site = &gOverlayTypes[index];
            if (!strcmpi(gOverlayTypes[index].name, name))
                river = &gOverlayTypes[index];
        }
        PlaceOverlay(site, x, y);
        PlaceMineResource(river, x + 1, y, 1);
    }
}

VA(0x00414795, 0xbd6)
void editManager::PlaceRandomObjects(i32 density, i32 strength) {
    i32 valid;
    i32 siteIndex;
    overlayType* obeliskTypes[EDITOR_GENERATOR_TERRAIN_COUNT];
    i32 attempts;
    overlayType* strong;
    i32 towns;
    i32 obeliskTurns;
    i32 x;
    i32 appeal;
    overlayType* veryStrong;
    i32 entry;
    i32 y;
    i32 i;
    overlayType* weak;
    i32 mineTries;
    i32 j;
    i32 spacing;
    i32 ground;
    i32 quota[RANDOM_MAP_SITE_KIND_COUNT];
    i32 laid;
    overlayType* anyMonster;
    overlayType* medium;
    overlayType* townObject;

    anyMonster = NULL;
    weak = NULL;
    medium = NULL;
    strong = NULL;
    veryStrong = NULL;
    townObject = NULL;
    laid = 0;
    ScatterDecorations();
    for (entry = 0; entry < EDITOR_GENERATOR_TERRAIN_COUNT; entry++)
        obeliskTypes[entry] = NULL;
    for (entry = 0; entry < OVERLAY_TYPE_COUNT; entry++) {
        if (!strcmpi(gOverlayTypes[entry].name, "xtown   "))
            townObject = &gOverlayTypes[entry];
        if (!strcmpi(gOverlayTypes[entry].name, "mon-28  "))
            anyMonster = &gOverlayTypes[entry];
        if (!strcmpi(gOverlayTypes[entry].name, "mon-29  "))
            weak = &gOverlayTypes[entry];
        if (!strcmpi(gOverlayTypes[entry].name, "mon-30  "))
            medium = &gOverlayTypes[entry];
        if (!strcmpi(gOverlayTypes[entry].name, "mon-31  "))
            strong = &gOverlayTypes[entry];
        if (!strcmpi(gOverlayTypes[entry].name, "mon-32  "))
            veryStrong = &gOverlayTypes[entry];
        if (!strnicmp(gOverlayTypes[entry].name, "obelisk", 7))
            obeliskTypes[gOverlayTypes[entry].name[7] - '0'] = &gOverlayTypes[entry];
    }
    towns = gLandCellCount / 640;
    if (towns > 12)
        towns = 12;
    if (towns < 1)
        towns = 1;
    towns *= 100;
    while (towns > 0) {
        towns--;
        x = Random(2, 69);
        y = Random(2, 70);
        valid = 1;
        for (i = -2; i <= 2; i++) {
            for (j = -2; j <= 1; j++) {
                if (m_map.cells[x + i][y + j].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
                    || m_map.cells[x + i][y + j].m_objectTileset == TILESET_TOWN32
                    || m_map.cells[x + i][y + j].m_overlayTileset == TILESET_TOWN32)
                    valid = 0;
            }
        }
        for (i = 0; i < MAP_CELL_GRID_SIZE - 1; i++) {
            for (j = 0; j < MAP_CELL_GRID_SIZE - 1; j++) {
                if (m_map.cells[i][j].m_objectTileset == TILESET_TOWN32
                    && m_map.cells[i][j].m_triggerType & MAP_TRIGGER_EVENT) {
                    spacing = abs(i - x) + abs(j - y);
                    if (spacing < 10 || spacing < Random(0, 40))
                        valid = 0;
                }
            }
        }
        if (valid) {
            towns -= 100;
            gEditManager->ClearArea(x - 2, y - 2, 5, 4, EDIT_CLEAR_ALL, 1);
            PlaceOverlay(townObject, x - 2, y);
        }
    }
    mineTries = gLandCellCount / 140;
    ScaleByDensity(&mineTries, density);
    quota[RANDOM_MAP_SITE_SAWMILL] = (mineTries - 13) / 7 + 5;
    quota[RANDOM_MAP_SITE_FIRST_MINE] = (mineTries - 13) / 7 + 5;
    quota[6] = (mineTries - 13) / 7 + 2;
    quota[5] = (mineTries - 13) / 7 + 2;
    quota[4] = (mineTries - 13) / 7 + 2;
    quota[3] = (mineTries - 13) / 7 + 2;
    quota[RANDOM_MAP_SITE_ALCHEMIST_LAB] = (mineTries - 13) / 7 + 2;
    if (mineTries > 34)
        mineTries = 34;
    if (mineTries < 6)
        mineTries = 6;
    mineTries *= 1000;
    while (mineTries > 0) {
        mineTries--;
        valid = 1;
        x = Random(1, 70);
        y = Random(1, 70);
        if (valid && m_map.cells[x][y].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
            && m_map.cells[x + 1][y].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
            && m_map.cells[x][y - 1].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
            && m_map.cells[x + 1][y - 1].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
            && m_map.cells[x - 1][y + 1].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
            && !m_map.cells[x][y - 1].m_overlayTileset
            && !m_map.cells[x + 1][y - 1].m_overlayTileset
            && m_map.cells[x][y].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                   == m_map.cells[x + 1][y].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN) {
            attempts = 0;
            valid = 1;
            while (valid && attempts < 10) {
                attempts++;
                siteIndex = Random(0, RANDOM_MAP_SITE_KIND_COUNT - 1);
                appeal = 4;
                if (quota[siteIndex] > 0)
                    appeal += 30;
                if (siteIndex == RANDOM_MAP_SITE_SAWMILL)
                    appeal += 8;
                if (siteIndex == RANDOM_MAP_SITE_FIRST_MINE)
                    appeal += 8;
                for (i = 0; i < MAP_CELL_GRID_SIZE; i++) {
                    for (j = 0; j < MAP_CELL_GRID_SIZE; j++) {
                        if (m_map.cells[i][j].m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_MINE)
                            || m_map.cells[i][j].m_triggerType
                                   == (MAP_TRIGGER_EVENT | MAP_OBJECT_SAWMILL)
                            || m_map.cells[i][j].m_triggerType
                                   == (MAP_TRIGGER_EVENT | MAP_OBJECT_ALCHEMIST_LAB)) {
                            spacing = abs(i - x) + abs(j - y);
                            if (spacing < 10)
                                appeal -= 10 - spacing;
                            if (siteIndex == RANDOM_MAP_SITE_SAWMILL
                                    && m_map.cells[i][j].m_triggerType
                                           == (MAP_TRIGGER_EVENT | MAP_OBJECT_SAWMILL)
                                || siteIndex == RANDOM_MAP_SITE_ALCHEMIST_LAB
                                       && m_map.cells[i][j].m_triggerType
                                              == (MAP_TRIGGER_EVENT | MAP_OBJECT_ALCHEMIST_LAB)
                                || m_map.cells[i][j].m_triggerType
                                           == (MAP_TRIGGER_EVENT | MAP_OBJECT_MINE)
                                       && gMineSiteKinds[m_map.cells[i + 1][j].m_extraFrame % 5]
                                              == siteIndex) {
                                if (spacing < 15)
                                    appeal -= (15 - spacing) * 2;
                            }
                        }
                    }
                }
                if (Random(0, 40) < appeal) {
                    gEditManager->ClearArea(x, y - 1, 2, 2, EDIT_CLEAR_ALL, 0);
                    gEditManager->ClearArea(x - 1, y + 1, 1, 1, EDIT_CLEAR_ALL, 0);
                    PlaceResourceSite(x, y, siteIndex);
                    laid++;
                    quota[siteIndex]--;
                    if (Random(0, 100) < strength)
                        PlaceOverlay(Random(0, 100) < 30 ? medium : strong, x - 1, y + 1);
                    mineTries -= 1000;
                    valid = 0;
                }
            }
        }
    }
    obeliskTurns = gLandCellCount / 180;
    if (obeliskTurns > 24)
        obeliskTurns = 24;
    if (obeliskTurns < 8)
        obeliskTurns = 8;
    obeliskTurns *= 100;
    while (obeliskTurns > 0) {
        obeliskTurns--;
        valid = 1;
        x = Random(0, MAP_CELL_GRID_SIZE - 1);
        y = Random(0, MAP_CELL_GRID_SIZE - 1);
        if (m_map.cells[x][y].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN) {
            ground = m_map.cells[x][y].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN;
            if (CanPlaceOverlay(obeliskTypes[ground], x, y)) {
                obeliskTurns -= 100;
                PlaceOverlay(obeliskTypes[ground], x, y);
            }
        }
    }
}

VA(0x0041536b, 0xd32)
void editManager::PlaceTreasures(i32 density, i32 strength) {
    overlayType* chest;
    overlayType* genieLamp;
    i32 caches;
    i32 seOpen;
    i32 down;
    i32 neOpen;
    i32 right;
    overlayType* veryStrong;
    i32 kindRoll;
    overlayType* strongMonster;
    overlayType* weak;
    overlayType* anyMonster;
    overlayType* medium;
    i32 curX;
    i32 nwOpen;
    i32 swOpen;
    overlayType* goods;
    overlayType* bonfire;
    i32 up;
    i32 pilesSoFar;
    i32 wanderers;
    i32 k;
    i32 curY;
    overlayType* bounty;
    i32 guardsDone;
    i32 layout;
    i32 left;

    anyMonster = NULL;
    weak = NULL;
    medium = NULL;
    strongMonster = NULL;
    veryStrong = NULL;
    goods = NULL;
    bounty = NULL;
    chest = NULL;
    bonfire = NULL;
    genieLamp = NULL;
    for (k = 0; k < OVERLAY_TYPE_COUNT; k++) {
        if (!strcmpi(gOverlayTypes[k].name, "mon-28  "))
            anyMonster = &gOverlayTypes[k];
        if (!strcmpi(gOverlayTypes[k].name, "mon-29  "))
            weak = &gOverlayTypes[k];
        if (!strcmpi(gOverlayTypes[k].name, "mon-30  "))
            medium = &gOverlayTypes[k];
        if (!strcmpi(gOverlayTypes[k].name, "mon-31  "))
            strongMonster = &gOverlayTypes[k];
        if (!strcmpi(gOverlayTypes[k].name, "mon-32  "))
            veryStrong = &gOverlayTypes[k];
        if (!strcmpi(gOverlayTypes[k].name, "xrsrc   "))
            goods = &gOverlayTypes[k];
        if (!strcmpi(gOverlayTypes[k].name, "xart    "))
            bounty = &gOverlayTypes[k];
        if (!strcmpi(gOverlayTypes[k].name, "chest   "))
            chest = &gOverlayTypes[k];
        if (!strcmpi(gOverlayTypes[k].name, "lamp    "))
            genieLamp = &gOverlayTypes[k];
        if (!strcmpi(gOverlayTypes[k].name, "firemult"))
            bonfire = &gOverlayTypes[k];
    }
    caches = gLandCellCount / 40;
    ScaleByDensity(&caches, density);
    caches *= 100;
    pilesSoFar = 0;
    guardsDone = 0;
    while (caches > 0) {
        caches--;
        curX = Random(1, 70);
        curY = Random(1, 70);
        if (m_map.cells[curX][curY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
            && !m_map.cells[curX][curY].m_objectTileset) {
            gEditManager->ClearArea(curX, curY, 1, 1, EDIT_CLEAR_ALL, 0);
            kindRoll = Random(0, 100);
            nwOpen = swOpen = neOpen = seOpen = up = down = right = left = 0;
            if (curY == 0 || m_map.cells[curX][curY - 1].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
                || m_map.cells[curX][curY - 1].m_objectTileset <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX][curY - 1].m_objectTileset > 0
                || m_map.cells[curX][curY - 1].m_overlayTileset <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX][curY - 1].m_overlayTileset > 0)
                up = 1;
            if (curY == MAP_CELL_GRID_SIZE - 1
                || m_map.cells[curX][curY + 1].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
                || m_map.cells[curX][curY + 1].m_objectTileset <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX][curY + 1].m_objectTileset > 0
                || m_map.cells[curX][curY + 1].m_overlayTileset <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX][curY + 1].m_overlayTileset > 0)
                down = 1;
            if (curX == 0 || m_map.cells[curX - 1][curY].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
                || m_map.cells[curX - 1][curY].m_objectTileset <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX - 1][curY].m_objectTileset > 0
                || m_map.cells[curX - 1][curY].m_overlayTileset <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX - 1][curY].m_overlayTileset > 0)
                left = 1;
            if (curX == MAP_CELL_GRID_SIZE - 1
                || m_map.cells[curX + 1][curY].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
                || m_map.cells[curX + 1][curY].m_objectTileset <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX + 1][curY].m_objectTileset > 0
                || m_map.cells[curX + 1][curY].m_overlayTileset <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX + 1][curY].m_overlayTileset > 0)
                right = 1;
            if (curX < MAP_CELL_GRID_SIZE + 1 && curY > 0
                && m_map.cells[curX + 1][curY - 1].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                && !m_map.cells[curX + 1][curY - 1].m_objectTileset)
                neOpen = 1;
            if (curX < MAP_CELL_GRID_SIZE + 1 && curY < MAP_CELL_GRID_SIZE - 1
                && m_map.cells[curX + 1][curY + 1].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                && !m_map.cells[curX + 1][curY + 1].m_objectTileset)
                seOpen = 1;
            if (curX > 0 && curY < MAP_CELL_GRID_SIZE - 1
                && m_map.cells[curX - 1][curY + 1].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                && !m_map.cells[curX - 1][curY + 1].m_objectTileset)
                swOpen = 1;
            if (curX > 0 && curY > 0
                && m_map.cells[curX - 1][curY - 1].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                && !m_map.cells[curX - 1][curY - 1].m_objectTileset)
                nwOpen = 1;
            if (!right || !left || !up || !down) {
                pilesSoFar++;
                caches -= 100;
                layout = TREASURE_UNGUARDED;
                if (guardsDone * 6 < pilesSoFar && guardsDone < 31) {
                    if (left && down && neOpen && !swOpen && !seOpen && !nwOpen)
                        layout = TREASURE_GUARD_NE;
                    else if (left && up && seOpen && !nwOpen && !swOpen && !neOpen)
                        layout = TREASURE_GUARD_SE;
                    else if (right && up && swOpen && !neOpen && !nwOpen && !seOpen)
                        layout = TREASURE_GUARD_SW;
                    else if (right && down && nwOpen && !seOpen && !swOpen && !neOpen)
                        layout = TREASURE_GUARD_NW;
                }
                if (layout > TREASURE_UNGUARDED_LAST) {
                    guardsDone++;
                    if (kindRoll < 35)
                        PlaceOverlay(chest, curX, curY);
                    else
                        PlaceOverlay(bounty, curX, curY);
                    if (layout == TREASURE_GUARD_NE) {
                        gEditManager->ClearArea(curX + 1, curY - 1, 1, 1, EDIT_CLEAR_ALL, 0);
                        PlaceOverlay(
                            Random(0, 100) < 50 ? strongMonster : veryStrong,
                            curX + 1,
                            curY - 1
                        );
                    } else if (layout == TREASURE_GUARD_SE) {
                        gEditManager->ClearArea(curX + 1, curY + 1, 1, 1, EDIT_CLEAR_ALL, 0);
                        PlaceOverlay(
                            Random(0, 100) < 50 ? strongMonster : veryStrong,
                            curX + 1,
                            curY + 1
                        );
                    } else if (layout == TREASURE_GUARD_SW) {
                        gEditManager->ClearArea(curX - 1, curY + 1, 1, 1, EDIT_CLEAR_ALL, 0);
                        PlaceOverlay(
                            Random(0, 100) < 50 ? strongMonster : veryStrong,
                            curX - 1,
                            curY + 1
                        );
                    } else {
                        gEditManager->ClearArea(curX - 1, curY - 1, 1, 1, EDIT_CLEAR_ALL, 0);
                        PlaceOverlay(
                            Random(0, 100) < 50 ? strongMonster : veryStrong,
                            curX - 1,
                            curY - 1
                        );
                    }
                } else if (Random(0, 100) < 90) {
                    caches += 101;
                    pilesSoFar--;
                } else if (m_map.cells[curX][curY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                               == TERRAIN_DESERT
                           && kindRoll < 20)
                    PlaceOverlay(genieLamp, curX, curY);
                else if (kindRoll < 2)
                    PlaceOverlay(genieLamp, curX, curY);
                else if (kindRoll < 25)
                    PlaceOverlay(chest, curX, curY);
                else if (kindRoll < 35)
                    PlaceOverlay(bonfire, curX, curY);
                else
                    PlaceOverlay(goods, curX, curY);
            }
        }
    }
    wanderers = gLandCellCount / 160;
    ScaleByDensity(&wanderers, strength);
    wanderers *= 100;
    while (wanderers > 0) {
        wanderers--;
        curX = Random(0, MAP_CELL_GRID_SIZE - 1);
        curY = Random(0, MAP_CELL_GRID_SIZE - 1);
        if (m_map.cells[curX][curY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
            && !m_map.cells[curX][curY].m_objectTileset) {
            wanderers -= 100;
            gEditManager->ClearArea(curX, curY, 1, 1, EDIT_CLEAR_ALL, 0);
            k = Random(0, 100);
            if (k < 40)
                PlaceOverlay(weak, curX, curY);
            else if (k < 80)
                PlaceOverlay(medium, curX, curY);
            else
                PlaceOverlay(strongMonster, curX, curY);
        }
    }
}

VA(0x0041609d, 0x1ac)
void editManager::ScatterDecorations(void) {
    i32 terrainChance[EDITOR_GENERATOR_TERRAIN_COUNT] = {15, 120, 120, 120, 120, 80, 120};
    overlayType* chosen;
    i32 selected;
    i32 dice;
    i32 attempts;
    i16 x;
    i16 y;
    mapCell* cell;

    for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
        for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
            cell = &gEditManager->m_map.cells[x][y];
            if (Random(1, 1000) <= terrainChance[cell->m_tileIndex / MAP_CELL_TILES_PER_TERRAIN]
                && cell->m_objectIndex == MAP_CELL_NO_FRAME
                && cell->m_overlayIndex == MAP_CELL_NO_FRAME
                && cell->m_tileIndex % MAP_CELL_TILES_PER_TERRAIN < 4) {
                attempts = 100;
                while (attempts-- > 0) {
                    selected = Random(0, OVERLAY_TYPE_COUNT - 1);
                    dice = Random(1, 100);
                    chosen = &gOverlayTypes[selected];
                    if (dice <= chosen->frequency && CanPlaceOverlay(chosen, x, y)) {
                        PlaceOverlay(chosen, x, y);
                        attempts = 0;
                    }
                }
            }
        }
    }
}

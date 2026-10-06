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
static mapStep gChainSteps[CHAIN_DIRECTION_COUNT] =
    {{1, -2}, {1, -1}, {1, 2}, {1, 1}, {-1, 2}, {-1, 1}, {-1, -2}, {-1, -1}};

// A chain's sideways shift when it turns right ([0]) or left ([1]).
DATA(0x0044c124)
static mapStep gChainTurns[CHAIN_DIRECTION_COUNT][2] = {
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
static i32 gTreasureSizes[5] = {2, 3, 4, 5, 6};

VA(0x00410fa0, 0x3c5)
void editManager::GenerateRandomMap(void) {
    i32 attempt;
    i32 type;
    i32 done;
    i32 unusedTries;
    double unusedPercent;
    i32 canvas;
    double unusedRatio;

    if (!RandomMapDialog()) {
        DrawMap();
        UpdateMapView();
        DrawRadar(1);
        return;
    }
    if (gRandomMapPromptSave)
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
            if (gRandomTerrainPercent[type] > 0.0) {
                PaintRandomTerrain(type, 100, 0);
                canvas = type + 1;
                type = 99;
            }
        }
        for (type = canvas; type <= TERRAIN_LAST; type++) {
            if (gRandomTerrainPercent[type] > 0.0) {
                sprintf(
                    gText,
                    localization::Tr("editor.random.status.terrain"),
                    gEditTerrainNames[type]
                );
                ShowStatusText(gText);
                PaintRandomTerrain(type, static_cast<i32>(gRandomTerrainPercent[type]), canvas - 1);
            }
        }
        ShowStatusText(localization::Tr("editor.random.status.smoothing"));
        RemoveSmallRegions();
        for (type = 0; type <= TERRAIN_LAST; type++)
            BlendTerrain(type, 1, 0, 1, 0);
        BlendTerrain(TERRAIN_WATER, 1, 0, 0, 1);
        ShowStatusText(localization::Tr("editor.random.status.mountains"));
        PlaceObstacleChains(
            static_cast<i32>(gRandomMapDensity[RANDOM_MAP_MOUNTAINS]),
            TILESET_MTN32
        );
        ShowStatusText(localization::Tr("editor.random.status.trees"));
        PlaceObstacleChains(static_cast<i32>(gRandomMapDensity[RANDOM_MAP_TREES]), TILESET_TREE32);
        ShowStatusText(localization::Tr("editor.random.status.objects"));
        PlaceRandomObjects(
            static_cast<i32>(gRandomMapDensity[RANDOM_MAP_OBJECTS]),
            static_cast<i32>(gRandomMapDensity[RANDOM_MAP_MONSTERS])
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
            static_cast<i32>(gRandomMapDensity[RANDOM_MAP_TREASURE]),
            static_cast<i32>(gRandomMapDensity[RANDOM_MAP_MONSTERS])
        );
        done = HasEnoughCastles();
        if (!done)
            continue;
        if (gGeneratingMaps) {
            ShowStatusText(localization::Tr("editor.random.status.save_prompt"));
            if (EditMapDetails(1) && !SaveMap(m_mapFileName)) {
                sprintf(gText, localization::Tr("editor.random.saved"), gpMapHeader->name);
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
            cell = &m_cells[x][y];
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
                m_cells[walkX][walkY].m_tileIndex = terrain * MAP_CELL_TILES_PER_TERRAIN;
    } else {
        total = percent * (MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE) / 100;
        patches = Random(0, percent + 51) / 30 + 1;
        balance = total;
        escapes = 0;
        lowerCap = gRandomMapClimate ? 2 : 3;
        upperWeight = (gRandomMapClimate != 0) + 6;
        for (cluster = 0; cluster < patches; cluster++) {
            perSeed = balance / (patches - cluster);
            looking = 1;
            guard = 0;
            while (guard < 200 && looking) {
                guard++;
                if (gRandomMapClimate)
                    startX = Random(0, MAP_CELL_GRID_SIZE - 1);
                else
                    startX =
                        (Random(0, MAP_CELL_GRID_SIZE - 1) + Random(0, MAP_CELL_GRID_SIZE - 1)
                         + Random(0, MAP_CELL_GRID_SIZE - 1) + Random(0, MAP_CELL_GRID_SIZE - 1))
                        / 4;
                if (gRandomMapClimate) {
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
                if (m_cells[startX][sourceY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
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
                while (m_cells[walkX][walkY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN == terrain
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
                if (m_cells[walkX][walkY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN == baseTerrain)
                    m_cells[walkX][walkY].m_tileIndex = terrain * MAP_CELL_TILES_PER_TERRAIN;
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
            ground = giGroundToTerrain[m_cells[startX][homeY].m_tileIndex];
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
                        if (giGroundToTerrain[m_cells[n][j].m_tileIndex] != ground) {
                            if (other == -1)
                                other = giGroundToTerrain[m_cells[n][j].m_tileIndex];
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
                        m_cells[n][j].m_tileIndex = other * MAP_CELL_TILES_PER_TERRAIN;
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
            if (m_cells[tallyX][landY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN)
                gLandCellCount++;
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
            if (m_cells[rootX][rootY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN)
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
            ground = giGroundToTerrain[m_cells[rootX][rootY].m_tileIndex];
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
                        rootX += gChainTurns[direction][0].x;
                        rootY += gChainTurns[direction][0].y;
                        direction = (direction + 10) % 8;
                    } else {
                        rootX += gChainTurns[direction][1].x;
                        rootY += gChainTurns[direction][1].y;
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
    editObject* generic;
    i32 ground;
    editObject* specific;
    i32 n;
    i32 bit;

    if (*x < 0 || *x > MAP_CELL_GRID_SIZE - 1 || *y < 0 || *y > MAP_CELL_GRID_SIZE - 1)
        return 0;
    frame = 0;
    ground = giGroundToTerrain[m_cells[*x][*y].m_tileIndex];
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
    for (n = 0; n < EDIT_OBJECT_SCAN_COUNT; n++) {
        if (!specific && gEditObjects[n].tileset == tileset && gEditObjects[n].terrainMask == bit)
            specific = &gEditObjects[n + frame];
        if (!generic && gEditObjects[n].tileset == tileset
            && gEditObjects[n].terrainMask == EDIT_OBJECT_ANY_TERRAIN
            && (!kind || kind == gEditObjects[n].name[0]))
            generic = &gEditObjects[n + frame];
    }
    if (specific && CanPlaceObject(specific, *x, *y)) {
        PlaceObject(specific, *x, *y);
        *x += gChainSteps[direction].x;
        *y += gChainSteps[direction].y;
        return 1;
    }
    if (CanPlaceObject(generic, *x, *y)) {
        PlaceObject(generic, *x, *y);
        *x += gChainSteps[direction].x;
        *y += gChainSteps[direction].y;
        return 1;
    }
    return 0;
}

VA(0x00414576, 0x21f)
void editManager::PlaceResourceSite(i32 x, i32 y, i32 kind) {
    editObject* river;
    editObject* site;
    i32 ground;
    i32 index;
    char name[12];

    site = NULL;
    river = NULL;
    ground = m_cells[x][y].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN;
    if (kind == 0) {
        for (index = 0; index < EDIT_OBJECT_SCAN_COUNT; index++)
            if (!strcmpi(gEditObjects[index].name, "sawmill "))
                site = &gEditObjects[index];
        PlaceObject(site, x, y);
    } else if (kind == 1) {
        for (index = 0; index < EDIT_OBJECT_SCAN_COUNT; index++)
            if (!strnicmp(gEditObjects[index].name, "alch-0", 6)
                && gEditObjects[index].terrainMask & 1 << ground)
                site = &gEditObjects[index];
        PlaceObject(site, x, y);
    } else {
        sprintf(name, "rovr-0%d ", kind);
        for (index = 0; index < EDIT_OBJECT_SCAN_COUNT; index++) {
            if (!strnicmp(gEditObjects[index].name, "mine-0", 6)
                && gEditObjects[index].terrainMask & 1 << ground)
                site = &gEditObjects[index];
            if (!strcmpi(gEditObjects[index].name, name))
                river = &gEditObjects[index];
        }
        PlaceObject(site, x, y);
        PlaceObjectOverlay(river, x + 1, y, 1);
    }
}

VA(0x0041609d, 0x1ac)
void editManager::ScatterDecorations(void) {
    i32 terrainChance[EDITOR_TERRAIN_COUNT] = {15, 120, 120, 120, 120, 80, 120};
    editObject* chosen;
    i32 selected;
    i32 dice;
    i32 attempts;
    i16 x;
    i16 y;
    mapCell* cell;

    for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
        for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
            cell = &gEditManager->m_cells[x][y];
            if (Random(1, 1000) <= terrainChance[cell->m_tileIndex / MAP_CELL_TILES_PER_TERRAIN]
                && cell->m_objectIndex == MAP_CELL_NO_FRAME
                && cell->m_overlayIndex == MAP_CELL_NO_FRAME
                && cell->m_tileIndex % MAP_CELL_TILES_PER_TERRAIN < 4) {
                attempts = 100;
                while (attempts-- > 0) {
                    selected = Random(0, EDIT_OBJECT_SCAN_COUNT - 1);
                    dice = Random(1, 100);
                    chosen = &gEditObjects[selected];
                    if (dice <= chosen->frequency && CanPlaceObject(chosen, x, y)) {
                        PlaceObject(chosen, x, y);
                        attempts = 0;
                    }
                }
            }
        }
    }
}

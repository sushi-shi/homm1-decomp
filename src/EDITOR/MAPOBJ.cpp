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
#include <SOURCE/resourceTypes.h>
#include <SOURCE/terrainTypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The cell after a chain link in each of the eight directions.
DATA(0x0044c0e4)
static H1_ENUM_ARRAY(MapStepPair, gChainSteps, ChainDirection, CHAIN_DIRECTION_COUNT) =
    {{1, -2}, {1, -1}, {1, 2}, {1, 1}, {-1, 2}, {-1, 1}, {-1, -2}, {-1, -1}};

// A chain's sideways shift when it turns clockwise or counterclockwise.
DATA(0x0044c124)
static H1_ENUM_ARRAY2(
    MapStepPair,
    gChainTurns,
    ChainDirection,
    CHAIN_DIRECTION_COUNT,
    ChainTurn,
    CHAIN_TURN_COUNT
) = {
    {{1, 3}, {-1, 0}},
    {{1, 0}, {0, 0}},
    {{-1, 0}, {-3, 1}},
    {{0, 0}, {1, 0}},
    {{-1, -3}, {-1, 0}},
    {{0, 0}, {0, 0}},
    {{1, 0}, {3, -1}},
    {{0, 0}, {0, 0}}
};

// The resource each mine resource marker frame stands for (WriteMines
// records a mine as its marker frame + 2).
DATA(0x0044c1a4)
static H1_ENUM_STORAGE(ResourceType, i32) gMineSiteKinds[RANDOM_MAP_MINE_RESOURCE_COUNT] =
    {RESOURCE_ORE, RESOURCE_SULFUR, RESOURCE_CRYSTAL, RESOURCE_GEMS, RESOURCE_GOLD};

// No retail code reads this; it holds its retail .bss place, before
// GenerateRandomMap's empty status text.
DATA(0x00452ef4)
i32 gUnusedData452ef4;

#define terrain type     // frame-slot spelling
#define paintFrom canvas // frame-slot spelling
VA(0x00410fa0, 0x3c5)
void editManager::GenerateRandomMap(void) {
    i32 attempt;
    i32 terrain;
    b32 done;
    i32 unusedTries;
    double unusedPercent;
    i32 paintFrom;
    double unusedRatio;

    if (!NewMapDialog()) {
        DrawMap();
        UpdateMapView();
        DrawRadar(1);
        return;
    }
    if (gSaveUnseen)
        gGeneratingMaps = true;
    NewMap(true);
    done = false;
    attempt = 0;
    while (!done && attempt < RANDOM_MAP_ATTEMPTS) {
        attempt++;
        ResetArea(0, 0, MAP_CELL_GRID_SIZE, MAP_CELL_GRID_SIZE);
        DrawMap();
        UpdateMapView();
        DrawRadar(1);
        unusedPercent = 100.0;
        paintFrom = 0;
        for (terrain = 0; terrain <= H1_ENUM_ENCODE(TerrainType, TERRAIN_LAST); terrain++) {
            if (gTerrainPercent[terrain] > 0.0) {
                PaintRandomTerrain(
                    H1_ENUM_DECODE(TerrainType, terrain),
                    RANDOM_MAP_FULL_PERCENT,
                    TERRAIN_WATER
                );
                paintFrom = terrain + 1;
                terrain = RANDOM_MAP_END_TERRAIN_SCAN;
            }
        }
        for (terrain = paintFrom; terrain <= H1_ENUM_ENCODE(TerrainType, TERRAIN_LAST); terrain++) {
            if (gTerrainPercent[terrain] > 0.0) {
                sprintf(
                    gText,
                    localization::Tr("editor.random.status.terrain"),
                    gGeneratorTerrainNames[terrain]
                );
                ShowStatusText(gText);
                PaintRandomTerrain(
                    H1_ENUM_DECODE(TerrainType, terrain),
                    gTerrainPercent[terrain],
                    H1_ENUM_DECODE(TerrainType, paintFrom - 1)
                );
            }
        }
        ShowStatusText(localization::Tr("editor.random.status.smoothing"));
        RemoveSmallRegions();
        for (terrain = 0; terrain <= H1_ENUM_ENCODE(TerrainType, TERRAIN_LAST); terrain++)
            BlendTerrain(H1_ENUM_DECODE(TerrainType, terrain), 1, 0, 1, 0);
        BlendTerrain(TERRAIN_WATER, 1, 0, 0, 1);
        ShowStatusText(localization::Tr("editor.random.status.mountains"));
        PlaceObstacleChains(
            gDensityPercent[H1_ENUM_ENCODE(GeneratorDensity, GENERATOR_DENSITY_MOUNTAINS)],
            TILESET_MTN32
        );
        ShowStatusText(localization::Tr("editor.random.status.trees"));
        PlaceObstacleChains(
            gDensityPercent[H1_ENUM_ENCODE(GeneratorDensity, GENERATOR_DENSITY_TREES)],
            TILESET_TREE32
        );
        ShowStatusText(localization::Tr("editor.random.status.objects"));
        PlaceRandomObjects(
            gDensityPercent[H1_ENUM_ENCODE(GeneratorDensity, GENERATOR_DENSITY_OBJECTS)],
            gDensityPercent[H1_ENUM_ENCODE(GeneratorDensity, GENERATOR_DENSITY_MONSTERS)]
        );
        ShowStatusText(localization::Tr("editor.random.status.land"));
        PlaceTowns();
        for (terrain = 0; terrain <= H1_ENUM_ENCODE(TerrainType, TERRAIN_LAST); terrain++)
            BlendTerrain(H1_ENUM_DECODE(TerrainType, terrain), 1, 0, 1, 0);
        gVaryTiles = true;
        BlendTerrain(TERRAIN_WATER, 1, 0, 0, 1);
        gVaryTiles = false;
        ShowStatusText(localization::Tr("editor.random.status.treasure"));
        PlaceTreasures(
            gDensityPercent[H1_ENUM_ENCODE(GeneratorDensity, GENERATOR_DENSITY_TREASURE)],
            gDensityPercent[H1_ENUM_ENCODE(GeneratorDensity, GENERATOR_DENSITY_MONSTERS)]
        );
        done = HasEnoughCastles();
        if (!done)
            continue;
        if (gGeneratingMaps) {
            ShowStatusText(localization::Tr("editor.random.status.save_prompt"));
            if (MapDetailsDialog(true)
                && !H1_ENUM_ENCODE(BaseManagerStatus, SaveMap(m_mapFileName))) {
                sprintf(gText, localization::Tr("editor.random.saved"), gMapHeader->name[0]);
                NormalDialog(gText, NORMAL_DIALOG_TYPE_OK);
            }
            ResetArea(0, 0, MAP_CELL_GRID_SIZE, MAP_CELL_GRID_SIZE);
            gGeneratingMaps = false;
        }
    }
    DrawMap();
    ShowStatusText("");
    ClearStatusText();
    UpdateMapView();
    DrawRadar(1);
}
#undef terrain
#undef paintFrom

VA(0x00411365, 0xd5)
b32 editManager::HasEnoughCastles(void) {
    i32 count;
    i32 y;
    i32 x;
    mapCell* cell;

    count = 0;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map.cells[x][y];
            if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_CASTLE)
                || cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)
                       && (cell->m_objectIndex == EDIT_CASTLE_FRAME(TOWN_TYPE_KNIGHT)
                           || cell->m_objectIndex == EDIT_CASTLE_FRAME(TOWN_TYPE_SORCERESS)
                           || cell->m_objectIndex == EDIT_CASTLE_FRAME(TOWN_TYPE_BARBARIAN)
                           || cell->m_objectIndex == EDIT_CASTLE_FRAME(TOWN_TYPE_WARLOCK)))
                count++;
        }
    }
    return count >= EDIT_MAP_MIN_CASTLES;
}

#define seedY sourceY          // frame-slot spelling
#define targetCells total      // frame-slot spelling
#define horizontalWeight xBias // frame-slot spelling
#define maxWeight upperWeight  // frame-slot spelling
#define upWeight yAxis         // frame-slot spelling
#define leftWeight turnWeight  // frame-slot spelling
#define minWeight lowerCap     // frame-slot spelling
#define seedX startX           // frame-slot spelling
VA(0x0041143a, 0x601)
void editManager::PaintRandomTerrain(
    H1_ENUM_PARAM(TerrainType, i32) terrain,
    i32 percent,
    H1_ENUM_PARAM(TerrainType, i32) baseTerrain
) {
    i32 perSeed;
    i32 walkX;
    i32 walkY;
    i32 seedY;
    i32 guard;
    i32 targetCells;
    i32 horizontalWeight;
    i32 reserve;
    b32 looking;
    i32 stepCount;
    i32 maxWeight;
    i32 upWeight;
    i32 balance;
    i32 escapes;
    i32 patches;
    i32 leftWeight;
    i32 minWeight;
    i32 seedX;
    i32 placed;
    i32 cluster;

    if (percent == RANDOM_MAP_FULL_PERCENT) {
        for (walkX = 0; walkX < MAP_CELL_GRID_SIZE; walkX++)
            for (walkY = 0; walkY < MAP_CELL_GRID_SIZE; walkY++)
                m_map.cells[walkX][walkY].m_tileIndex =
                    H1_ENUM_ENCODE(TerrainType, terrain) * MAP_CELL_TILES_PER_TERRAIN;
    } else {
        targetCells = percent * (MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE) / RANDOM_MAP_FULL_PERCENT;
        patches = Random(0, percent + 51) / 30 + 1;
        balance = targetCells;
        escapes = 0;
        minWeight = gScatterTowns ? 2 : 3;
        maxWeight = (gScatterTowns != false) + 6;
        for (cluster = 0; cluster < patches; cluster++) {
            perSeed = balance / (patches - cluster);
            looking = true;
            guard = 0;
            while (guard < RANDOM_MAP_SEED_TRIES && looking) {
                guard++;
                if (gScatterTowns)
                    seedX = Random(0, MAP_CELL_GRID_SIZE - 1);
                else
                    seedX =
                        (Random(0, MAP_CELL_GRID_SIZE - 1) + Random(0, MAP_CELL_GRID_SIZE - 1)
                         + Random(0, MAP_CELL_GRID_SIZE - 1) + Random(0, MAP_CELL_GRID_SIZE - 1))
                        / 4;
                if (gScatterTowns) {
                    if (terrain == TERRAIN_DESERT || terrain == TERRAIN_LAVA)
                        seedY =
                            (Random(0, MAP_CELL_GRID_SIZE - 1) + Random(0, MAP_CELL_GRID_SIZE - 1)
                             + Random(0, MAP_CELL_GRID_SIZE - 1))
                            / 3;
                    else if (terrain == TERRAIN_SNOW)
                        seedY = Random(4, 20) + (Random(0, 1) ? 48 : 0);
                    else
                        seedY = Random(0, MAP_CELL_GRID_SIZE - 1);
                } else {
                    seedY =
                        (Random(0, MAP_CELL_GRID_SIZE - 1) + Random(0, MAP_CELL_GRID_SIZE - 1)
                         + Random(0, MAP_CELL_GRID_SIZE - 1) + Random(0, MAP_CELL_GRID_SIZE - 1))
                        / 4;
                }
                if (m_map.cells[seedX][seedY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                    == H1_ENUM_ENCODE(TerrainType, baseTerrain))
                    looking = false;
            }
            horizontalWeight = Random(3, 7);
            leftWeight = Random(3, 7);
            upWeight = Random(3, 7);
            reserve = perSeed * 1.5;
            stepCount = 0;
            for (placed = 0; placed < perSeed; placed++) {
                stepCount++;
                if ((stepCount & RANDOM_MAP_SEED_DRIFT_MASK) == RANDOM_MAP_SEED_DRIFT_MASK) {
                    seedX = seedX + Random(0, 2) - 1;
                    seedY = seedY + Random(0, 2) - 1;
                    if (seedX < 0)
                        seedX = 0;
                    if (seedX >= MAP_CELL_GRID_SIZE)
                        seedX = MAP_CELL_GRID_SIZE - 1;
                    if (seedY < 0)
                        seedY = 0;
                    if (seedY >= MAP_CELL_GRID_SIZE)
                        seedY = MAP_CELL_GRID_SIZE - 1;
                    if ((stepCount & RANDOM_MAP_WEIGHT_DRIFT_MASK)
                        == RANDOM_MAP_WEIGHT_DRIFT_MASK) {
                        horizontalWeight = horizontalWeight + Random(0, 2) - 1;
                        leftWeight = leftWeight + Random(0, 2) - 1;
                        upWeight = upWeight + Random(0, 2) - 1;
                        if (horizontalWeight < minWeight)
                            horizontalWeight = minWeight;
                        if (horizontalWeight > maxWeight)
                            horizontalWeight = maxWeight;
                        if (leftWeight < minWeight)
                            leftWeight = minWeight;
                        if (leftWeight > maxWeight)
                            leftWeight = maxWeight;
                        if (upWeight < minWeight)
                            upWeight = minWeight;
                        if (upWeight > maxWeight)
                            upWeight = maxWeight;
                    }
                }
                walkX = seedX;
                walkY = seedY;
                guard = 0;
                while (m_map.cells[walkX][walkY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                           == H1_ENUM_ENCODE(TerrainType, terrain)
                       && guard++ < RANDOM_MAP_WALK_LIMIT) {
                    if (Random(0, 9) < horizontalWeight) {
                        if (walkX == 0) {
                            walkX++;
                            if (leftWeight > maxWeight)
                                leftWeight = maxWeight;
                        } else if (walkX == MAP_CELL_GRID_SIZE - 1) {
                            walkX--;
                            if (leftWeight < minWeight)
                                leftWeight = minWeight;
                        } else if (Random(0, 9) < leftWeight)
                            walkX--;
                        else
                            walkX++;
                    } else {
                        if (walkY == 0) {
                            walkY++;
                            if (upWeight > maxWeight)
                                upWeight = maxWeight;
                        } else if (walkY == MAP_CELL_GRID_SIZE - 1) {
                            walkY--;
                            if (upWeight < minWeight)
                                upWeight = minWeight;
                        } else if (Random(0, 9) < upWeight)
                            walkY--;
                        else
                            walkY++;
                    }
                }
                if (guard >= RANDOM_MAP_WALK_LIMIT)
                    escapes++;
                if (m_map.cells[walkX][walkY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                    == H1_ENUM_ENCODE(TerrainType, baseTerrain))
                    m_map.cells[walkX][walkY].m_tileIndex =
                        H1_ENUM_ENCODE(TerrainType, terrain) * MAP_CELL_TILES_PER_TERRAIN;
                else if (reserve) {
                    reserve--;
                    placed--;
                } else {
                    balance++;
                    if (cluster + 1 == patches && patches < RANDOM_MAP_SEED_LIMIT)
                        patches++;
                }
            }
            balance -= perSeed;
        }
    }
}
#undef seedY
#undef targetCells
#undef horizontalWeight
#undef maxWeight
#undef upWeight
#undef leftWeight
#undef minWeight
#undef seedX

#define minX fromX     // frame-slot spelling
#define countX tallyX  // frame-slot spelling
#define y j            // frame-slot spelling
#define countY landY   // frame-slot spelling
#define startY homeY   // frame-slot spelling
#define maxY y1        // frame-slot spelling
#define x n            // frame-slot spelling
#define minY searchTop // frame-slot spelling
VA(0x00411a3b, 0x4bc)
void editManager::RemoveSmallRegions(void) {
    i32 minX;
    b32 spread;
    H1_ENUM_LOCAL(TerrainType, i32) neighbourTerrain;
    i8* done;
    i32 countX;
    i32 y;
    H1_ENUM_LOCAL(TerrainType, i32) ground;
    i32 extent;
    i32 countY;
    i32 startY;
    i32 maxX;
    i32 startX;
    i32 maxY;
    i8* inRegion;
    i32 x;
    i32 minY;

    done = static_cast<i8*>(malloc(MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE));
    inRegion = static_cast<i8*>(malloc(MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE));
    memset(done, 0, MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE);
    for (startY = 0; startY < MAP_CELL_GRID_SIZE; startY++) {
        for (startX = 0; startX < MAP_CELL_GRID_SIZE; startX++) {
            if (MAP_GRID_CELL(done, startX, startY))
                continue;
            memset(inRegion, 0, MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE);
            (MAP_GRID_CELL(inRegion, startX, startY))++;
            ground = CELL_TERRAIN(&m_map.cells[startX][startY]);
            spread = true;
            extent = 1;
            minX = startX - 1;
            maxX = startX + 1;
            minY = startY - 1;
            maxY = startY + 1;
            neighbourTerrain = TERRAIN_INVALID;
            while (spread) {
                spread = false;
                if (minX < 0)
                    minX = 0;
                if (maxX >= MAP_CELL_GRID_SIZE)
                    maxX = MAP_CELL_GRID_SIZE - 1;
                if (minY < 0)
                    minY = 0;
                if (maxY >= MAP_CELL_GRID_SIZE)
                    maxY = MAP_CELL_GRID_SIZE - 1;
                for (y = minY; y <= maxY; y++) {
                    for (x = minX; x <= maxX; x++) {
                        if (CELL_TERRAIN(&m_map.cells[x][y]) != ground) {
                            if (neighbourTerrain == TERRAIN_INVALID)
                                neighbourTerrain = CELL_TERRAIN(&m_map.cells[x][y]);
                            continue;
                        }
                        if (MAP_GRID_CELL(inRegion, x, y))
                            continue;
                        if (x < MAP_CELL_GRID_SIZE - 1
                            && *(inRegion + x + 1 + y * MAP_CELL_GRID_SIZE))
                            (MAP_GRID_CELL(inRegion, x, y))++;
                        else if (x > 0 && *(inRegion + x - 1 + y * MAP_CELL_GRID_SIZE))
                            (MAP_GRID_CELL(inRegion, x, y))++;
                        else if (y < MAP_CELL_GRID_SIZE - 1 && MAP_GRID_CELL(inRegion, x, y + 1))
                            (MAP_GRID_CELL(inRegion, x, y))++;
                        else if (y > 0 && MAP_GRID_CELL(inRegion, x, y - 1))
                            (MAP_GRID_CELL(inRegion, x, y))++;
                        else
                            continue;
                        spread = true;
                        extent++;
                        if (x == minX)
                            minX--;
                        if (x == maxX)
                            maxX++;
                        if (y == minY)
                            minX--;
                        if (y == maxY)
                            maxY++;
                        if (minX < 0)
                            minX = 0;
                        if (maxX >= MAP_CELL_GRID_SIZE)
                            maxX = MAP_CELL_GRID_SIZE - 1;
                        if (minY < 0)
                            minY = 0;
                        if (maxY >= MAP_CELL_GRID_SIZE)
                            maxY = MAP_CELL_GRID_SIZE - 1;
                    }
                }
            }
            if (extent > RANDOM_MAP_SMALL_REGION_SIZE)
                neighbourTerrain = ground;
            for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
                for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
                    if (MAP_GRID_CELL(inRegion, x, y)) {
                        MAP_GRID_CELL(done, x, y) = 1;
                        m_map.cells[x][y].m_tileIndex =
                            H1_ENUM_ENCODE(TerrainType, neighbourTerrain)
                            * MAP_CELL_TILES_PER_TERRAIN;
                    }
                }
            }
        }
    }
    free(done);
    free(inRegion);
    gLandCellCount = 0;
    for (countX = 0; countX < MAP_CELL_GRID_SIZE; countX++)
        for (countY = 0; countY < MAP_CELL_GRID_SIZE; countY++)
            if (m_map.cells[countX][countY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN)
                gLandCellCount++;
}
#undef minX
#undef countX
#undef y
#undef countY
#undef startY
#undef maxY
#undef x
#undef minY

VA(0x00411ef7, 0x62)
void ScaleByDensity(i32* count, i32 density) {
    i32 base;

    base = *count;
    *count = base * (base + 50) / RANDOM_MAP_FULL_PERCENT;
    if (density < RANDOM_MAP_NEUTRAL_DENSITY)
        *count = *count * (density + RANDOM_MAP_NEUTRAL_DENSITY) / RANDOM_MAP_FULL_PERCENT;
    else
        *count = *count * density / RANDOM_MAP_NEUTRAL_DENSITY;
}

#define chainCount ridges // frame-slot spelling
VA(0x00411f59, 0x3ce)
void editManager::PlaceObstacleChains(i32 density, H1_ENUM_PARAM(MapTileset, i32) tileset) {
    i32 chance;
    b32 going;
    H1_ENUM_LOCAL(ChainDirection, i32) direction;
    i32 unusedStep;
    i32 unusedMask;
    H1_ENUM_LOCAL(TerrainType, i32) ground;
    i32 budget;
    i32 rootX;
    char treeFamily;
    i32 chainCount;
    i32 placed;
    i32 landCells;
    b32 hunting;
    i32 rootY;

    placed = 0;
    treeFamily = 0;
    ground = TERRAIN_WATER;
    landCells = 0;
    for (rootX = 0; rootX < MAP_CELL_GRID_SIZE; rootX++)
        for (rootY = 0; rootY < MAP_CELL_GRID_SIZE; rootY++)
            if (m_map.cells[rootX][rootY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN)
                landCells++;
    chainCount = landCells / 40;
    ScaleByDensity(&chainCount, density);
    budget = chainCount * 13;
    while (placed < budget) {
        hunting = true;
        while (hunting) {
            hunting = false;
            rootX = Random(0, MAP_CELL_GRID_SIZE - 1);
            rootY = Random(0, MAP_CELL_GRID_SIZE - 1);
            ground = CELL_TERRAIN(&m_map.cells[rootX][rootY]);
            if (tileset == TILESET_TREE32 && ground == TERRAIN_LAVA && Random(0, 100) < 80)
                hunting = true;
            if (tileset == TILESET_TREE32 && ground == TERRAIN_DESERT && Random(0, 100) < 70)
                hunting = true;
        }
        // An even (steep) direction; one in four turns shallow.
        direction = H1_ENUM_DECODE(ChainDirection, Random(0, 3) * 2);
        if (Random(1, 100) <= 25)
            direction++;
        going = true;
        treeFamily = 0;
        if (tileset == TILESET_TREE32) {
            switch (H1_ENUM_DECODE(ChainTreeFamily, Random(0, 2))) {
                case CHAIN_TREE_AUTUMN:
                    treeFamily = 'a';
                    break;
                case CHAIN_TREE_PINE:
                    treeFamily = 'p';
                    break;
                case CHAIN_TREE_DECIDUOUS:
                    treeFamily = 't';
                    break;
            }
            if (ground == TERRAIN_DIRT) {
                if (rootY < MAP_CELL_GRID_SIZE / 2)
                    treeFamily = 'p';
                else
                    treeFamily = 'a';
            }
            if (ground == TERRAIN_GRASS) {
                if (rootY < MAP_CELL_GRID_SIZE / 2)
                    treeFamily = 'p';
                else
                    treeFamily = 't';
            }
            if (ground == TERRAIN_DESERT)
                treeFamily = 'a';
            if (ground == TERRAIN_SNOW)
                treeFamily = 'p';
            if (ground == TERRAIN_LAVA)
                treeFamily = 't';
            if (ground == TERRAIN_SWAMP)
                treeFamily = 't';
        }
        while (going) {
            if (PlaceChainLink(&rootX, &rootY, direction, tileset, treeFamily)) {
                placed += 12;
                if (tileset == TILESET_TREE32) {
                    if (Random(1, 100) < (H1_ENUM_ENCODE(ChainDirection, direction) % 1 ? 30 : 10))
                        going = false;
                } else {
                    if (Random(1, 100) < (H1_ENUM_ENCODE(ChainDirection, direction) % 1 ? 40 : 20))
                        going = false;
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
                        rootX += gChainTurns[direction][CHAIN_TURN_CLOCKWISE][MAP_STEP_X];
                        rootY += gChainTurns[direction][CHAIN_TURN_CLOCKWISE][MAP_STEP_Y];
                        direction = H1_ENUM_DECODE(
                            ChainDirection,
                            (H1_ENUM_ENCODE(ChainDirection, direction) + CHAIN_CLOCKWISE_STEP)
                                % H1_ENUM_ENCODE(ChainDirection, CHAIN_DIRECTION_COUNT)
                        );
                    } else {
                        rootX += gChainTurns[direction][CHAIN_TURN_COUNTERCLOCKWISE][MAP_STEP_X];
                        rootY += gChainTurns[direction][CHAIN_TURN_COUNTERCLOCKWISE][MAP_STEP_Y];
                        direction = H1_ENUM_DECODE(
                            ChainDirection,
                            (H1_ENUM_ENCODE(ChainDirection, direction)
                             + CHAIN_COUNTERCLOCKWISE_STEP)
                                % H1_ENUM_ENCODE(ChainDirection, CHAIN_DIRECTION_COUNT)
                        );
                    }
                }
            } else {
                going = false;
                placed++;
            }
        }
    }
}
#undef chainCount

#define piece frame             // frame-slot spelling
#define anyTerrainChain generic // frame-slot spelling
#define terrainChain specific   // frame-slot spelling
#define terrainBit bit          // frame-slot spelling
VA(0x00412327, 0x303)
i32 editManager::PlaceChainLink(
    i32* x,
    i32* y,
    H1_ENUM_PARAM(ChainDirection, i32) direction,
    H1_ENUM_PARAM(MapTileset, i32) tileset,
    char treeFamily
) {
    H1_ENUM_LOCAL(ChainPiece, i32) piece;
    overlayType* anyTerrainChain;
    H1_ENUM_LOCAL(TerrainType, i32) ground;
    overlayType* terrainChain;
    i32 n;
    i32 terrainBit;

    if (*x < 0 || *x > MAP_CELL_GRID_SIZE - 1 || *y < 0 || *y > MAP_CELL_GRID_SIZE - 1)
        return 0;
    piece = CHAIN_PIECE_STEEP_RISING;
    ground = CELL_TERRAIN(&m_map.cells[*x][*y]);
    terrainBit = RANDOM_MAP_NO_CHAIN_TERRAIN;
    if (tileset == TILESET_MTN32) {
        switch (ground) {
            case TERRAIN_GRASS:
                terrainBit = H1_ENUM_BIT(TerrainType, TERRAIN_GRASS);
                break;
            case TERRAIN_SNOW:
                terrainBit = H1_ENUM_BIT(TerrainType, TERRAIN_SNOW);
                break;
            case TERRAIN_SWAMP:
                terrainBit = H1_ENUM_BIT(TerrainType, TERRAIN_SWAMP);
                break;
            case TERRAIN_DESERT:
                terrainBit = H1_ENUM_BIT(TerrainType, TERRAIN_DESERT);
                break;
            case TERRAIN_DIRT:
                terrainBit = H1_ENUM_BIT(TerrainType, TERRAIN_DIRT);
                break;
        }
    }
    if (tileset == TILESET_TREE32) {
        switch (ground) {
            case TERRAIN_SNOW:
                terrainBit = H1_ENUM_BIT(TerrainType, TERRAIN_SNOW);
                break;
        }
    }
    if (direction == CHAIN_UP_RIGHT_STEEP)
        piece = CHAIN_PIECE_STEEP_RISING;
    if (direction == CHAIN_UP_RIGHT)
        piece = CHAIN_PIECE_RISING;
    if (direction == CHAIN_DOWN_RIGHT_STEEP)
        piece = CHAIN_PIECE_STEEP_FALLING;
    if (direction == CHAIN_DOWN_RIGHT)
        piece = CHAIN_PIECE_FALLING;
    if (direction == CHAIN_DOWN_LEFT_STEEP)
        piece = CHAIN_PIECE_STEEP_RISING;
    if (direction == CHAIN_DOWN_LEFT)
        piece = CHAIN_PIECE_RISING;
    if (direction == CHAIN_UP_LEFT_STEEP)
        piece = CHAIN_PIECE_STEEP_FALLING;
    if (direction == CHAIN_UP_LEFT)
        piece = CHAIN_PIECE_FALLING;
    terrainChain = NULL;
    anyTerrainChain = NULL;
    for (n = 0; n < OVERLAY_TYPE_COUNT; n++) {
        if (!terrainChain && gOverlayTypes[n].tileset == tileset
            && gOverlayTypes[n].terrainMask == terrainBit)
            terrainChain = &gOverlayTypes[n + H1_ENUM_ENCODE(ChainPiece, piece)];
        if (!anyTerrainChain && gOverlayTypes[n].tileset == tileset
            && gOverlayTypes[n].terrainMask == RANDOM_MAP_ANY_TERRAIN
            && (!treeFamily || treeFamily == gOverlayTypes[n].name[0]))
            anyTerrainChain = &gOverlayTypes[n + H1_ENUM_ENCODE(ChainPiece, piece)];
    }
    if (terrainChain && CanPlaceOverlay(terrainChain, *x, *y)) {
        PlaceOverlay(terrainChain, *x, *y);
        *x += gChainSteps[direction][MAP_STEP_X];
        *y += gChainSteps[direction][MAP_STEP_Y];
        return 1;
    }
    if (CanPlaceOverlay(anyTerrainChain, *x, *y)) {
        PlaceOverlay(anyTerrainChain, *x, *y);
        *x += gChainSteps[direction][MAP_STEP_X];
        *y += gChainSteps[direction][MAP_STEP_Y];
        return 1;
    }
    return 0;
}
#undef piece
#undef anyTerrainChain
#undef terrainChain
#undef terrainBit

#define castleRegion belongs             // frame-slot spelling
#define stoneLiths anyGate               // frame-slot spelling
#define snowStoneLiths winterGate        // frame-slot spelling
#define desertStoneLiths desertGateLiths // frame-slot spelling
#define destY endY                       // frame-slot spelling
#define fromY y0                         // frame-slot spelling
#define foundX freeX                     // frame-slot spelling
#define nearY scanY                      // frame-slot spelling
VA(0x0041262a, 0x1f4c)
void editManager::PlaceTowns(void) {
    i32 terrain;
    i32 cutOff[RANDOM_MAP_CASTLE_SLOTS];
    i32 extraRoads[RANDOM_MAP_CASTLE_SLOTS];
    i32 nearX;
    i32 castleRegion[RANDOM_MAP_CASTLE_SLOTS];
    i32 tileX;
    i32 reachable[RANDOM_MAP_CASTLE_SLOTS];
    i32 regionId;
    double shareValue[RANDOM_MAP_REGION_LIMIT];
    i32 destY;
    overlayType* stoneLiths;
    b32 coastAt;
    i32 destX;
    i32 continents;
    mapStep keeps[RANDOM_MAP_CASTLE_SLOTS];
    b32 tracing;
    i32 fromX;
    i32 roadMaskSet;
    overlayType* snowStoneLiths;
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
    b32 filled;
    i32 stepX;
    i32 stepY;
    u8* regionGrid;
    i32 fromY;
    i16 regionSizes[RANDOM_MAP_REGION_LIMIT];
    b32 meet;
    i32 foundY;
    i32 top;
    overlayType* desertStoneLiths;
    i32 foundX;
    i32 nearY;
    i32 unusedTotal;
    u8* reachedGrids[RANDOM_MAP_CASTLE_SLOTS];

    for (slot = 0; slot < RANDOM_MAP_CASTLE_SLOTS; slot++) {
        cutOff[slot] = 0;
        extraRoads[slot] = 0;
    }
    castle = NULL;
    snowStoneLiths = NULL;
    desertStoneLiths = NULL;
    stoneLiths = NULL;
    for (slot = 0; slot < OVERLAY_TYPE_COUNT; slot++) {
        if (!strcmpi(gOverlayTypes[slot].name, "xcast   "))
            castle = &gOverlayTypes[slot];
        if (!strcmpi(gOverlayTypes[slot].name, "stgate  "))
            snowStoneLiths = &gOverlayTypes[slot];
        if (!strcmpi(gOverlayTypes[slot].name, "dtgate  "))
            desertStoneLiths = &gOverlayTypes[slot];
        if (!strcmpi(gOverlayTypes[slot].name, "xtgate  "))
            stoneLiths = &gOverlayTypes[slot];
    }
    regionGrid = static_cast<u8*>(malloc(MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE));
    memset(regionGrid, 0, MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE);
    for (slot = 0; slot < RANDOM_MAP_CASTLE_SLOTS; slot++) {
        reachedGrids[slot] = static_cast<u8*>(malloc(MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE));
        memset(reachedGrids[slot], 0, MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE);
    }
    continents = 0;
    for (regionId = 1; regionId < RANDOM_MAP_REGION_LIMIT; regionId++) {
        foundX = foundY = EDIT_NO_CELL;
        for (tileX = 0; tileX < MAP_CELL_GRID_SIZE; tileX++) {
            for (tileY = 0; tileY < MAP_CELL_GRID_SIZE; tileY++) {
                if (m_map.cells[tileX][tileY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                    && !MAP_GRID_CELL(regionGrid, tileX, tileY)) {
                    continents++;
                    foundX = tileX;
                    foundY = tileY;
                    MAP_GRID_CELL(regionGrid, tileX, tileY) = regionId;
                    tileX = tileY = EDIT_END_SCAN;
                }
            }
        }
        if (foundX >= 0) {
            filled = true;
            while (filled) {
                filled = false;
                for (tileX = 0; tileX < MAP_CELL_GRID_SIZE; tileX++) {
                    for (tileY = 0; tileY < MAP_CELL_GRID_SIZE; tileY++) {
                        if (m_map.cells[tileX][tileY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                            && !MAP_GRID_CELL(regionGrid, tileX, tileY)) {
                            if (tileX > 0
                                && *(regionGrid + tileX - 1 + tileY * MAP_CELL_GRID_SIZE) > 0)
                                MAP_GRID_CELL(regionGrid, tileX, tileY) =
                                    *(regionGrid + tileX - 1 + tileY * MAP_CELL_GRID_SIZE);
                            else if (tileX < MAP_CELL_GRID_SIZE - 1
                                     && *(regionGrid + tileX + 1 + tileY * MAP_CELL_GRID_SIZE) > 0)
                                MAP_GRID_CELL(regionGrid, tileX, tileY) =
                                    *(regionGrid + tileX + 1 + tileY * MAP_CELL_GRID_SIZE);
                            else if (tileY > 0 && MAP_GRID_CELL(regionGrid, tileX, tileY - 1) > 0)
                                MAP_GRID_CELL(regionGrid, tileX, tileY) =
                                    MAP_GRID_CELL(regionGrid, tileX, tileY - 1);
                            else if (tileY < MAP_CELL_GRID_SIZE - 1
                                     && MAP_GRID_CELL(regionGrid, tileX, tileY + 1) > 0)
                                MAP_GRID_CELL(regionGrid, tileX, tileY) =
                                    MAP_GRID_CELL(regionGrid, tileX, tileY + 1);
                            if (MAP_GRID_CELL(regionGrid, tileX, tileY))
                                filled = true;
                        }
                    }
                }
            }
        } else
            regionId = EDIT_END_SCAN;
    }
    memset(regionSizes, 0, sizeof(regionSizes));
    for (tileX = 0; tileX < MAP_CELL_GRID_SIZE; tileX++)
        for (tileY = 0; tileY < MAP_CELL_GRID_SIZE; tileY++)
            regionSizes[MAP_GRID_CELL(regionGrid, tileX, tileY)]++;
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
        castleRegion[0] = castleRegion[1] = castleRegion[2] = castleRegion[3] = rank[1];
        regionsUsed = 1;
    } else if (shareValue[rank[1]] > 40.0 && shareValue[rank[2]] < 15.0) {
        castleRegion[0] = castleRegion[1] = castleRegion[2] = castleRegion[3] = rank[1];
        regionsUsed = 1;
    } else if (shareValue[rank[1]] < 55.0 && shareValue[rank[2]] > 25.0) {
        castleRegion[0] = castleRegion[1] = rank[1];
        castleRegion[2] = castleRegion[3] = rank[2];
        regionsUsed = 2;
    } else if (shareValue[rank[1]] < 50.0 && shareValue[rank[2]] > 15.0
               && shareValue[rank[3]] > 15.0 && shareValue[rank[4]] > 15.0) {
        castleRegion[0] = rank[1];
        castleRegion[1] = rank[2];
        castleRegion[2] = rank[3];
        castleRegion[3] = rank[4];
        regionsUsed = RANDOM_MAP_CASTLE_SLOTS;
    } else if (shareValue[rank[1]] < 30.0 && shareValue[rank[2]] > 8.0 && shareValue[rank[3]] > 8.0
               && shareValue[rank[4]] > 8.0) {
        castleRegion[0] = rank[1];
        castleRegion[1] = rank[2];
        castleRegion[2] = rank[3];
        castleRegion[3] = rank[4];
        regionsUsed = RANDOM_MAP_CASTLE_SLOTS;
    } else {
        castleRegion[0] = castleRegion[1] = castleRegion[2] = castleRegion[3] = rank[1];
        regionsUsed = 1;
    }
    ShowStatusText(localization::Tr("editor.random.status.castles"));
    for (c = 0; c < RANDOM_MAP_CASTLE_SLOTS; c++) {
        top = 0;
        for (tileX = 2; tileX < MAP_CELL_GRID_SIZE - 3; tileX++) {
            for (tileY = 4; tileY < MAP_CELL_GRID_SIZE - 3; tileY++) {
                if (MAP_GRID_CELL(regionGrid, tileX, tileY) == castleRegion[c]) {
                    rating = Random(1000, 1200);
                    if (m_map.cells[tileX - 1][tileY + 1].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)
                        rating += regionsUsed > 1 ? 1000 : continents * 50 + 100;
                    else if (m_map.cells[tileX - 2][tileY + 2].m_tileIndex
                             < MAP_CELL_TILES_PER_TERRAIN)
                        rating += regionsUsed > 1 ? 1000 : continents * 40 + 100;
                    for (nearX = tileX - TOWN_FOOTPRINT_LEFT; nearX <= tileX + 1; nearX++)
                        for (nearY = tileY - TOWN_FOOTPRINT_TOP; nearY <= tileY; nearY++)
                            if (MAP_GRID_CELL(regionGrid, nearX, nearY) == castleRegion[c])
                                rating += 50;
                    if (rating > top) {
                        for (nearX = 0; nearX < MAP_CELL_GRID_SIZE - 1; nearX++) {
                            for (nearY = 0; nearY < MAP_CELL_GRID_SIZE - 1; nearY++) {
                                if (H1_ENUM_DECODE(
                                        MapTileset,
                                        m_map.cells[nearX][nearY].m_objectTileset
                                    ) == TILESET_TOWN32
                                    && m_map.cells[nearX][nearY].m_triggerType
                                           & MAP_TRIGGER_EVENT) {
                                    dist = abs(nearX - tileX) + abs(nearY - tileY);
                                    if (m_map.cells[nearX][nearY].m_triggerType
                                        == MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_CASTLE)) {
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
        gEditManager->ClearArea(
            tileX - RANDOM_MAP_SITE_LEFT,
            tileY - RANDOM_MAP_SITE_TOP,
            RANDOM_MAP_SITE_WIDTH,
            RANDOM_MAP_SITE_HEIGHT,
            EDIT_CLEAR_ALL,
            true
        );
        for (nearX = tileX - RANDOM_MAP_SITE_LEFT; nearX <= tileX + RANDOM_MAP_SITE_RIGHT; nearX++)
            for (nearY = tileY - RANDOM_MAP_SITE_TOP; nearY <= tileY + RANDOM_MAP_SITE_BOTTOM;
                 nearY++)
                m_map.cells[nearX][nearY].m_tileIndex = terrain * MAP_CELL_TILES_PER_TERRAIN;
        PlaceOverlay(castle, tileX - TOWN_FOOTPRINT_LEFT, tileY);
        if (regionsUsed > 1)
            steps = RANDOM_MAP_UNLIMITED_STEPS;
        else if (m_map.cells[tileX - 1][tileY + 1].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)
            steps = 1;
        else if (m_map.cells[tileX - 2][tileY + 2].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)
            steps = 3;
        else
            steps = 0;
        if (steps) {
            coastAt = false;
            nearX = tileX - 2;
            nearY = tileY + 1;
            ResetArea(tileX - 2, tileY + 1, 2, 2);
            while (!coastAt && steps) {
                steps--;
                if (H1_ENUM_DECODE(
                        TerrainType,
                        m_map.cells[nearX][nearY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                    ) == TERRAIN_WATER
                    || H1_ENUM_DECODE(
                           TerrainType,
                           m_map.cells[nearX + 1][nearY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                       ) == TERRAIN_WATER)
                    coastAt = true;
                else {
                    m_map.cells[nearX][nearY].m_tileIndex =
                        H1_ENUM_ENCODE(TerrainType, TERRAIN_WATER) * MAP_CELL_TILES_PER_TERRAIN;
                    m_map.cells[nearX + 1][nearY].m_tileIndex =
                        H1_ENUM_ENCODE(TerrainType, TERRAIN_WATER) * MAP_CELL_TILES_PER_TERRAIN;
                    if (nearX > 0)
                        nearX--;
                    if (nearY < MAP_CELL_GRID_SIZE - 1)
                        nearY++;
                    if (nearX == 0 && nearY == MAP_CELL_GRID_SIZE - 1)
                        coastAt = true;
                }
            }
        }
        // Roads run to every earlier castle when this castle's region is the
        // previous castle's; a road's end row is taken from the peer castle's
        // column.
        for (peerIndex = 0; peerIndex < c; peerIndex++) {
            if (castleRegion[c] != castleRegion[c - 1])
                continue;
            if (keeps[peerIndex].x < keeps[c].x && keeps[peerIndex].y < keeps[c].y) {
                fromX = tileX - 2;
                fromY = tileY - 2;
                destX = keeps[peerIndex].x + 1;
                destY = keeps[peerIndex].x + 1;
            } else if (keeps[peerIndex].x < keeps[c].x && keeps[peerIndex].y >= keeps[c].y) {
                fromX = tileX + 1;
                fromY = tileY + 1;
                destX = keeps[peerIndex].x + 1;
                destY = keeps[peerIndex].x - 2;
            } else if (keeps[peerIndex].x >= keeps[c].x && keeps[peerIndex].y < keeps[c].y) {
                fromX = tileX + 1;
                fromY = tileY - 2;
                destX = keeps[peerIndex].x + 1;
                destY = keeps[peerIndex].x + 1;
            } else {
                fromX = tileX + 1;
                fromY = tileY + 1;
                destX = keeps[peerIndex].x - 1;
                destY = keeps[peerIndex].x - 2;
            }
            nearX = fromX;
            nearY = fromY;
            tracing = true;
            while (tracing) {
                if (nearX == destX && nearY == destY)
                    tracing = false;
                if (destX > nearX)
                    stepX = 1;
                else if (destX < nearX)
                    stepX = -1;
                else
                    stepX = 0;
                if (destY > nearY)
                    stepY = 1;
                else if (destY < nearY)
                    stepY = -1;
                else
                    stepY = 0;
                if (stepX
                    && m_map.cells[nearX + stepX][nearY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN)
                    stepY = 0;
                else if (stepY
                         && m_map.cells[nearX][nearY + stepY].m_tileIndex
                                >= MAP_CELL_TILES_PER_TERRAIN)
                    stepX = 0;
                else {
                    tracing = false;
                    cutOff[c] = 1;
                    cutOff[peerIndex] = 1;
                }
                if (tracing
                    && m_map.cells[nearX + stepX][nearY + stepY].m_tileIndex
                           >= MAP_CELL_TILES_PER_TERRAIN) {
                    nearX += stepX;
                    nearY += stepY;
                    roadMaskSet = EDIT_CLEAR_ROAD;
                    gEditManager->ClearArea(nearX, nearY, 1, 1, roadMaskSet, false);
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
            for (tileX = keeps[slot].x - RANDOM_MAP_SITE_LEFT;
                 tileX <= keeps[slot].x + RANDOM_MAP_SITE_RIGHT;
                 tileX++)
                for (tileY = keeps[slot].y - RANDOM_MAP_SITE_TOP;
                     tileY <= keeps[slot].y + RANDOM_MAP_SITE_BOTTOM;
                     tileY++)
                    MAP_GRID_CELL(reachedGrids[slot], tileX, tileY) = 1;
            filled = true;
            while (filled) {
                filled = false;
                for (tileX = 0; tileX < MAP_CELL_GRID_SIZE; tileX++) {
                    for (tileY = 0; tileY < MAP_CELL_GRID_SIZE; tileY++) {
                        if (m_map.cells[tileX][tileY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                            && (!m_map.cells[tileX][tileY].m_objectTileset
                                || H1_ENUM_DECODE(
                                       MapTileset,
                                       m_map.cells[tileX][tileY].m_objectTileset
                                   ) == TILESET_MONS32)
                            && !MAP_GRID_CELL(reachedGrids[slot], tileX, tileY)) {
                            if (tileX > 0
                                && *(reachedGrids[slot] + tileX - 1 + tileY * MAP_CELL_GRID_SIZE)
                                       > 0)
                                MAP_GRID_CELL(reachedGrids[slot], tileX, tileY) =
                                    *(reachedGrids[slot] + tileX - 1 + tileY * MAP_CELL_GRID_SIZE);
                            else if (tileX < MAP_CELL_GRID_SIZE - 1
                                     && *(reachedGrids[slot] + tileX + 1
                                          + tileY * MAP_CELL_GRID_SIZE)
                                            > 0)
                                MAP_GRID_CELL(reachedGrids[slot], tileX, tileY) =
                                    *(reachedGrids[slot] + tileX + 1 + tileY * MAP_CELL_GRID_SIZE);
                            else if (tileY > 0
                                     && MAP_GRID_CELL(reachedGrids[slot], tileX, tileY - 1) > 0)
                                MAP_GRID_CELL(reachedGrids[slot], tileX, tileY) =
                                    MAP_GRID_CELL(reachedGrids[slot], tileX, tileY - 1);
                            else if (tileY < MAP_CELL_GRID_SIZE - 1
                                     && MAP_GRID_CELL(reachedGrids[slot], tileX, tileY + 1) > 0)
                                MAP_GRID_CELL(reachedGrids[slot], tileX, tileY) =
                                    MAP_GRID_CELL(reachedGrids[slot], tileX, tileY + 1);
                            if (MAP_GRID_CELL(reachedGrids[slot], tileX, tileY)) {
                                filled = true;
                                reachable[slot]++;
                            }
                        }
                    }
                }
            }
        }
        for (slot = 1; slot < RANDOM_MAP_CASTLE_SLOTS; slot++) {
            for (t = 0; t < slot; t++) {
                if (castleRegion[slot] == castleRegion[t]) {
                    meet = false;
                    for (tileX = 0; tileX < MAP_CELL_GRID_SIZE; tileX++)
                        for (tileY = 0; tileY < MAP_CELL_GRID_SIZE; tileY++)
                            if (MAP_GRID_CELL(reachedGrids[slot], tileX, tileY)
                                && MAP_GRID_CELL(reachedGrids[t], tileX, tileY))
                                meet = true;
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
                tracing = true;
                while (tracing) {
                    if (steps-- < 0)
                        tracing = false;
                    tileX = Random(0, MAP_CELL_GRID_SIZE - 1);
                    tileY = Random(0, MAP_CELL_GRID_SIZE - 1);
                    if (MAP_GRID_CELL(reachedGrids[slot], tileX, tileY)) {
                        dist = abs(tileX - keeps[slot].x) + abs(tileY - keeps[slot].y);
                        if (steps < 10000 || Random(0, 100) < dist) {
                            tracing = false;
                            for (nearX = 0; nearX < MAP_CELL_GRID_SIZE; nearX++) {
                                for (nearY = 0; nearY < MAP_CELL_GRID_SIZE; nearY++) {
                                    if (m_map.cells[nearX][nearY].m_triggerType
                                        == MAP_EVENT_TRIGGER(MAP_OBJECT_STONE_LITHS)) {
                                        dist = abs(tileX - nearX) + abs(tileY - nearY);
                                        if (steps > 10000 && dist < 40 && Random(0, 100) > dist)
                                            tracing = true;
                                    }
                                }
                            }
                        }
                    }
                }
                gEditManager->ClearArea(tileX, tileY, 1, 1, EDIT_CLEAR_ALL, true);
                if (H1_ENUM_DECODE(
                        TerrainType,
                        m_map.cells[tileX][tileY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                    )
                    == TERRAIN_DESERT)
                    PlaceOverlay(desertStoneLiths, tileX, tileY);
                if (H1_ENUM_DECODE(
                        TerrainType,
                        m_map.cells[tileX][tileY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                    )
                    == TERRAIN_SNOW)
                    PlaceOverlay(snowStoneLiths, tileX, tileY);
                else
                    PlaceOverlay(stoneLiths, tileX, tileY);
            }
        }
    }
    free(regionGrid);
    for (slot = 0; slot < RANDOM_MAP_CASTLE_SLOTS; slot++)
        free(reachedGrids[slot]);
}
#undef castleRegion
#undef stoneLiths
#undef snowStoneLiths
#undef desertStoneLiths
#undef destY
#undef fromY
#undef foundX
#undef nearY

VA(0x00414576, 0x21f)
void editManager::PlaceResourceSite(i32 x, i32 y, H1_ENUM_PARAM(ResourceType, i32) resource) {
    overlayType* resourceMarker;
    overlayType* site;
    i32 ground;
    i32 index;
    char name[12];

    site = NULL;
    resourceMarker = NULL;
    ground = m_map.cells[x][y].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN;
    if (resource == RESOURCE_WOOD) {
        for (index = 0; index < OVERLAY_TYPE_COUNT; index++)
            if (!strcmpi(gOverlayTypes[index].name, "sawmill "))
                site = &gOverlayTypes[index];
        PlaceOverlay(site, x, y);
    } else if (resource == RESOURCE_MERCURY) {
        for (index = 0; index < OVERLAY_TYPE_COUNT; index++)
            if (!strnicmp(gOverlayTypes[index].name, "alch-0", 6)
                && gOverlayTypes[index].terrainMask & OVERLAY_TERRAIN_BIT(ground))
                site = &gOverlayTypes[index];
        PlaceOverlay(site, x, y);
    } else {
        sprintf(name, "rovr-0%d ", H1_ENUM_ENCODE(ResourceType, resource));
        for (index = 0; index < OVERLAY_TYPE_COUNT; index++) {
            if (!strnicmp(gOverlayTypes[index].name, "mine-0", 6)
                && gOverlayTypes[index].terrainMask & OVERLAY_TERRAIN_BIT(ground))
                site = &gOverlayTypes[index];
            if (!strcmpi(gOverlayTypes[index].name, name))
                resourceMarker = &gOverlayTypes[index];
        }
        PlaceOverlay(site, x, y);
        PlaceMineResource(resourceMarker, x + 1, y, true);
    }
}

#define siteResource siteIndex    // frame-slot spelling
#define townTries towns           // frame-slot spelling
#define obeliskTries obeliskTurns // frame-slot spelling
#define randomTown townObject     // frame-slot spelling
VA(0x00414795, 0xbd6)
void editManager::PlaceRandomObjects(i32 density, i32 strength) {
    b32 valid;
    H1_ENUM_LOCAL(ResourceType, i32) siteResource;
    overlayType* obeliskTypes[EDITOR_TERRAIN_COUNT];
    i32 attempts;
    overlayType* strong;
    i32 townTries;
    i32 obeliskTries;
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
    H1_ENUM_ARRAY(i32, quota, ResourceType, RESOURCE_COUNT);
    i32 laid;
    overlayType* anyMonster;
    overlayType* medium;
    overlayType* randomTown;

    anyMonster = NULL;
    weak = NULL;
    medium = NULL;
    strong = NULL;
    veryStrong = NULL;
    randomTown = NULL;
    laid = 0;
    ScatterDecorations();
    for (entry = 0; entry < EDITOR_TERRAIN_COUNT; entry++)
        obeliskTypes[entry] = NULL;
    for (entry = 0; entry < OVERLAY_TYPE_COUNT; entry++) {
        if (!strcmpi(gOverlayTypes[entry].name, "xtown   "))
            randomTown = &gOverlayTypes[entry];
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
            obeliskTypes[gOverlayTypes[entry].name[RANDOM_MAP_OBELISK_NAME_LENGTH] - '0'] =
                &gOverlayTypes[entry];
    }
    townTries = gLandCellCount / 640;
    if (townTries > 12)
        townTries = 12;
    if (townTries < 1)
        townTries = 1;
    townTries *= RANDOM_MAP_TRIES_PER_OBJECT;
    while (townTries > 0) {
        townTries--;
        x = Random(2, 69);
        y = Random(2, 70);
        valid = true;
        for (i = -RANDOM_MAP_SITE_LEFT; i <= RANDOM_MAP_SITE_RIGHT; i++) {
            for (j = -RANDOM_MAP_SITE_TOP; j <= RANDOM_MAP_SITE_BOTTOM; j++) {
                if (m_map.cells[x + i][y + j].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
                    || H1_ENUM_DECODE(MapTileset, m_map.cells[x + i][y + j].m_objectTileset)
                           == TILESET_TOWN32
                    || H1_ENUM_DECODE(MapTileset, m_map.cells[x + i][y + j].m_overlayTileset)
                           == TILESET_TOWN32)
                    valid = false;
            }
        }
        for (i = 0; i < MAP_CELL_GRID_SIZE - 1; i++) {
            for (j = 0; j < MAP_CELL_GRID_SIZE - 1; j++) {
                if (H1_ENUM_DECODE(MapTileset, m_map.cells[i][j].m_objectTileset) == TILESET_TOWN32
                    && m_map.cells[i][j].m_triggerType & MAP_TRIGGER_EVENT) {
                    spacing = abs(i - x) + abs(j - y);
                    if (spacing < 10 || spacing < Random(0, 40))
                        valid = false;
                }
            }
        }
        if (valid) {
            townTries -= RANDOM_MAP_TRIES_PER_OBJECT;
            gEditManager->ClearArea(
                x - RANDOM_MAP_SITE_LEFT,
                y - RANDOM_MAP_SITE_TOP,
                RANDOM_MAP_SITE_WIDTH,
                RANDOM_MAP_SITE_HEIGHT,
                EDIT_CLEAR_ALL,
                true
            );
            PlaceOverlay(randomTown, x - TOWN_FOOTPRINT_LEFT, y);
        }
    }
    mineTries = gLandCellCount / 140;
    ScaleByDensity(&mineTries, density);
    quota[RESOURCE_WOOD] = (mineTries - 13) / 7 + 5;
    quota[RESOURCE_ORE] = (mineTries - 13) / 7 + 5;
    quota[RESOURCE_GOLD] = (mineTries - 13) / 7 + 2;
    quota[RESOURCE_GEMS] = (mineTries - 13) / 7 + 2;
    quota[RESOURCE_CRYSTAL] = (mineTries - 13) / 7 + 2;
    quota[RESOURCE_SULFUR] = (mineTries - 13) / 7 + 2;
    quota[RESOURCE_MERCURY] = (mineTries - 13) / 7 + 2;
    if (mineTries > OVERLAY_MINE_LIMIT)
        mineTries = OVERLAY_MINE_LIMIT;
    if (mineTries < 6)
        mineTries = 6;
    mineTries *= RANDOM_MAP_TRIES_PER_MINE;
    while (mineTries > 0) {
        mineTries--;
        valid = true;
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
            valid = true;
            while (valid && attempts < 10) {
                attempts++;
                siteResource = H1_ENUM_DECODE(
                    ResourceType,
                    Random(0, H1_ENUM_ENCODE(ResourceType, RESOURCE_LAST))
                );
                appeal = 4;
                if (quota[siteResource] > 0)
                    appeal += 30;
                if (siteResource == RESOURCE_WOOD)
                    appeal += 8;
                if (siteResource == RESOURCE_ORE)
                    appeal += 8;
                for (i = 0; i < MAP_CELL_GRID_SIZE; i++) {
                    for (j = 0; j < MAP_CELL_GRID_SIZE; j++) {
                        if (m_map.cells[i][j].m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_MINE)
                            || m_map.cells[i][j].m_triggerType
                                   == MAP_EVENT_TRIGGER(MAP_OBJECT_SAWMILL)
                            || m_map.cells[i][j].m_triggerType
                                   == MAP_EVENT_TRIGGER(MAP_OBJECT_ALCHEMIST_LAB)) {
                            spacing = abs(i - x) + abs(j - y);
                            if (spacing < 10)
                                appeal -= 10 - spacing;
                            if (siteResource == RESOURCE_WOOD
                                    && m_map.cells[i][j].m_triggerType
                                           == MAP_EVENT_TRIGGER(MAP_OBJECT_SAWMILL)
                                || siteResource == RESOURCE_MERCURY
                                       && m_map.cells[i][j].m_triggerType
                                              == MAP_EVENT_TRIGGER(MAP_OBJECT_ALCHEMIST_LAB)
                                || m_map.cells[i][j].m_triggerType
                                           == MAP_EVENT_TRIGGER(MAP_OBJECT_MINE)
                                       && gMineSiteKinds
                                                  [m_map.cells[i + 1][j].m_extraFrame
                                                   % RANDOM_MAP_MINE_RESOURCE_COUNT]
                                              == siteResource) {
                                if (spacing < 15)
                                    appeal -= (15 - spacing) * 2;
                            }
                        }
                    }
                }
                if (Random(0, 40) < appeal) {
                    gEditManager->ClearArea(x, y - 1, 2, 2, EDIT_CLEAR_ALL, false);
                    gEditManager->ClearArea(x - 1, y + 1, 1, 1, EDIT_CLEAR_ALL, false);
                    PlaceResourceSite(x, y, siteResource);
                    laid++;
                    quota[siteResource]--;
                    if (Random(0, 100) < strength)
                        PlaceOverlay(Random(0, 100) < 30 ? medium : strong, x - 1, y + 1);
                    mineTries -= RANDOM_MAP_TRIES_PER_MINE;
                    valid = false;
                }
            }
        }
    }
    obeliskTries = gLandCellCount / 180;
    if (obeliskTries > 24)
        obeliskTries = 24;
    if (obeliskTries < 8)
        obeliskTries = 8;
    obeliskTries *= RANDOM_MAP_TRIES_PER_OBJECT;
    while (obeliskTries > 0) {
        obeliskTries--;
        valid = true;
        x = Random(0, MAP_CELL_GRID_SIZE - 1);
        y = Random(0, MAP_CELL_GRID_SIZE - 1);
        if (m_map.cells[x][y].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN) {
            ground = m_map.cells[x][y].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN;
            if (CanPlaceOverlay(obeliskTypes[ground], x, y)) {
                obeliskTries -= RANDOM_MAP_TRIES_PER_OBJECT;
                PlaceOverlay(obeliskTypes[ground], x, y);
            }
        }
    }
}
#undef siteResource
#undef townTries
#undef obeliskTries
#undef randomTown

#define treasureTries caches   // frame-slot spelling
#define southBlocked down      // frame-slot spelling
#define eastBlocked right      // frame-slot spelling
#define northBlocked up        // frame-slot spelling
#define westBlocked left       // frame-slot spelling
#define randomResource goods   // frame-slot spelling
#define campfire bonfire       // frame-slot spelling
#define monsterTries wanderers // frame-slot spelling
#define randomArtifact bounty  // frame-slot spelling
#define guard layout           // frame-slot spelling
VA(0x0041536b, 0xd32)
void editManager::PlaceTreasures(i32 density, i32 strength) {
    overlayType* chest;
    overlayType* genieLamp;
    i32 treasureTries;
    b32 seOpen;
    b32 southBlocked;
    b32 neOpen;
    b32 eastBlocked;
    overlayType* veryStrong;
    i32 kindRoll;
    overlayType* strongMonster;
    overlayType* weak;
    overlayType* anyMonster;
    overlayType* medium;
    i32 curX;
    b32 nwOpen;
    b32 swOpen;
    overlayType* randomResource;
    overlayType* campfire;
    b32 northBlocked;
    i32 pilesSoFar;
    i32 monsterTries;
    i32 k;
    i32 curY;
    overlayType* randomArtifact;
    i32 guardsDone;
    H1_ENUM_LOCAL(TreasureGuard, i32) guard;
    b32 westBlocked;

    anyMonster = NULL;
    weak = NULL;
    medium = NULL;
    strongMonster = NULL;
    veryStrong = NULL;
    randomResource = NULL;
    randomArtifact = NULL;
    chest = NULL;
    campfire = NULL;
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
            randomResource = &gOverlayTypes[k];
        if (!strcmpi(gOverlayTypes[k].name, "xart    "))
            randomArtifact = &gOverlayTypes[k];
        if (!strcmpi(gOverlayTypes[k].name, "chest   "))
            chest = &gOverlayTypes[k];
        if (!strcmpi(gOverlayTypes[k].name, "lamp    "))
            genieLamp = &gOverlayTypes[k];
        if (!strcmpi(gOverlayTypes[k].name, "firemult"))
            campfire = &gOverlayTypes[k];
    }
    treasureTries = gLandCellCount / 40;
    ScaleByDensity(&treasureTries, density);
    treasureTries *= RANDOM_MAP_TRIES_PER_OBJECT;
    pilesSoFar = 0;
    guardsDone = 0;
    while (treasureTries > 0) {
        treasureTries--;
        curX = Random(1, 70);
        curY = Random(1, 70);
        if (m_map.cells[curX][curY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
            && !m_map.cells[curX][curY].m_objectTileset) {
            gEditManager->ClearArea(curX, curY, 1, 1, EDIT_CLEAR_ALL, false);
            kindRoll = Random(0, 100);
            nwOpen = swOpen = neOpen = seOpen = northBlocked = southBlocked = eastBlocked =
                westBlocked = false;
            if (curY == 0 || m_map.cells[curX][curY - 1].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
                || H1_ENUM_DECODE(MapTileset, m_map.cells[curX][curY - 1].m_objectTileset)
                           <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX][curY - 1].m_objectTileset > 0
                || H1_ENUM_DECODE(MapTileset, m_map.cells[curX][curY - 1].m_overlayTileset)
                           <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX][curY - 1].m_overlayTileset > 0)
                northBlocked = true;
            if (curY == MAP_CELL_GRID_SIZE - 1
                || m_map.cells[curX][curY + 1].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
                || H1_ENUM_DECODE(MapTileset, m_map.cells[curX][curY + 1].m_objectTileset)
                           <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX][curY + 1].m_objectTileset > 0
                || H1_ENUM_DECODE(MapTileset, m_map.cells[curX][curY + 1].m_overlayTileset)
                           <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX][curY + 1].m_overlayTileset > 0)
                southBlocked = true;
            if (curX == 0 || m_map.cells[curX - 1][curY].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
                || H1_ENUM_DECODE(MapTileset, m_map.cells[curX - 1][curY].m_objectTileset)
                           <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX - 1][curY].m_objectTileset > 0
                || H1_ENUM_DECODE(MapTileset, m_map.cells[curX - 1][curY].m_overlayTileset)
                           <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX - 1][curY].m_overlayTileset > 0)
                westBlocked = true;
            if (curX == MAP_CELL_GRID_SIZE - 1
                || m_map.cells[curX + 1][curY].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
                || H1_ENUM_DECODE(MapTileset, m_map.cells[curX + 1][curY].m_objectTileset)
                           <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX + 1][curY].m_objectTileset > 0
                || H1_ENUM_DECODE(MapTileset, m_map.cells[curX + 1][curY].m_overlayTileset)
                           <= TILESET_TERRAIN_OBJECT_LAST
                       && m_map.cells[curX + 1][curY].m_overlayTileset > 0)
                eastBlocked = true;
            if (curX < MAP_CELL_GRID_SIZE + 1 && curY > 0
                && m_map.cells[curX + 1][curY - 1].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                && !m_map.cells[curX + 1][curY - 1].m_objectTileset)
                neOpen = true;
            if (curX < MAP_CELL_GRID_SIZE + 1 && curY < MAP_CELL_GRID_SIZE - 1
                && m_map.cells[curX + 1][curY + 1].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                && !m_map.cells[curX + 1][curY + 1].m_objectTileset)
                seOpen = true;
            if (curX > 0 && curY < MAP_CELL_GRID_SIZE - 1
                && m_map.cells[curX - 1][curY + 1].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                && !m_map.cells[curX - 1][curY + 1].m_objectTileset)
                swOpen = true;
            if (curX > 0 && curY > 0
                && m_map.cells[curX - 1][curY - 1].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
                && !m_map.cells[curX - 1][curY - 1].m_objectTileset)
                nwOpen = true;
            if (!eastBlocked || !westBlocked || !northBlocked || !southBlocked) {
                pilesSoFar++;
                treasureTries -= RANDOM_MAP_TRIES_PER_OBJECT;
                guard = TREASURE_UNGUARDED;
                if (guardsDone * 6 < pilesSoFar && guardsDone < 31) {
                    if (westBlocked && southBlocked && neOpen && !swOpen && !seOpen && !nwOpen)
                        guard = TREASURE_GUARD_NE;
                    else if (westBlocked && northBlocked && seOpen && !nwOpen && !swOpen && !neOpen)
                        guard = TREASURE_GUARD_SE;
                    else if (eastBlocked && northBlocked && swOpen && !neOpen && !nwOpen && !seOpen)
                        guard = TREASURE_GUARD_SW;
                    else if (eastBlocked && southBlocked && nwOpen && !seOpen && !swOpen && !neOpen)
                        guard = TREASURE_GUARD_NW;
                }
                if (guard > TREASURE_UNGUARDED_LAST) {
                    guardsDone++;
                    if (kindRoll < 35)
                        PlaceOverlay(chest, curX, curY);
                    else
                        PlaceOverlay(randomArtifact, curX, curY);
                    if (guard == TREASURE_GUARD_NE) {
                        gEditManager->ClearArea(curX + 1, curY - 1, 1, 1, EDIT_CLEAR_ALL, false);
                        PlaceOverlay(
                            Random(0, 100) < 50 ? strongMonster : veryStrong,
                            curX + 1,
                            curY - 1
                        );
                    } else if (guard == TREASURE_GUARD_SE) {
                        gEditManager->ClearArea(curX + 1, curY + 1, 1, 1, EDIT_CLEAR_ALL, false);
                        PlaceOverlay(
                            Random(0, 100) < 50 ? strongMonster : veryStrong,
                            curX + 1,
                            curY + 1
                        );
                    } else if (guard == TREASURE_GUARD_SW) {
                        gEditManager->ClearArea(curX - 1, curY + 1, 1, 1, EDIT_CLEAR_ALL, false);
                        PlaceOverlay(
                            Random(0, 100) < 50 ? strongMonster : veryStrong,
                            curX - 1,
                            curY + 1
                        );
                    } else {
                        gEditManager->ClearArea(curX - 1, curY - 1, 1, 1, EDIT_CLEAR_ALL, false);
                        PlaceOverlay(
                            Random(0, 100) < 50 ? strongMonster : veryStrong,
                            curX - 1,
                            curY - 1
                        );
                    }
                } else if (Random(0, 100) < 90) {
                    treasureTries += RANDOM_MAP_TRIES_PER_OBJECT + 1;
                    pilesSoFar--;
                } else if (H1_ENUM_DECODE(
                               TerrainType,
                               m_map.cells[curX][curY].m_tileIndex / MAP_CELL_TILES_PER_TERRAIN
                           ) == TERRAIN_DESERT
                           && kindRoll < 20)
                    PlaceOverlay(genieLamp, curX, curY);
                else if (kindRoll < 2)
                    PlaceOverlay(genieLamp, curX, curY);
                else if (kindRoll < 25)
                    PlaceOverlay(chest, curX, curY);
                else if (kindRoll < 35)
                    PlaceOverlay(campfire, curX, curY);
                else
                    PlaceOverlay(randomResource, curX, curY);
            }
        }
    }
    monsterTries = gLandCellCount / 160;
    ScaleByDensity(&monsterTries, strength);
    monsterTries *= RANDOM_MAP_TRIES_PER_OBJECT;
    while (monsterTries > 0) {
        monsterTries--;
        curX = Random(0, MAP_CELL_GRID_SIZE - 1);
        curY = Random(0, MAP_CELL_GRID_SIZE - 1);
        if (m_map.cells[curX][curY].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN
            && !m_map.cells[curX][curY].m_objectTileset) {
            monsterTries -= RANDOM_MAP_TRIES_PER_OBJECT;
            gEditManager->ClearArea(curX, curY, 1, 1, EDIT_CLEAR_ALL, false);
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
#undef treasureTries
#undef southBlocked
#undef eastBlocked
#undef northBlocked
#undef westBlocked
#undef randomResource
#undef campfire
#undef monsterTries
#undef randomArtifact
#undef guard

VA(0x0041609d, 0x1ac)
void editManager::ScatterDecorations(void) {
    i32 terrainChance[EDITOR_TERRAIN_COUNT] = {15, 120, 120, 120, 120, 80, 120};
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
                && cell->m_tileIndex % MAP_CELL_TILES_PER_TERRAIN < TERRAIN_TILE_VARIANT_COUNT) {
                attempts = RANDOM_MAP_TRIES_PER_OBJECT;
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

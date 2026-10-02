#ifndef HOMM1_SOURCE_RESOURCETYPES_H
#define HOMM1_SOURCE_RESOURCETYPES_H

#include <Domains.h>

// clang-format off
H1_ENUM_BEGIN(ResourceType)
    // No resource (Buka RES_NONE): recruitUnit's creature without a
    // secondary cost.
    RESOURCE_NONE = -1,
    RESOURCE_WOOD = 0,
    RESOURCE_FIRST = RESOURCE_WOOD,
    RESOURCE_MERCURY = 1,
    RESOURCE_ORE = 2,
    RESOURCE_SULFUR = 3,
    RESOURCE_CRYSTAL = 4,
    RESOURCE_GEMS = 5,
    // The resources before gold (half-open): a creature's secondary cost is
    // the first of them it needs (recruitUnit, QuickViewRecruit); bankBox
    // lists them before the gold line.
    RESOURCE_NON_GOLD_END = 6,
    RESOURCE_GOLD = 6,
    RESOURCE_LAST = RESOURCE_GOLD,
    RESOURCE_COUNT = 7
H1_ENUM_END(ResourceType)
// clang-format on

#endif

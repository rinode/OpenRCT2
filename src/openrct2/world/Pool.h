/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../core/Money.hpp"
#include "../object/ObjectTypes.h"
#include "Location.hpp"

#include <cstdint>

namespace OpenRCT2
{
    struct PoolElement;
    class PoolObject;

    enum
    {
        PROVISIONAL_POOL_FLAG_SHOW_ARROW = (1 << 0),
        PROVISIONAL_POOL_FLAG_1 = (1 << 1),
        PROVISIONAL_POOL_FLAG_2 = (1 << 2),
    };

    enum class PoolEdgeStyle : uint8_t
    {
        square,
        angled,
        curved,
    };

    struct PoolSelection
    {
        ObjectEntryIndex Pool = kObjectEntryIndexNull;
    };

    struct ProvisionalPool
    {
        ObjectEntryIndex Type;
        CoordsXYZ Position;
        uint8_t Flags;
    };

    constexpr auto kPoolMaxHeight = 248 * kCoordsZStep;
    constexpr auto kPoolMinHeight = 2 * kCoordsZStep;
    constexpr auto kPoolHeightStep = 2 * kCoordsZStep;
    constexpr auto kPoolDepth = 2 * kCoordsZStep;
    constexpr auto kPoolClearance = 6 * kCoordsZStep;

    extern PoolSelection gPoolSelection;
    extern ProvisionalPool gProvisionalPool;

    const PoolObject* GetPoolEntry(ObjectEntryIndex entryIndex);

    money64 PoolRemove(const CoordsXYZ& poolLoc, uint32_t flags);
    void PoolProvisionalRemove();
    void PoolProvisionalUpdate();
    money64 PoolProvisionalSet(ObjectEntryIndex type, const CoordsXYZ& poolLoc, bool isWater, uint8_t edgeStyle);

    PoolElement* MapGetPoolElement(const CoordsXYZ& coords);

    void PoolConnectEdges(const CoordsXY& poolPos, PoolElement* poolElement);
    void PoolRemoveEdges(const CoordsXY& poolPos, PoolElement* poolElement);
} // namespace OpenRCT2

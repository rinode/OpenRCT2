/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../../object/ObjectTypes.h"
#include "TileElementBase.h"

#include <cstdint>

namespace OpenRCT2
{
    class PoolObject;

    enum
    {
        POOL_ELEMENT_FLAGS_WATER = 1 << 0,
        POOL_ELEMENT_FLAGS_IN_GROUND = 1 << 1,
    };

#pragma pack(push, 1)
    struct PoolElement : TileElementBase
    {
        static constexpr TileElementType kElementType = TileElementType::Pool;

    private:
        ObjectEntryIndex PoolIndex;     // 5-6
        uint8_t Flags;                  // 7
        uint8_t EdgeStyle;              // 8
        uint8_t EdgesAndCorners;        // 9
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
        uint8_t Pad0A[6];               // A-F
#pragma clang diagnostic pop

    public:
        ObjectEntryIndex GetPoolEntryIndex() const;
        void SetPoolEntryIndex(ObjectEntryIndex newIndex);
        const PoolObject* GetPoolEntry() const;

        bool IsInGround() const;
        void SetInGround(bool isInGround);

        bool IsWater() const;
        void SetIsWater(bool isWater);

        uint8_t GetEdgeStyle() const;
        void SetEdgeStyle(uint8_t newEdgeStyle);

        uint8_t GetEdges() const;
        void SetEdges(uint8_t newEdges);
        uint8_t GetCorners() const;
        void SetCorners(uint8_t newCorners);
        uint8_t GetEdgesAndCorners() const;
        void SetEdgesAndCorners(uint8_t newEdgesAndCorners);
    };
    static_assert(sizeof(PoolElement) == kTileElementSize);
#pragma pack(pop)
} // namespace OpenRCT2

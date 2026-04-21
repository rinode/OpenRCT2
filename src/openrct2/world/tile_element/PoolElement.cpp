/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "PoolElement.h"

#include "../../Context.h"
#include "../../object/ObjectManager.h"
#include "../../object/PoolObject.h"

namespace OpenRCT2
{
    ObjectEntryIndex PoolElement::GetPoolEntryIndex() const
    {
        return PoolIndex;
    }

    void PoolElement::SetPoolEntryIndex(ObjectEntryIndex newIndex)
    {
        PoolIndex = newIndex;
    }

    const PoolObject* PoolElement::GetPoolEntry() const
    {
        auto& objMgr = GetContext()->GetObjectManager();
        return static_cast<const PoolObject*>(objMgr.GetLoadedObject(ObjectType::pool, GetPoolEntryIndex()));
    }

    bool PoolElement::IsInGround() const
    {
        return (Flags & POOL_ELEMENT_FLAGS_IN_GROUND) != 0;
    }

    void PoolElement::SetInGround(bool isInGround)
    {
        Flags &= ~POOL_ELEMENT_FLAGS_IN_GROUND;
        if (isInGround)
            Flags |= POOL_ELEMENT_FLAGS_IN_GROUND;
    }

    bool PoolElement::IsWater() const
    {
        return (Flags & POOL_ELEMENT_FLAGS_WATER) != 0;
    }

    void PoolElement::SetIsWater(bool isWater)
    {
        Flags &= ~POOL_ELEMENT_FLAGS_WATER;
        if (isWater)
            Flags |= POOL_ELEMENT_FLAGS_WATER;
    }

    uint8_t PoolElement::GetEdgeStyle() const
    {
        return EdgeStyle;
    }

    void PoolElement::SetEdgeStyle(uint8_t newEdgeStyle)
    {
        EdgeStyle = newEdgeStyle;
    }

    uint8_t PoolElement::GetEdges() const
    {
        return EdgesAndCorners & 0b00001111;
    }

    void PoolElement::SetEdges(uint8_t newEdges)
    {
        EdgesAndCorners &= ~0b00001111;
        EdgesAndCorners |= (newEdges & 0b00001111);
    }

    uint8_t PoolElement::GetCorners() const
    {
        return EdgesAndCorners >> 4;
    }

    void PoolElement::SetCorners(uint8_t newCorners)
    {
        EdgesAndCorners &= ~0b11110000;
        EdgesAndCorners |= (newCorners << 4);
    }

    uint8_t PoolElement::GetEdgesAndCorners() const
    {
        return EdgesAndCorners;
    }

    void PoolElement::SetEdgesAndCorners(uint8_t newEdgesAndCorners)
    {
        EdgesAndCorners = newEdgesAndCorners;
    }
} // namespace OpenRCT2

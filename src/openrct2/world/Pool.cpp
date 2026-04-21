/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Pool.h"

#include "../Context.h"
#include "../GameState.h"
#include "../actions/GameActionRunner.h"
#include "../actions/PoolPlaceAction.h"
#include "../actions/PoolRemoveAction.h"
#include "../object/ObjectManager.h"
#include "../object/PoolObject.h"
#include "Map.h"
#include "tile_element/PoolElement.h"
#include "tile_element/TileElement.h"

namespace OpenRCT2
{
    using OpenRCT2::GameActions::CommandFlag;
    using OpenRCT2::GameActions::CommandFlags;

    PoolSelection gPoolSelection;
    ProvisionalPool gProvisionalPool;

    const PoolObject* GetPoolEntry(ObjectEntryIndex entryIndex)
    {
        auto& objMgr = GetContext()->GetObjectManager();
        auto obj = objMgr.GetLoadedObject(ObjectType::pool, entryIndex);
        if (obj == nullptr)
            return nullptr;

        return static_cast<const PoolObject*>(obj);
    }

    PoolElement* MapGetPoolElement(const CoordsXYZ& coords)
    {
        TileElement* tileElement = MapGetFirstElementAt(coords);
        if (tileElement == nullptr)
            return nullptr;
        do
        {
            if (tileElement->GetType() == TileElementType::Pool && tileElement->GetBaseZ() == coords.z)
                return tileElement->AsPool();
        } while (!(tileElement++)->IsLastForTile());

        return nullptr;
    }

    static bool PoolShouldConnectNeighbour(PoolElement* pool, PoolElement* neighbour)
    {
        return neighbour->IsWater() == pool->IsWater();
    }

    // Sprite lookup tables: index is an 8-bit bitmap of connected neighbours
    // (bits 0-3 orthogonal N/E/S/W, bits 4-7 diagonal).
    static constexpr const uint8_t kWaterSpriteMap[] = {
        0, 1, 2, 16, 4, 5, 17, 20, 8, 21, 10, 24, 25, 28, 31, 46, 159, 151, 156, 16, 60, 59, 95, 20, 81, 105, 80, 24, 117, 139,
        144, 46, 160, 63, 152, 96, 153, 62, 17, 20, 66, 132, 65, 142, 98, 147, 31, 46, 97, 96, 97, 96, 95, 20, 95, 20, 85, 142,
        85, 142, 144, 46, 144, 46, 157, 72, 69, 120, 149, 71, 99, 145, 154, 101, 68, 138, 25, 28, 31, 46, 111, 112, 119, 120,
        115, 116, 137, 145, 113, 143, 121, 138, 117, 139, 144, 46, 100, 88, 99, 145, 100, 88, 99, 145, 98, 147, 31, 46, 98, 147,
        31, 46, 137, 145, 137, 145, 137, 145, 137, 145, 144, 46, 144, 46, 144, 46, 144, 46, 158, 155, 78, 104, 75, 74, 135, 141,
        150, 21, 77, 24, 102, 28, 148, 46, 106, 106, 104, 104, 94, 94, 141, 141, 105, 105, 24, 24, 139, 139, 46, 46, 126, 128,
        127, 146, 134, 136, 135, 141, 130, 132, 131, 142, 140, 147, 148, 46, 146, 146, 146, 146, 141, 141, 141, 141, 142, 142,
        142, 142, 46, 46, 46, 46, 103, 101, 91, 138, 102, 28, 148, 46, 103, 101, 91, 138, 102, 28, 148, 46, 143, 143, 138, 138,
        139, 139, 46, 46, 143, 143, 138, 138, 139, 139, 46, 46, 140, 147, 148, 46, 140, 147, 148, 46, 140, 147, 148, 46, 140,
        147, 148, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46, 46,
    };

    static constexpr const uint8_t kSolidSpriteMap[] = {
        162, 193, 194, 176, 191, 161, 177, 161, 192, 175, 161, 161, 178, 161, 161, 161, 162, 189, 186, 176, 191, 161, 169, 161,
        192, 174, 161, 161, 178, 161, 161, 161, 162, 193, 190, 171, 183, 161, 177, 161, 192, 175, 161, 161, 170, 161, 161, 161,
        162, 189, 182, 171, 183, 161, 169, 161, 192, 174, 161, 161, 170, 161, 161, 161, 162, 193, 194, 176, 187, 161, 172, 161,
        184, 167, 161, 161, 178, 161, 161, 161, 162, 189, 186, 176, 187, 161, 165, 161, 184, 163, 161, 161, 178, 161, 161, 161,
        162, 193, 190, 171, 179, 161, 172, 161, 184, 167, 161, 161, 170, 161, 161, 161, 162, 189, 182, 171, 179, 161, 165, 161,
        184, 163, 161, 161, 170, 161, 161, 161, 162, 185, 194, 168, 191, 161, 177, 161, 188, 175, 161, 161, 173, 161, 161, 161,
        162, 181, 186, 168, 191, 161, 169, 161, 188, 174, 161, 161, 173, 161, 161, 161, 162, 185, 190, 164, 183, 161, 177, 161,
        188, 175, 161, 161, 166, 161, 161, 161, 162, 181, 182, 164, 183, 161, 169, 161, 188, 174, 161, 161, 166, 161, 161, 161,
        162, 185, 194, 168, 187, 161, 172, 161, 180, 167, 161, 161, 173, 161, 161, 161, 162, 181, 186, 168, 187, 161, 165, 161,
        180, 163, 161, 161, 173, 161, 161, 161, 162, 185, 190, 164, 179, 161, 172, 161, 180, 167, 161, 161, 166, 161, 161, 161,
        162, 181, 182, 164, 179, 161, 165, 161, 180, 163, 161, 161, 166, 161, 161, 161,
    };

    static void PoolUpdateSprite(const CoordsXY& poolPos, PoolElement* poolElement)
    {
        auto z = poolElement->GetBaseZ();
        uint8_t edges = 0;
        for (int direction = 0; direction < 8; direction++)
        {
            PoolElement* neighbour = MapGetPoolElement({ poolPos + CoordsDirectionDelta[direction], z });
            if ((neighbour != nullptr && PoolShouldConnectNeighbour(poolElement, neighbour))
                || (neighbour == nullptr && !poolElement->IsWater()))
            {
                edges |= 1 << direction;
            }
        }

        if (poolElement->IsWater())
            poolElement->SetEdgesAndCorners(kWaterSpriteMap[edges]);
        else
            poolElement->SetEdgesAndCorners(kSolidSpriteMap[edges]);
    }

    static void PoolUpdateNeighbours(const CoordsXY& poolPos, PoolElement* poolElement)
    {
        auto z = poolElement->GetBaseZ();
        for (int direction = 0; direction < 8; direction++)
        {
            PoolElement* neighbour = MapGetPoolElement({ poolPos + CoordsDirectionDelta[direction], z });
            if (neighbour != nullptr)
            {
                PoolUpdateSprite(poolPos + CoordsDirectionDelta[direction], neighbour);
            }
        }
    }

    void PoolConnectEdges(const CoordsXY& poolPos, PoolElement* poolElement)
    {
        PoolUpdateSprite(poolPos, poolElement);
        PoolUpdateNeighbours(poolPos, poolElement);
    }

    void PoolRemoveEdges(const CoordsXY& poolPos, PoolElement* poolElement)
    {
        PoolUpdateNeighbours(poolPos, poolElement);
    }

    money64 PoolRemove(const CoordsXYZ& poolLoc, uint32_t flags)
    {
        auto action = GameActions::PoolRemoveAction(poolLoc);
        action.SetFlags(GameActions::CommandFlags(flags));

        if (GameActions::CommandFlags(flags).has(CommandFlag::apply))
        {
            auto res = GameActions::Execute(&action, getGameState());
            return res.cost;
        }
        auto res = GameActions::Query(&action, getGameState());
        return res.cost;
    }

    void PoolProvisionalRemove()
    {
        if (gProvisionalPool.Flags & PROVISIONAL_POOL_FLAG_1)
        {
            gProvisionalPool.Flags &= ~PROVISIONAL_POOL_FLAG_1;

            auto action = GameActions::PoolRemoveAction(gProvisionalPool.Position);
            action.SetFlags({ CommandFlag::apply, CommandFlag::allowDuringPaused, CommandFlag::noSpend, CommandFlag::ghost });
            GameActions::Execute(&action, getGameState());
        }
    }

    void PoolProvisionalUpdate()
    {
        PoolProvisionalRemove();
    }

    money64 PoolProvisionalSet(ObjectEntryIndex type, const CoordsXYZ& poolLoc, bool isWater, uint8_t edgeStyle)
    {
        PoolProvisionalRemove();

        auto action = GameActions::PoolPlaceAction(poolLoc, type, isWater, edgeStyle);
        action.SetFlags({ CommandFlag::ghost, CommandFlag::allowDuringPaused });
        auto res = GameActions::Execute(&action, getGameState());
        money64 cost = res.error == GameActions::Status::ok ? res.cost : kMoney64Undefined;
        if (res.error == GameActions::Status::ok)
        {
            gProvisionalPool.Type = type;
            gProvisionalPool.Position = poolLoc;
            gProvisionalPool.Flags |= PROVISIONAL_POOL_FLAG_1;
        }
        return cost;
    }
} // namespace OpenRCT2

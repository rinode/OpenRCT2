/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "PoolRemoveAction.h"

#include "../Cheats.h"
#include "../GameState.h"
#include "../OpenRCT2.h"
#include "../core/MemoryStream.h"
#include "../localisation/StringIds.h"
#include "../management/Finance.h"
#include "../world/Location.hpp"
#include "../world/Map.h"
#include "../world/Park.h"
#include "../world/Pool.h"
#include "../world/TileElementsView.h"
#include "../world/tile_element/PoolElement.h"

namespace OpenRCT2::GameActions
{
    PoolRemoveAction::PoolRemoveAction(const CoordsXYZ& location)
        : _loc(location)
    {
    }

    void PoolRemoveAction::AcceptParameters(GameActionParameterVisitor& visitor)
    {
        visitor.Visit(_loc);
    }

    uint16_t PoolRemoveAction::GetActionFlags() const
    {
        return GameAction::GetActionFlags();
    }

    void PoolRemoveAction::Serialise(DataSerialiser& stream)
    {
        GameAction::Serialise(stream);

        stream << DS_TAG(_loc);
    }

    Result PoolRemoveAction::Query(GameState_t& gameState, Park::ParkData& park) const
    {
        auto res = Result();
        res.cost = -6.00_GBP;
        res.expenditure = ExpenditureType::landscaping;
        res.position = _loc;

        if (!LocationValid(_loc))
        {
            return Result(Status::invalidParameters, STR_CANT_REMOVE_THIS, STR_OFF_EDGE_OF_MAP);
        }

        if (!(gLegacyScene == LegacyScene::scenarioEditor) && !GetFlags().has(CommandFlag::ghost) && !gameState.cheats.sandboxMode)
        {
            if (!MapIsLocationOwned(_loc))
            {
                return Result(Status::notOwned, STR_CANT_REMOVE_THIS, STR_LAND_NOT_OWNED_BY_PARK);
            }
        }

        auto* poolElement = FindPoolElement();
        if (poolElement == nullptr)
        {
            return Result(Status::invalidParameters, STR_CANT_REMOVE_THIS, STR_INVALID_SELECTION_OF_OBJECTS);
        }

        return res;
    }

    Result PoolRemoveAction::Execute(GameState_t& gameState, Park::ParkData& park) const
    {
        auto res = Result();
        res.cost = -6.00_GBP;
        res.expenditure = ExpenditureType::landscaping;
        res.position = _loc;

        auto* poolElement = FindPoolElement();
        if (poolElement == nullptr)
        {
            return Result(Status::invalidParameters, STR_CANT_REMOVE_THIS, STR_INVALID_SELECTION_OF_OBJECTS);
        }

        res.position.z = TileElementHeight(res.position);

        MapInvalidateTileFull(_loc);
        PoolRemoveEdges(_loc, poolElement);
        TileElementRemove(poolElement->as<TileElement>());

        return res;
    }

    PoolElement* PoolRemoveAction::FindPoolElement() const
    {
        const bool isGhost = GetFlags().has(CommandFlag::ghost);
        for (auto* poolElement : TileElementsView<PoolElement>(_loc))
        {
            if (isGhost != poolElement->IsGhost())
                continue;

            if (poolElement->GetBaseZ() != _loc.z)
                continue;

            return poolElement;
        }
        return nullptr;
    }
} // namespace OpenRCT2::GameActions

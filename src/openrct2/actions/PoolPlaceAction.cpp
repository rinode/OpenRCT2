/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "PoolPlaceAction.h"

#include "../Cheats.h"
#include "../GameState.h"
#include "../OpenRCT2.h"
#include "../core/Guard.hpp"
#include "../core/MemoryStream.h"
#include "../localisation/StringIds.h"
#include "../management/Finance.h"
#include "../world/ConstructionClearance.h"
#include "../world/Location.hpp"
#include "../world/Map.h"
#include "../world/Park.h"
#include "../world/Pool.h"
#include "../world/QuarterTile.h"
#include "../world/TileElementsView.h"
#include "../world/tile_element/PoolElement.h"
#include "../world/tile_element/SurfaceElement.h"

namespace OpenRCT2::GameActions
{
    PoolPlaceAction::PoolPlaceAction(const CoordsXYZ& loc, ObjectEntryIndex type, bool isWater, uint8_t edgeStyle)
        : _loc(loc)
        , _type(type)
        , _isWater(isWater)
        , _edgeStyle(edgeStyle)
    {
    }

    void PoolPlaceAction::AcceptParameters(GameActionParameterVisitor& visitor)
    {
        visitor.Visit(_loc);
        visitor.Visit("object", _type);
        visitor.Visit("isWater", _isWater);
        visitor.Visit("edgeStyle", _edgeStyle);
    }

    uint16_t PoolPlaceAction::GetActionFlags() const
    {
        return GameAction::GetActionFlags();
    }

    void PoolPlaceAction::Serialise(DataSerialiser& stream)
    {
        GameAction::Serialise(stream);

        stream << DS_TAG(_loc) << DS_TAG(_type) << DS_TAG(_isWater) << DS_TAG(_edgeStyle);
    }

    Result PoolPlaceAction::Query(GameState_t& gameState, Park::ParkData& park) const
    {
        auto res = Result();
        res.cost = 0;
        res.expenditure = ExpenditureType::landscaping;
        res.position = _loc.ToTileCentre();

        if (!LocationValid(_loc) || MapIsEdge(_loc))
        {
            return Result(Status::invalidParameters, STR_CANT_BUILD_POOL_HERE, STR_OFF_EDGE_OF_MAP);
        }

        if (!(gLegacyScene == LegacyScene::scenarioEditor || gameState.cheats.sandboxMode) && !MapIsLocationOwned(_loc))
        {
            return Result(Status::disallowed, STR_CANT_BUILD_POOL_HERE, STR_LAND_NOT_OWNED_BY_PARK);
        }

        if (_loc.z < kPoolMinHeight)
        {
            return Result(Status::disallowed, STR_CANT_BUILD_POOL_HERE, STR_TOO_LOW);
        }

        if (_loc.z > kPoolMaxHeight)
        {
            return Result(Status::disallowed, STR_CANT_BUILD_POOL_HERE, STR_TOO_HIGH);
        }

        PoolProvisionalRemove();

        auto* poolElement = MapGetPoolElement(_loc);
        if (poolElement == nullptr)
        {
            return ElementInsertQueryExecute(gameState, std::move(res), false);
        }
        return ElementUpdateQuery(poolElement, std::move(res));
    }

    Result PoolPlaceAction::Execute(GameState_t& gameState, Park::ParkData& park) const
    {
        auto res = Result();
        res.cost = 0;
        res.expenditure = ExpenditureType::landscaping;
        res.position = _loc.ToTileCentre();

        auto* poolElement = MapGetPoolElement(_loc);
        if (poolElement == nullptr)
        {
            return ElementInsertQueryExecute(gameState, std::move(res), true);
        }
        return ElementUpdateExecute(poolElement, std::move(res));
    }

    Result PoolPlaceAction::ElementUpdateQuery(PoolElement* poolElement, Result res) const
    {
        if (GetFlags().has(CommandFlag::ghost) && !poolElement->IsGhost())
        {
            return Result(Status::itemAlreadyPlaced, STR_CANT_BUILD_POOL_HERE, kStringIdNone);
        }
        return res;
    }

    Result PoolPlaceAction::ElementUpdateExecute(PoolElement* poolElement, Result res) const
    {
        poolElement->SetPoolEntryIndex(_type);

        if (poolElement->IsWater() != _isWater || poolElement->GetEdgeStyle() != _edgeStyle)
        {
            poolElement->SetIsWater(_isWater);
            poolElement->SetEdgeStyle(_edgeStyle);
            PoolConnectEdges(_loc, poolElement);
        }
        return res;
    }

    Result PoolPlaceAction::ElementInsertQueryExecute(GameState_t& gameState, Result res, bool isExecuting) const
    {
        if (!isExecuting && !MapCheckCapacityAndReorganise(_loc))
        {
            return Result(Status::noFreeElements, STR_CANT_BUILD_POOL_HERE, kStringIdNone);
        }

        res.cost = 12.00_GBP;

        QuarterTile quarterTile{ 0b1111, 0 };
        auto zLow = _loc.z;
        auto zHigh = zLow + kPoolClearance;

        auto* surfaceElement = MapGetSurfaceElementAt(_loc);
        if (surfaceElement == nullptr)
        {
            return Result(Status::invalidParameters, STR_CANT_BUILD_POOL_HERE, kStringIdNone);
        }

        bool inGround = false;
        if (surfaceElement->GetSlope() == 0 && surfaceElement->GetBaseZ() == zLow + kPoolDepth)
        {
            inGround = true;
        }

        auto canBuild = MapCanConstructWithClearAt(
            { _loc, inGround ? zLow + kPoolDepth : zLow, zHigh }, MapPlaceNonSceneryClearFunc, quarterTile,
            GetFlags().with(CommandFlag::apply), 0);
        if (canBuild.error != Status::ok)
        {
            canBuild.errorTitle = STR_CANT_BUILD_POOL_HERE;
            return canBuild;
        }
        res.cost += canBuild.cost;

        const auto clearanceData = canBuild.getData<ConstructClearResult>();
        if (!isExecuting && !gameState.cheats.disableClearanceChecks && (clearanceData.GroundFlags & ELEMENT_IS_UNDERWATER))
        {
            return Result(Status::disallowed, STR_CANT_BUILD_POOL_HERE, STR_CANT_BUILD_THIS_UNDERWATER);
        }

        if (isExecuting)
        {
            auto* poolElement = TileElementInsert<PoolElement>(_loc, 0b1111);
            Guard::Assert(poolElement != nullptr);

            poolElement->SetClearanceZ(zHigh);
            poolElement->SetPoolEntryIndex(_type);
            poolElement->SetGhost(GetFlags().has(CommandFlag::ghost));
            poolElement->SetInGround(inGround);
            poolElement->SetIsWater(_isWater);
            poolElement->SetEdgeStyle(_edgeStyle);

            PoolConnectEdges(_loc, poolElement);
            MapInvalidateTileFull(_loc);
        }
        return res;
    }
} // namespace OpenRCT2::GameActions

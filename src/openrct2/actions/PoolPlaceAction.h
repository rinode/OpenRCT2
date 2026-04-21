/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "GameAction.hpp"

namespace OpenRCT2
{
    struct PoolElement;
}

namespace OpenRCT2::GameActions
{
    class PoolPlaceAction final : public GameActionBase<GameCommand::PlacePool>
    {
    private:
        CoordsXYZ _loc;
        ObjectEntryIndex _type{};
        bool _isWater{};
        uint8_t _edgeStyle{};

    public:
        PoolPlaceAction() = default;
        PoolPlaceAction(const CoordsXYZ& loc, ObjectEntryIndex type, bool isWater, uint8_t edgeStyle);

        void AcceptParameters(GameActionParameterVisitor& visitor) override;
        uint16_t GetActionFlags() const override;
        void Serialise(DataSerialiser& stream) override;
        Result Query(GameState_t& gameState, Park::ParkData& park) const override;
        Result Execute(GameState_t& gameState, Park::ParkData& park) const override;

    private:
        Result ElementUpdateQuery(PoolElement* poolElement, Result res) const;
        Result ElementUpdateExecute(PoolElement* poolElement, Result res) const;
        Result ElementInsertQueryExecute(GameState_t& gameState, Result res, bool isExecuting) const;
    };
} // namespace OpenRCT2::GameActions

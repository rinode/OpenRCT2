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
    class PoolRemoveAction final : public GameActionBase<GameCommand::RemovePool>
    {
    private:
        CoordsXYZ _loc;

    public:
        PoolRemoveAction() = default;
        PoolRemoveAction(const CoordsXYZ& location);

        void AcceptParameters(GameActionParameterVisitor& visitor) override;
        uint16_t GetActionFlags() const override;
        void Serialise(DataSerialiser& stream) override;
        Result Query(GameState_t& gameState, Park::ParkData& park) const override;
        Result Execute(GameState_t& gameState, Park::ParkData& park) const override;

    private:
        PoolElement* FindPoolElement() const;
    };
} // namespace OpenRCT2::GameActions

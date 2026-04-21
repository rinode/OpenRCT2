/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "../core/StringTypes.h"
#include "../drawing/ImageIndexType.h"
#include "Object.h"

namespace OpenRCT2
{
    class PoolObject final : public Object
    {
    public:
        static constexpr ObjectType kObjectType = ObjectType::pool;

        StringId NameStringId{};
        ImageIndex PreviewImageId{};
        ImageIndex BaseImageId{};
        uint8_t Flags{};

    public:
        void ReadJson(IReadObjectContext* context, json_t& root) override;
        void Load() override;
        void Unload() override;

        void DrawPreview(Drawing::RenderTarget& rt, int32_t width, int32_t height) const override;
    };
} // namespace OpenRCT2

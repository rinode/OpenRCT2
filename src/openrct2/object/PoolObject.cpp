/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma warning(disable : 4706) // assignment within conditional expression

#include "PoolObject.h"

#include "../core/Guard.hpp"
#include "../core/Json.hpp"
#include "../drawing/Drawing.h"
#include "../drawing/Image.h"
#include "../localisation/Language.h"

namespace OpenRCT2
{
    void PoolObject::ReadJson(IReadObjectContext* context, json_t& root)
    {
        Guard::Assert(root.is_object(), "PoolObject::ReadJson expects parameter root to be object");

        PopulateTablesFromJson(context, root);
    }

    void PoolObject::Load()
    {
        GetStringTable().Sort();
        NameStringId = LanguageAllocateObjectString(GetName());

        PreviewImageId = LoadImages();
        BaseImageId = PreviewImageId + 1;
    }

    void PoolObject::Unload()
    {
        UnloadImages();
        LanguageFreeObjectString(NameStringId);

        NameStringId = 0;
        PreviewImageId = 0;
        BaseImageId = 0;
    }

    void PoolObject::DrawPreview(Drawing::RenderTarget& rt, int32_t width, int32_t height) const
    {
        auto screenCoords = ScreenCoordsXY{ width / 2, height / 2 };
        GfxDrawSprite(rt, ImageId(PreviewImageId), screenCoords);
    }
} // namespace OpenRCT2

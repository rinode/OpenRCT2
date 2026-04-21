/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Paint.Pool.h"

#include "../../Context.h"
#include "../../drawing/Drawing.h"
#include "../../interface/Viewport.h"
#include "../../object/ObjectManager.h"
#include "../../object/PoolObject.h"
#include "../../profiling/Profiling.h"
#include "../../world/tile_element/PoolElement.h"
#include "../Paint.h"
#include "../Paint.SessionFlags.h"
#include "Paint.TileElement.h"

using namespace OpenRCT2;
using namespace OpenRCT2::Drawing;

static const PoolObject* GetPoolObject(ObjectEntryIndex index)
{
    PROFILED_FUNCTION();

    auto& objMgr = GetContext()->GetObjectManager();
    auto* obj = objMgr.GetLoadedObject(ObjectType::pool, index);
    if (obj == nullptr)
        return nullptr;

    return static_cast<const PoolObject*>(obj);
}

void PaintPool(PaintSession& session, uint8_t direction, int32_t height, const PoolElement& poolElement)
{
    PROFILED_FUNCTION();

    session.InteractionType = ViewportInteractionItem::pool;
    session.Flags |= PaintSessionFlags::PassedSurface;

    const auto poolType = poolElement.GetPoolEntryIndex();

    ImageId imageId;
    if (poolElement.IsGhost())
    {
        session.InteractionType = poolElement.IsInGround() ? ViewportInteractionItem::terrain
                                                           : ViewportInteractionItem::none;
        imageId = imageId.WithRemap(FilterPaletteID::paletteGhost);
    }

    const auto* obj = GetPoolObject(poolType);
    if (obj != nullptr)
    {
        imageId = imageId.WithIndex(obj->BaseImageId + poolElement.GetEdgesAndCorners());
    }

    PaintAddImageAsParent(session, imageId, { 0, 0, height }, { 32, 32, 16 });

    if (poolElement.IsInGround())
        session.Flags |= PaintSessionFlags::HideSurface;
}

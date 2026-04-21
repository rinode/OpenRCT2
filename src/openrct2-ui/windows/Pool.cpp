/*****************************************************************************
 * Copyright (c) 2014-2026 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include <openrct2-ui/interface/Dropdown.h>
#include <openrct2-ui/interface/Viewport.h>
#include <openrct2-ui/interface/ViewportInteraction.h>
#include <openrct2-ui/interface/Widget.h>
#include <openrct2-ui/windows/Windows.h>
#include <openrct2/Context.h>
#include <openrct2/Diagnostic.h>
#include <openrct2/GameState.h>
#include <openrct2/Input.h>
#include <openrct2/SpriteIds.h>
#include <openrct2/actions/GameActionRunner.h>
#include <openrct2/actions/PoolPlaceAction.h>
#include <openrct2/audio/Audio.h>
#include <openrct2/drawing/Drawing.h>
#include <openrct2/drawing/Text.h>
#include <openrct2/localisation/Formatter.h>
#include <openrct2/object/ObjectLimits.h>
#include <openrct2/object/ObjectManager.h>
#include <openrct2/object/PoolObject.h>
#include <openrct2/ui/WindowManager.h>
#include <openrct2/world/Map.h>
#include <openrct2/world/MapSelection.h>
#include <openrct2/world/Park.h>
#include <openrct2/world/Pool.h>
#include <openrct2/world/tile_element/PoolElement.h>
#include <openrct2/world/tile_element/SurfaceElement.h>

namespace OpenRCT2::Ui::Windows
{
    static constexpr StringId kWindowTitle = STR_POOLS;
    static constexpr ScreenSize kWindowSize = { 150, 180 };

    enum WindowPoolWidgetIdx : WidgetIndex
    {
        WIDX_BACKGROUND,
        WIDX_TITLE,
        WIDX_CLOSE,

        WIDX_TYPE_GROUP,
        WIDX_POOL_TYPE,

        WIDX_CORNER_GROUP,
        WIDX_CORNER_SQUARE,
        WIDX_CORNER_ANGLED,
        WIDX_CORNER_ROUNDED,

        WIDX_MODE_GROUP,
        WIDX_CONSTRUCT_PATH_TILE,
        WIDX_CONSTRUCT_WATER_TILE,
    };

    // clang-format off
    static constexpr auto kPoolWidgets = makeWidgets(
        makeWindowShim(kWindowTitle, kWindowSize),

        makeWidget({ 3,  17}, {100, 55}, WidgetType::groupbox, WindowColour::primary  , STR_TYPE                                                     ),
        makeWidget({ 6,  30}, { 47, 36}, WidgetType::flatBtn,  WindowColour::secondary, 0xFFFFFFFF,                            STR_POOL_TIP          ),

        makeWidget({ 3,  75}, { 80, 41}, WidgetType::groupbox, WindowColour::primary  , STR_CORNER_STYLE                                             ),
        makeWidget({ 7,  87}, { 24, 24}, WidgetType::flatBtn,  WindowColour::secondary, ImageId(SPR_G2_BUTTON_SQUARE_CORNERS),  STR_POOL_SQUARE_TIP  ),
        makeWidget({31,  87}, { 24, 24}, WidgetType::flatBtn,  WindowColour::secondary, ImageId(SPR_G2_BUTTON_ANGLED_CORNERS),  STR_POOL_ANGLED_TIP  ),
        makeWidget({55,  87}, { 24, 24}, WidgetType::flatBtn,  WindowColour::secondary, ImageId(SPR_G2_BUTTON_ROUNDED_CORNERS), STR_POOL_ROUNDED_TIP ),

        makeWidget({ 3, 119}, {100, 54}, WidgetType::groupbox, WindowColour::primary                                                                 ),
        makeWidget({ 5, 130}, { 36, 36}, WidgetType::flatBtn,  WindowColour::secondary, ImageId(SPR_G2_BUTTON_POOL_PATH),       STR_POOL_PATH_TIP    ),
        makeWidget({43, 130}, { 36, 36}, WidgetType::flatBtn,  WindowColour::secondary, ImageId(SPR_G2_BUTTON_POOL_WATER),      STR_POOL_WATER_TIP   )
    );
    // clang-format on

    class PoolWindow final : public Window
    {
    private:
        std::vector<ObjectEntryIndex> _dropdownEntries;
        PoolEdgeStyle _edgeStyle = PoolEdgeStyle::square;
        bool _isWater = true;
        money64 _windowPoolCost = kMoney64Undefined;
        bool _poolErrorOccured = false;

    public:
        void onOpen() override
        {
            setWidgets(kPoolWidgets);

            WindowInitScrollWidgets(*this);
            WindowPushOthersRight(*this);
            ShowGridlines();

            ToolCancel();
            ToolSet(*this, WIDX_CONSTRUCT_WATER_TILE, Tool::pathDown);
            gInputFlags.set(InputFlag::allowRightMouseRemoval);
            _isWater = true;
            _poolErrorOccured = false;
            setPressedWidgets();
        }

        void onClose() override
        {
            PoolProvisionalUpdate();
            ViewportSetVisibility(ViewportVisibility::standard);
            gMapSelectFlags.unset(MapSelectFlag::enableConstruct);

            auto* windowMgr = GetWindowManager();
            windowMgr->InvalidateByClass(WindowClass::topToolbar);
            HideGridlines();
        }

        void onUpdate() override
        {
            if (!isToolActive(WindowClass::pool, WIDX_CONSTRUCT_PATH_TILE)
                && !isToolActive(WindowClass::pool, WIDX_CONSTRUCT_WATER_TILE))
            {
                close();
            }
        }

        void onMouseDown(WidgetIndex widgetIndex) override
        {
            switch (widgetIndex)
            {
                case WIDX_POOL_TYPE:
                    showPoolTypesDialog(&widgets[widgetIndex]);
                    break;
                case WIDX_CORNER_SQUARE:
                    _edgeStyle = PoolEdgeStyle::square;
                    setPressedWidgets();
                    invalidate();
                    break;
                case WIDX_CORNER_ANGLED:
                    _edgeStyle = PoolEdgeStyle::angled;
                    setPressedWidgets();
                    invalidate();
                    break;
                case WIDX_CORNER_ROUNDED:
                    _edgeStyle = PoolEdgeStyle::curved;
                    setPressedWidgets();
                    invalidate();
                    break;
            }
        }

        void onMouseUp(WidgetIndex widgetIndex) override
        {
            switch (widgetIndex)
            {
                case WIDX_CLOSE:
                    close();
                    break;
                case WIDX_CONSTRUCT_PATH_TILE:
                    if (_isWater)
                    {
                        PoolProvisionalUpdate();
                        _windowPoolCost = kMoney64Undefined;
                        ToolCancel();
                        ToolSet(*this, WIDX_CONSTRUCT_PATH_TILE, Tool::pathDown);
                        gInputFlags.set(InputFlag::allowRightMouseRemoval);
                        _isWater = false;
                        _poolErrorOccured = false;
                        setPressedWidgets();
                    }
                    break;
                case WIDX_CONSTRUCT_WATER_TILE:
                    if (!_isWater)
                    {
                        PoolProvisionalUpdate();
                        _windowPoolCost = kMoney64Undefined;
                        ToolCancel();
                        ToolSet(*this, WIDX_CONSTRUCT_WATER_TILE, Tool::pathDown);
                        gInputFlags.set(InputFlag::allowRightMouseRemoval);
                        _isWater = true;
                        _poolErrorOccured = false;
                        setPressedWidgets();
                    }
                    break;
            }
        }

        void onDropdown(WidgetIndex widgetIndex, int32_t selectedIndex) override
        {
            if (widgetIndex != WIDX_POOL_TYPE)
                return;
            if (selectedIndex < 0 || static_cast<size_t>(selectedIndex) >= _dropdownEntries.size())
                return;

            gPoolSelection.Pool = _dropdownEntries[selectedIndex];
            PoolProvisionalUpdate();
            _windowPoolCost = kMoney64Undefined;
            invalidate();
        }

        void onToolUpdate(WidgetIndex widgetIndex, const ScreenCoordsXY& screenCoords) override
        {
            auto coords = getPlacePositionFromScreen(screenCoords);
            if (!coords)
                return;

            if (gProvisionalPool.Flags & PROVISIONAL_POOL_FLAG_1 && gProvisionalPool.Position == *coords)
                return;

            gMapSelectFlags.set(MapSelectFlag::enable);
            gMapSelectType = MapSelectType::full;
            gMapSelectPositionA = *coords;
            gMapSelectPositionB = *coords;

            _windowPoolCost = PoolProvisionalSet(gPoolSelection.Pool, *coords, _isWater, EnumValue(_edgeStyle));

            auto* windowMgr = GetWindowManager();
            windowMgr->InvalidateByClass(WindowClass::pool);
        }

        void onToolUp(WidgetIndex, const ScreenCoordsXY&) override
        {
            _poolErrorOccured = false;
        }

        void onToolDown(WidgetIndex, const ScreenCoordsXY& screenCoords) override
        {
            placePoolAt(screenCoords);
        }

        void onToolDrag(WidgetIndex, const ScreenCoordsXY& screenCoords) override
        {
            placePoolAt(screenCoords);
        }

        void onPrepareDraw() override
        {
            setPressedWidgets();
        }

        void onDraw(Drawing::RenderTarget& rt) override
        {
            drawWidgets(rt);
            drawDropdownButtons(rt);

            if (_windowPoolCost != kMoney64Undefined && !(getGameState().park.flags & PARK_FLAGS_NO_MONEY))
            {
                auto screenCoords = windowPos
                    + ScreenCoordsXY{ widgets[WIDX_MODE_GROUP].midX(), widgets[WIDX_MODE_GROUP].bottom - 12 };
                auto ft = Formatter();
                ft.Add<money64>(_windowPoolCost);
                drawText(rt, screenCoords, STR_COST_LABEL, ft, { TextAlignment::centre });
            }
        }

    private:
        void setPressedWidgets()
        {
            uint64_t pressed = pressedWidgets;
            pressed &= ~(1uLL << WIDX_CORNER_SQUARE);
            pressed &= ~(1uLL << WIDX_CORNER_ANGLED);
            pressed &= ~(1uLL << WIDX_CORNER_ROUNDED);
            pressed &= ~(1uLL << WIDX_CONSTRUCT_PATH_TILE);
            pressed &= ~(1uLL << WIDX_CONSTRUCT_WATER_TILE);

            switch (_edgeStyle)
            {
                case PoolEdgeStyle::square:
                    pressed |= (1uLL << WIDX_CORNER_SQUARE);
                    break;
                case PoolEdgeStyle::angled:
                    pressed |= (1uLL << WIDX_CORNER_ANGLED);
                    break;
                case PoolEdgeStyle::curved:
                    pressed |= (1uLL << WIDX_CORNER_ROUNDED);
                    break;
            }
            pressed |= _isWater ? (1uLL << WIDX_CONSTRUCT_WATER_TILE) : (1uLL << WIDX_CONSTRUCT_PATH_TILE);
            pressedWidgets = pressed;
        }

        std::optional<CoordsXYZ> getPlacePositionFromScreen(const ScreenCoordsXY& screenCoords)
        {
            auto mapCoords = ViewportInteractionGetTileStartAtCursor(screenCoords);
            if (mapCoords.IsNull())
                return std::nullopt;

            mapCoords = mapCoords.ToTileStart();
            auto surfaceElement = MapGetSurfaceElementAt(mapCoords);
            if (surfaceElement == nullptr)
                return std::nullopt;

            auto z = MapGetHighestZ(mapCoords);
            if (surfaceElement->GetSlope() == 0 && surfaceElement->GetBaseZ() == z)
                z -= kPoolDepth;

            return CoordsXYZ(mapCoords, z);
        }

        void placePoolAt(const ScreenCoordsXY& screenCoords)
        {
            if (_poolErrorOccured)
                return;

            PoolProvisionalUpdate();

            auto coords = getPlacePositionFromScreen(screenCoords);
            if (!coords)
                return;

            auto poolPlaceAction = GameActions::PoolPlaceAction(*coords, gPoolSelection.Pool, _isWater, EnumValue(_edgeStyle));
            poolPlaceAction.SetCallback([this](const GameActions::GameAction* ga, const GameActions::Result* result) {
                if (result->error == GameActions::Status::ok)
                {
                    if (result->cost != 0)
                    {
                        Audio::Play3D(Audio::SoundId::placeItem, result->position);
                    }
                }
                else
                {
                    _poolErrorOccured = true;
                }
            });
            GameActions::Execute(&poolPlaceAction, getGameState());
        }

        void drawDropdownButton(Drawing::RenderTarget& rt, WidgetIndex widgetIndex, ImageIndex image)
        {
            const auto& widget = widgets[widgetIndex];
            GfxDrawSprite(rt, ImageId(image), { windowPos.x + widget.left, windowPos.y + widget.top });
        }

        void drawDropdownButtons(Drawing::RenderTarget& rt)
        {
            auto poolImage = kImageIndexUndefined;
            auto poolEntry = GetPoolEntry(gPoolSelection.Pool);
            if (poolEntry != nullptr)
            {
                poolImage = poolEntry->PreviewImageId;
            }
            drawDropdownButton(rt, WIDX_POOL_TYPE, poolImage);
        }

        void showPoolTypesDialog(Widget* widget)
        {
            auto& objManager = GetContext()->GetObjectManager();
            uint32_t numTypes = 0;

            _dropdownEntries.clear();
            std::optional<size_t> defaultIndex;
            for (ObjectEntryIndex i = 0; i < kMaxPoolObjects; i++)
            {
                const auto* poolType = static_cast<PoolObject*>(objManager.GetLoadedObject(ObjectType::pool, i));
                if (poolType == nullptr)
                    continue;

                if (gPoolSelection.Pool == i)
                    defaultIndex = numTypes;

                gDropdown.items[numTypes] = Dropdown::ImageItem(ImageId(poolType->PreviewImageId), poolType->NameStringId);
                _dropdownEntries.push_back(i);
                numTypes++;
            }

            auto itemsPerRow = DropdownGetAppropriateImageDropdownItemsPerRow(numTypes);
            WindowDropdownShowImage(
                windowPos + ScreenCoordsXY{ widget->left, widget->top }, widget->height(), colours[1], 0, numTypes, 47, 36,
                itemsPerRow);

            if (defaultIndex)
                gDropdown.defaultIndex = static_cast<int32_t>(*defaultIndex);
        }
    };

    static ObjectEntryIndex PoolGetDefault()
    {
        auto& objManager = GetContext()->GetObjectManager();
        for (ObjectEntryIndex i = 0; i < kMaxPoolObjects; i++)
        {
            if (objManager.GetLoadedObject(ObjectType::pool, i) != nullptr)
                return i;
        }
        return kObjectEntryIndexNull;
    }

    static bool PoolSelectDefault()
    {
        ObjectEntryIndex index = PoolGetDefault();
        if (index == kObjectEntryIndexNull)
            return false;
        gPoolSelection.Pool = index;
        return true;
    }

    WindowBase* PoolOpen()
    {
        if (!PoolSelectDefault())
        {
            ContextShowError(STR_CANT_BUILD_POOL_HERE, STR_OBJECT_SELECTION_POOL, {});
            LOG_WARNING("PoolOpen: no pool object is loaded; cannot open pool window");
            return nullptr;
        }

        auto* windowMgr = GetWindowManager();
        return windowMgr->FocusOrCreate<PoolWindow>(WindowClass::pool, kWindowSize, {});
    }

    void TogglePoolWindow()
    {
        auto* windowMgr = GetWindowManager();
        if (windowMgr->FindByClass(WindowClass::pool) != nullptr)
        {
            windowMgr->CloseByClass(WindowClass::pool);
        }
        else
        {
            PoolOpen();
        }
    }
} // namespace OpenRCT2::Ui::Windows

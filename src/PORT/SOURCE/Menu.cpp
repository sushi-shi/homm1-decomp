// The window menu bar of both programs, drawn by the port.
//
// The original loaded its menus from the executable's resources (LoadMenu)
// and let Windows draw them above the game image in windowed mode; the game
// checks, enables and disables items and receives WM_COMMAND when one is
// chosen. The port keeps that model: the menus come from the same .rc
// scripts (tools/port/menus.py generates kMenuResources), the game's calls
// (KBLoadMenu, KBCheckMenuItem, SetMenus, ...) change the same state, and a
// chosen item runs AppMenuCommand as WM_COMMAND did. The bar and its pop-ups
// are drawn with the game's small font in the display's reserved system
// colours, as a Windows menu would look on the original's 8-bit display, on
// the platform's chrome layer. As in the original, the bar shows only in a
// window, not at full screen.

#include <H1/Ints.h>

#include <SOURCE/kbwin.h>

#include <BASE/Iconm2b.h>
#include <BASE/inputManager.h>
#include <BASE/bitmap.h>
#include <BASE/font.h>
#include <BASE/icon.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/wingraph.h>

#include <PLATFORM/Platform.h>

#include "../PortHost.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct MenuResourceItem {
    i32 depth;
    i32 flags;
    i32 command;
    const char* text;
};

struct MenuResource {
    const char* name;
    const MenuResourceItem* items;
    i32 count;
};

enum MenuResourceFlag {
    MENU_RESOURCE_POPUP = 1,
    MENU_RESOURCE_SEPARATOR = 2,
    MENU_RESOURCE_CHECKED = 4,
    MENU_RESOURCE_GRAYED = 8
};

#ifdef HOMM1_EDITOR
#include "menus_editor.inc"
#else
#include "menus_game.inc"
#endif

// The display's reserved Windows system colours (see wingraph.cpp).
enum MenuColor {
    MENU_COLOR_TEXT = 0,          // black
    MENU_COLOR_HIGHLIGHT = 4,     // navy
    MENU_COLOR_FACE = 7,          // silver
    MENU_COLOR_SHADOW = 248,      // gray
    MENU_COLOR_LIGHT = 255        // white
};

enum MenuLayout {
    MENU_BAR_PADDING = 6,
    MENU_ITEM_PADDING = 2,
    MENU_CHECK_COLUMN = 14,
    MENU_ARROW_COLUMN = 14,
    MENU_SHORTCUT_GAP = 16,
    MENU_SEPARATOR_HEIGHT = 7,
    MENU_POPUP_BORDER = 3
};

struct Rect {
    i32 x = 0;
    i32 y = 0;
    i32 width = 0;
    i32 height = 0;

    bool Contains(i32 px, i32 py) const {
        return px >= x && py >= y && px < x + width && py < y + height;
    }
};

}  // namespace

struct MenuItem {
    std::string text;
    std::string shortcut;
    i32 command = 0;
    bool popup = false;
    bool separator = false;
    bool checked = false;
    bool grayed = false;
    std::vector<MenuItem> children;
    Rect rect;  // display coordinates once laid out
};

struct KBMenuData {
    std::string name;
    std::vector<MenuItem> items;
};

namespace {

font* gFont = NULL;
std::vector<u8> gPixels;
std::vector<u8> gMask;
bitmap* gCanvas = NULL;
i32 gBar = 0;

// The open menu: one item index per level, the first in the bar.
std::vector<i32> gOpen;
std::vector<Rect> gPopupRects;
// The highlighted item per open pop-up, -1 for none.
std::vector<i32> gHover;
bool gTracking = false;
// The command chosen in the open menu, run once the menu has closed.
i32 gChosenCommand = 0;
bool gInMenuLoop = false;

std::string Lowercase(const char* text) {
    std::string result;
    for (const char* cursor = text; *cursor != '\0'; cursor++)
        result += static_cast<char>(*cursor >= 'A' && *cursor <= 'Z' ? *cursor - 'A' + 'a' : *cursor);
    return result;
}

void SplitText(const char* source, MenuItem& item) {
    std::string text;
    for (const char* cursor = source; *cursor != '\0'; cursor++) {
        if (*cursor == '&')
            continue;
        if (*cursor == '\t') {
            item.text = text;
            text.clear();
            continue;
        }
        text += *cursor;
    }
    if (item.text.empty())
        item.text = text;
    else
        item.shortcut = text;
}

// Builds the item tree from a flat, depth-ordered resource.
void Build(const MenuResourceItem* items, i32 count, i32& index, i32 depth,
           std::vector<MenuItem>& out) {
    while (index < count && items[index].depth == depth) {
        const MenuResourceItem& source = items[index++];
        MenuItem item;
        item.command = source.command;
        item.popup = (source.flags & MENU_RESOURCE_POPUP) != 0;
        item.separator = (source.flags & MENU_RESOURCE_SEPARATOR) != 0;
        item.checked = (source.flags & MENU_RESOURCE_CHECKED) != 0;
        item.grayed = (source.flags & MENU_RESOURCE_GRAYED) != 0;
        SplitText(source.text, item);
        if (item.popup)
            Build(items, count, index, depth + 1, item.children);
        out.push_back(item);
    }
}

MenuItem* FindCommand(std::vector<MenuItem>& items, i32 command) {
    for (MenuItem& item : items) {
        if (!item.popup && !item.separator && item.command == command)
            return &item;
        if (item.popup) {
            MenuItem* found = FindCommand(item.children, command);
            if (found != NULL)
                return found;
        }
    }
    return NULL;
}

bool FontReady() {
    if (gFont != NULL)
        return true;
    if (gResourceManager == NULL || gResourceManager->m_active != 1)
        return false;
    gFont = gResourceManager->GetFont(const_cast<char*>("smalfont.fnt"));
    return gFont != NULL;
}

bool Visible() {
    return gAppMenu != NULL && CURRENT_GRAPHICS_CONFIG.showMenu != 0
           && CURRENT_GRAPHICS_CONFIG.fullScreen == 0 && FontReady();
}

i32 GlyphFor(u8 character) {
    i32 glyph = character;
    if (glyph < ' '
        || (glyph > 0x7f && glyph < CYRILLIC_CAPITAL_A && glyph != CYRILLIC_SMALL_YO
            && glyph != CYRILLIC_CAPITAL_YO))
        glyph = 0x7f;
    else if (glyph > 0x7f)
        glyph = RemapCyrillicCharacter(glyph);
    return glyph - ' ';
}

i32 TextWidth(const std::string& text) {
    i16* entries = gFont->m_glyphIcon->m_frameWords;
    i32 width = 0;
    for (char c : text)
        width += entries[GlyphFor(static_cast<u8>(c)) * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                 + FONT_GLYPH_ADVANCE_SPACING;
    return width;
}

// --------------------------------------------------------------- drawing

void Fill(const Rect& rect, u8 color) {
    for (i32 y = std::max(rect.y, -gBar); y < std::min(rect.y + rect.height, static_cast<i32>(LOGICAL_SCREEN_HEIGHT)); y++) {
        for (i32 x = std::max(rect.x, 0); x < std::min(rect.x + rect.width, static_cast<i32>(LOGICAL_SCREEN_WIDTH));
             x++) {
            size_t offset = static_cast<size_t>((y + gBar) * LOGICAL_SCREEN_WIDTH + x);
            gPixels[offset] = color;
            gMask[offset] = 1;
        }
    }
}

void DrawText(const std::string& text, i32 x, i32 y, u8 color) {
    i16* entries = gFont->m_glyphIcon->m_frameWords;
    i32 drawX = x;
    for (char c : text) {
        i32 glyph = GlyphFor(static_cast<u8>(c));
        if (glyph != 0)
            MonoIconToBitmap(gFont->m_glyphIcon, gCanvas, drawX, y + gBar + gFont->m_glyphOffsetY,
                             glyph, color, ICON_DRAW_OFFSET_FULL);
        drawX += entries[glyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]
                 + FONT_GLYPH_ADVANCE_SPACING;
    }
}

void DrawFrame(const Rect& rect) {
    Fill(rect, MENU_COLOR_FACE);
    Fill({rect.x, rect.y, rect.width, 1}, MENU_COLOR_LIGHT);
    Fill({rect.x, rect.y, 1, rect.height}, MENU_COLOR_LIGHT);
    Fill({rect.x, rect.y + rect.height - 1, rect.width, 1}, MENU_COLOR_TEXT);
    Fill({rect.x + rect.width - 1, rect.y, 1, rect.height}, MENU_COLOR_TEXT);
    Fill({rect.x + 1, rect.y + rect.height - 2, rect.width - 2, 1}, MENU_COLOR_SHADOW);
    Fill({rect.x + rect.width - 2, rect.y + 1, 1, rect.height - 2}, MENU_COLOR_SHADOW);
}

void DrawCheck(i32 x, i32 y, u8 color) {
    static const i32 kCheck[][2] = {{0, 3}, {1, 4}, {2, 5}, {3, 4}, {4, 3}, {5, 2}, {6, 1}};
    for (const auto& point : kCheck)
        Fill({x + point[0], y + point[1], 1, 2}, color);
}

void DrawArrow(i32 x, i32 y, u8 color) {
    for (i32 column = 0; column < 4; column++)
        Fill({x + column, y + column, 1, 7 - 2 * column}, color);
}

void DrawItemText(const MenuItem& item, i32 x, i32 y, u8 color) {
    if (item.grayed && color != MENU_COLOR_LIGHT) {
        DrawText(item.text, x + 1, y + 1, MENU_COLOR_LIGHT);
        color = MENU_COLOR_SHADOW;
    }
    DrawText(item.text, x, y, color);
}

void LayoutPopup(std::vector<MenuItem>& items, i32 x, i32 y, Rect& rect) {
    i32 textWidth = 0;
    i32 shortcutWidth = 0;
    i32 lineHeight = gFont->m_height + MENU_ITEM_PADDING * 2;
    i32 height = MENU_POPUP_BORDER;
    for (MenuItem& item : items) {
        textWidth = std::max(textWidth, TextWidth(item.text));
        shortcutWidth = std::max(shortcutWidth, TextWidth(item.shortcut));
    }
    i32 width = MENU_POPUP_BORDER * 2 + MENU_CHECK_COLUMN + textWidth
                + (shortcutWidth > 0 ? MENU_SHORTCUT_GAP + shortcutWidth : 0) + MENU_ARROW_COLUMN;
    for (MenuItem& item : items) {
        i32 itemHeight = item.separator ? MENU_SEPARATOR_HEIGHT : lineHeight;
        item.rect = {x + MENU_POPUP_BORDER, y + height, width - MENU_POPUP_BORDER * 2, itemHeight};
        height += itemHeight;
    }
    height += MENU_POPUP_BORDER;
    // Keep the pop-up on the display.
    i32 shiftX = std::max(0, x + width - LOGICAL_SCREEN_WIDTH);
    i32 shiftY = std::max(0, y + height - LOGICAL_SCREEN_HEIGHT);
    for (MenuItem& item : items) {
        item.rect.x -= shiftX;
        item.rect.y -= shiftY;
    }
    rect = {x - shiftX, y - shiftY, width, height};
}

void DrawPopup(const std::vector<MenuItem>& items, const Rect& rect, i32 hover) {
    DrawFrame(rect);
    for (size_t index = 0; index < items.size(); index++) {
        const MenuItem& item = items[index];
        if (item.separator) {
            i32 middle = item.rect.y + item.rect.height / 2;
            Fill({item.rect.x + 1, middle - 1, item.rect.width - 2, 1}, MENU_COLOR_SHADOW);
            Fill({item.rect.x + 1, middle, item.rect.width - 2, 1}, MENU_COLOR_LIGHT);
            continue;
        }
        bool lit = static_cast<i32>(index) == hover;
        u8 textColor = MENU_COLOR_TEXT;
        if (lit) {
            Fill(item.rect, MENU_COLOR_HIGHLIGHT);
            textColor = item.grayed ? MENU_COLOR_SHADOW : MENU_COLOR_LIGHT;
        }
        i32 textY = item.rect.y + MENU_ITEM_PADDING;
        if (item.checked)
            DrawCheck(item.rect.x + 3, textY + gFont->m_height / 2 - 4, textColor);
        DrawItemText(item, item.rect.x + MENU_CHECK_COLUMN, textY, textColor);
        if (!item.shortcut.empty())
            DrawText(item.shortcut,
                     item.rect.x + item.rect.width - MENU_ARROW_COLUMN - TextWidth(item.shortcut),
                     textY, textColor);
        if (item.popup)
            DrawArrow(item.rect.x + item.rect.width - MENU_ARROW_COLUMN + 4,
                      textY + gFont->m_height / 2 - 3, textColor);
    }
}

std::vector<MenuItem>* LevelItems(size_t level) {
    std::vector<MenuItem>* items = &gAppMenu->items;
    for (size_t i = 0; i < level; i++)
        items = &(*items)[static_cast<size_t>(gOpen[i])].children;
    return items;
}

void Redraw() {
    bool visible = Visible();
    i32 bar = visible ? gFont->m_height + MENU_ITEM_PADDING * 2 + 4 : 0;
    if (bar != gBar) {
        gBar = bar;
        platform::SetChromeBar(gBar);
    }
    if (!visible) {
        gOpen.clear();
        gHover.clear();
        gPopupRects.clear();
        gTracking = false;
        return;
    }
    size_t size = static_cast<size_t>(LOGICAL_SCREEN_WIDTH * (gBar + LOGICAL_SCREEN_HEIGHT));
    gPixels.assign(size, 0);
    gMask.assign(size, 0);
    if (gCanvas == NULL)
        gCanvas = new bitmap();
    gCanvas->m_width = LOGICAL_SCREEN_WIDTH;
    gCanvas->m_height = static_cast<i16>(gBar + LOGICAL_SCREEN_HEIGHT);
    gCanvas->m_pixels = gPixels.data();

    // The bar.
    Fill({0, -gBar, LOGICAL_SCREEN_WIDTH, gBar}, MENU_COLOR_FACE);
    Fill({0, -1, LOGICAL_SCREEN_WIDTH, 1}, MENU_COLOR_SHADOW);
    i32 x = MENU_BAR_PADDING / 2;
    std::vector<MenuItem>& top = gAppMenu->items;
    for (size_t index = 0; index < top.size(); index++) {
        MenuItem& item = top[index];
        item.rect = {x, -gBar + 1, TextWidth(item.text) + MENU_BAR_PADDING * 2, gBar - 3};
        bool open = !gOpen.empty() && gOpen[0] == static_cast<i32>(index);
        u8 color = MENU_COLOR_TEXT;
        if (open) {
            Fill(item.rect, MENU_COLOR_HIGHLIGHT);
            color = MENU_COLOR_LIGHT;
        }
        DrawItemText(item, x + MENU_BAR_PADDING, -gBar + 1 + MENU_ITEM_PADDING + 1, color);
        x += item.rect.width;
    }

    // The open pop-ups, each beside the item that opened it.
    gPopupRects.clear();
    for (size_t level = 0; level < gOpen.size(); level++) {
        std::vector<MenuItem>& items = *LevelItems(level);
        MenuItem& opener = items[static_cast<size_t>(gOpen[level])];
        if (!opener.popup)
            break;
        Rect rect;
        if (level == 0)
            LayoutPopup(opener.children, opener.rect.x, 0, rect);
        else
            LayoutPopup(opener.children, opener.rect.x + opener.rect.width - 2,
                        opener.rect.y - MENU_POPUP_BORDER, rect);
        gPopupRects.push_back(rect);
        DrawPopup(opener.children, rect, level < gHover.size() ? gHover[level] : -1);
    }
    platform::UpdateChrome(gPixels.data(), gMask.data());
}

void CloseMenu() {
    gOpen.clear();
    gHover.clear();
    gTracking = false;
    Redraw();
}

void OpenBarItem(i32 index) {
    gOpen.assign(1, index);
    gHover.assign(1, -1);
    Redraw();
}

// Which open pop-up level and item a point is on; false when on none.
bool HitPopup(i32 x, i32 y, size_t& level, i32& item) {
    for (size_t candidate = gPopupRects.size(); candidate-- > 0;) {
        if (!gPopupRects[candidate].Contains(x, y))
            continue;
        level = candidate;
        item = -1;
        std::vector<MenuItem>& items =
            (*LevelItems(candidate))[static_cast<size_t>(gOpen[candidate])].children;
        for (size_t index = 0; index < items.size(); index++) {
            if (items[index].rect.Contains(x, y))
                item = static_cast<i32>(index);
        }
        return true;
    }
    return false;
}

i32 HitBar(i32 x, i32 y) {
    if (y >= 0)
        return -1;
    for (size_t index = 0; index < gAppMenu->items.size(); index++) {
        if (gAppMenu->items[index].rect.Contains(x, y))
            return static_cast<i32>(index);
    }
    return -1;
}

void Hover(size_t level, i32 item) {
    // Pop-ups deeper than the hovered one close; a hovered sub-menu opens.
    gOpen.resize(level + 1);
    gHover.resize(level + 1);
    gHover[level] = item;
    if (item >= 0) {
        std::vector<MenuItem>& items =
            (*LevelItems(level))[static_cast<size_t>(gOpen[level])].children;
        if (items[static_cast<size_t>(item)].popup && !items[static_cast<size_t>(item)].grayed) {
            gOpen.push_back(item);
            gHover.push_back(-1);
        }
    }
    Redraw();
}

void Choose(size_t level, i32 index) {
    std::vector<MenuItem>& items = (*LevelItems(level))[static_cast<size_t>(gOpen[level])].children;
    MenuItem& item = items[static_cast<size_t>(index)];
    if (item.separator || item.popup || item.grayed)
        return;
    gChosenCommand = item.command;
    CloseMenu();
}

// Windows tracks an open menu in a modal loop: the program's own loop does
// not run until the menu closes, and the chosen command arrives afterwards as
// WM_COMMAND. The game relies on that - it polls the pointer, and would
// scroll the map under an open menu.
void RunMenuLoop() {
    bool quit = false;
    gInMenuLoop = true;
    while (!gOpen.empty()) {
        platform::Event event;
        while (!gOpen.empty() && platform::PollEvent(event, 10)) {
            if (event.type == platform::Event::QUIT) {
                quit = true;
                CloseMenu();
                break;
            }
            MenuHandleEvent(event);
        }
        platform::Present(false);
    }
    gInMenuLoop = false;
    i32 command = gChosenCommand;
    gChosenCommand = 0;
    if (quit)
        KBRequestClose();
    else if (command != 0)
        AppMenuCommand(command);
}

}  // namespace

// ---------------------------------------------------------------- input

bool MenuHandleEvent(const platform::Event& event) {
    if (gBar == 0 || gAppMenu == NULL)
        return false;
    bool open = !gOpen.empty();
    switch (event.type) {
        case platform::Event::KEY_DOWN:
        case platform::Event::KEY_UP:
            if (!open)
                return false;
            if (event.type == platform::Event::KEY_DOWN && event.scanCode == INPUT_SCAN_ESCAPE)
                CloseMenu();
            return true;
        case platform::Event::MOUSE_MOVE: {
            if (!open)
                return event.y < 0;
            size_t level;
            i32 item;
            i32 bar = HitBar(event.x, event.y);
            if (bar >= 0 && bar != gOpen[0])
                OpenBarItem(bar);
            else if (HitPopup(event.x, event.y, level, item)
                     && (level + 1 > gHover.size() || gHover[level] != item))
                Hover(level, item);
            return true;
        }
        case platform::Event::MOUSE_DOWN: {
            i32 bar = HitBar(event.x, event.y);
            size_t level;
            i32 item;
            if (bar >= 0) {
                if (open && gOpen[0] == bar)
                    CloseMenu();
                else
                    OpenBarItem(bar);
                gTracking = true;
                if (!gInMenuLoop && !gOpen.empty())
                    RunMenuLoop();
                return true;
            }
            if (open && HitPopup(event.x, event.y, level, item)) {
                gTracking = true;
                return true;
            }
            if (open) {
                CloseMenu();
                return true;
            }
            return event.y < 0;
        }
        case platform::Event::MOUSE_UP: {
            size_t level;
            i32 item;
            bool tracking = gTracking;
            gTracking = false;
            if (open && HitPopup(event.x, event.y, level, item)) {
                if (item >= 0)
                    Choose(level, item);
                return true;
            }
            return open || tracking || event.y < 0;
        }
        default:
            return false;
    }
}

void MenuRefresh() {
    if (gAppMenu == NULL || !Visible()) {
        if (gBar != 0)
            Redraw();
        return;
    }
    Redraw();
}

void MenuShutdown() {
    gFont = NULL;
    gOpen.clear();
    gHover.clear();
    if (gCanvas != NULL) {
        gCanvas->m_pixels = NULL;
        delete gCanvas;
        gCanvas = NULL;
    }
    if (gBar != 0) {
        gBar = 0;
        platform::SetChromeBar(0);
    }
}

// ---------------------------------------------------------------- model

KBMenu KBLoadMenu(const char* name) {
    std::string wanted = Lowercase(name);
    for (const MenuResource& resource : kMenuResources) {
        if (wanted != resource.name)
            continue;
        KBMenu menu = new KBMenuData;
        menu->name = resource.name;
        i32 index = 0;
        Build(resource.items, resource.count, index, 0, menu->items);
        return menu;
    }
    return NULL;
}

void KBDestroyMenu(KBMenu menu) {
    if (menu == gAppMenu)
        CloseMenu();
    delete menu;
}

void KBDetachMenu(void) {
    gOpen.clear();
    gHover.clear();
    if (gBar != 0) {
        gBar = 0;
        platform::SetChromeBar(0);
    }
}

void KBCheckMenuItem(KBMenu menu, i32 command, i32 checked) {
    if (menu == NULL)
        return;
    MenuItem* item = FindCommand(menu->items, command);
    if (item == NULL || item->checked == (checked != 0))
        return;
    item->checked = checked != 0;
    if (menu == gAppMenu && !gOpen.empty())
        Redraw();
}

// Windows' EnableMenuItem.
void MenuEnableItem(KBMenu menu, i32 command, bool enabled) {
    MenuItem* item = menu != NULL ? FindCommand(menu->items, command) : NULL;
    if (item == NULL || item->grayed == !enabled)
        return;
    item->grayed = !enabled;
    if (menu == gAppMenu && !gOpen.empty())
        Redraw();
}

namespace {

void SetMenuItems(std::vector<MenuItem>& items, KBMenu menu, i32 enabled) {
    for (MenuItem& item : items) {
        if (item.popup) {
            SetMenuItems(item.children, menu, enabled);
            continue;
        }
        if (item.separator)
            continue;
        // The original's rule: enabling enables everything; disabling greys
        // the commands the enable table marks as unavailable while a dialog
        // (or the setup dialog) is open.
        bool update = false;
        if (enabled) {
            update = true;
        } else {
            i32 match = 0;
            for (i32 entry = 0; entry < KBWIN_MENU_ENTRY_COUNT; entry++) {
                if (static_cast<i32>(gMenuEnableStatus[entry].command) == item.command)
                    match = entry;
            }
            update = (gInSetupDialog ? 1 - gMenuEnableStatus[match].setupEnabled
                                     : 1 - gMenuEnableStatus[match].normalEnabled)
                     != 0;
        }
        if (update)
            item.grayed = enabled == 0;
    }
}

}  // namespace

void SetMenus(KBMenu menu, i32 enabled) {
    if (menu == NULL)
        return;
    SetMenuItems(menu->items, menu, enabled);
    UpdateDfltMenu(menu);
    if (menu == gAppMenu)
        MenuRefresh();
}

// The window sizes larger than the desktop are unavailable, as in the
// original.
void UpdateDfltMenu(KBMenu menu) {
    if (menu == NULL || CURRENT_GRAPHICS_CONFIG.showMenu == 0)
        return;
    i32 desktopWidth;
    i32 desktopHeight;
    platform::DesktopSize(desktopWidth, desktopHeight);
    if (desktopWidth <= LOGICAL_SCREEN_WIDTH)
        MenuEnableItem(menu, KBWIN_MENU_SIZE_640_480, false);
    if (desktopWidth <= KBWIN_WIDTH_800)
        MenuEnableItem(menu, KBWIN_MENU_SIZE_800_600, false);
    if (desktopWidth <= KBWIN_WIDTH_1024)
        MenuEnableItem(menu, KBWIN_MENU_SIZE_1024_768, false);
    if (desktopWidth <= KBWIN_WIDTH_1280)
        MenuEnableItem(menu, KBWIN_MENU_SIZE_1280_1024, false);
    // The help book is shown converted from the game's WinHelp file; without
    // that file the item is unavailable.
    if (!HelpAvailable())
        MenuEnableItem(menu, KBWIN_MENU_HELP, false);
}

std::string MenuAboutText() {
    std::string text;
    for (const char* line : kAboutLines) {
        if (!text.empty())
            text += '\n';
        text += line;
    }
    return text;
}

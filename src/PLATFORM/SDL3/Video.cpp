#include "Internal.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>

namespace platform {

namespace {

constexpr int kWidth = DISPLAY_WIDTH;
constexpr int kHeight = DISPLAY_HEIGHT;
// The canvas holds the chrome bar above the game image.
constexpr int kCanvasHeight = kHeight + CHROME_BAR_MAX;
// Presentation is coalesced to this interval unless forced: the game copies
// many small rectangles per frame, the way DirectDraw blits were cheap.
constexpr u32 kPresentInterval = 8;

SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;
SDL_Texture* gTexture = nullptr;
bool gFullscreen = false;

std::array<u8, kWidth * kHeight> gIndexed{};
std::array<u32, kWidth * kCanvasHeight> gPixels{};
std::array<u8, kWidth * kCanvasHeight> gChrome{};
std::array<u8, kWidth * kCanvasHeight> gChromeMask{};
int gBar = 0;
std::array<Color, PALETTE_SIZE> gPalette{};
bool gDirty = false;
u32 gLastPresent = 0;

CursorImage gCursor;
bool gCursorSet = false;
int gCursorCount = 0;
int gPointerX = 0;
int gPointerY = 0;
bool gPointerInside = false;

bool Bit(const u8* mask, int x, int y) {
    return (mask[y * 4 + (x >> 3)] & (0x80 >> (x & 7))) != 0;
}

u32 Rgb(const Color& color) {
    return (static_cast<u32>(color.r) << 16) | (static_cast<u32>(color.g) << 8) | color.b;
}

void DrawCursor() {
    if (!gCursorSet || gCursorCount < 0 || !gPointerInside)
        return;
    for (int y = 0; y < CURSOR_SIZE; y++) {
        int screenY = gPointerY + gBar - gCursor.hotY + y;
        if (screenY < 0 || screenY >= kHeight + gBar)
            continue;
        for (int x = 0; x < CURSOR_SIZE; x++) {
            int screenX = gPointerX - gCursor.hotX + x;
            if (screenX < 0 || screenX >= kWidth)
                continue;
            u32& pixel = gPixels[static_cast<size_t>(screenY * kWidth + screenX)];
            bool andBit = Bit(gCursor.andMask, x, y);
            bool xorBit = Bit(gCursor.xorMask, x, y);
            if (andBit) {
                if (xorBit)
                    pixel ^= 0xFFFFFFu;
            } else if (gCursor.colorCursor) {
                pixel = Rgb(gPalette[gCursor.color[y * CURSOR_SIZE + x]]);
            } else {
                pixel = xorBit ? 0xFFFFFFu : 0;
            }
        }
    }
}

void UpdateSystemCursor() {
    if (gWindow == nullptr)
        return;
    // The game's own pointer is drawn into the display; the host's arrow
    // stands in only when the game has none.
    if (gCursorSet || gCursorCount < 0)
        SDL_HideCursor();
    else
        SDL_ShowCursor();
}

int ChooseScale() {
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    SDL_Rect bounds;
    if (display == 0 || !SDL_GetDisplayUsableBounds(display, &bounds))
        return 1;
    int scale = std::min((bounds.w - 32) / kWidth, (bounds.h - 64) / (kHeight + gBar));
    return std::max(1, std::min(scale, 3));
}

}  // namespace

namespace sdl {

SDL_Window* Window() {
    return gWindow;
}

SDL_Renderer* Renderer() {
    return gRenderer;
}

void SetPointer(int x, int y, bool inside) {
    if (x == gPointerX && y == gPointerY && inside == gPointerInside)
        return;
    gPointerX = x;
    gPointerY = y;
    gPointerInside = inside;
    if (gCursorSet)
        gDirty = true;
}

void PointerPosition(int& x, int& y, bool& inside) {
    x = gPointerX;
    y = gPointerY;
    inside = gPointerInside;
}

void WindowToDisplay(float windowX, float windowY, int& x, int& y, bool& inside) {
    float logicalX = windowX;
    float logicalY = windowY;
    if (gRenderer != nullptr)
        SDL_RenderCoordinatesFromWindow(gRenderer, windowX, windowY, &logicalX, &logicalY);
    x = static_cast<int>(logicalX);
    y = static_cast<int>(logicalY);
    y -= gBar;
    inside = x >= 0 && y >= -gBar && x < kWidth && y < kHeight;
    x = std::clamp(x, 0, kWidth - 1);
    y = std::clamp(y, -gBar, kHeight - 1);
}

void MarkDisplayDirty() {
    gDirty = true;
}

}  // namespace sdl

bool OpenDisplay() {
    if (gWindow != nullptr)
        return true;
    StartupOptions defaults;
    int scale = ChooseScale();
    std::string requested = Environment("HOMM1_SCALE");
    if (!requested.empty())
        scale = std::max(1, std::atoi(requested.c_str()));
#ifdef __EMSCRIPTEN__
    // In a page the canvas keeps the image's own size and the page scales
    // it to the browser window (src/PLATFORM/Web/homm1.js). A resizable SDL
    // window would instead follow the canvas's shown size and letterbox the
    // image inside it.
    scale = 1;
    SDL_WindowFlags flags = 0;
#else
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
#endif
    if (!SDL_CreateWindowAndRenderer(
            defaults.title, kWidth * scale, (kHeight + gBar) * scale, flags, &gWindow, &gRenderer)) {
        Log("cannot open a window: %s", SDL_GetError());
        return false;
    }
    SDL_SetRenderVSync(gRenderer, 0);
    SDL_SetRenderLogicalPresentation(gRenderer, kWidth, kHeight + gBar,
                                     SDL_LOGICAL_PRESENTATION_LETTERBOX);
    gTexture = SDL_CreateTexture(
        gRenderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, kWidth, kCanvasHeight);
    if (gTexture == nullptr) {
        Log("cannot create the display texture: %s", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode(gTexture, SDL_SCALEMODE_NEAREST);
    if (gFullscreen)
        SDL_SetWindowFullscreen(gWindow, true);
    UpdateSystemCursor();
    gDirty = true;
    Present(true);
    return true;
}

void CloseDisplay() {
    if (gTexture != nullptr)
        SDL_DestroyTexture(gTexture);
    if (gRenderer != nullptr)
        SDL_DestroyRenderer(gRenderer);
    if (gWindow != nullptr)
        SDL_DestroyWindow(gWindow);
    gTexture = nullptr;
    gRenderer = nullptr;
    gWindow = nullptr;
}

bool DisplayOpen() {
    return gWindow != nullptr;
}

void UpdateDisplay(
    const u8* pixels,
    int pitch,
    int sourceX,
    int sourceY,
    int width,
    int height,
    int destinationX,
    int destinationY
) {
    if (pixels == nullptr)
        return;
    // Clip against the display; the source is the game's 640x480 buffer, so
    // the same clip keeps the source in range.
    if (destinationX < 0) {
        sourceX -= destinationX;
        width += destinationX;
        destinationX = 0;
    }
    if (destinationY < 0) {
        sourceY -= destinationY;
        height += destinationY;
        destinationY = 0;
    }
    if (sourceX < 0) {
        destinationX -= sourceX;
        width += sourceX;
        sourceX = 0;
    }
    if (sourceY < 0) {
        destinationY -= sourceY;
        height += sourceY;
        sourceY = 0;
    }
    width = std::min({width, kWidth - destinationX, pitch - sourceX});
    height = std::min({height, kHeight - destinationY, kHeight - sourceY});
    if (width <= 0 || height <= 0)
        return;
    for (int row = 0; row < height; row++)
        std::memcpy(
            &gIndexed[static_cast<size_t>((destinationY + row) * kWidth + destinationX)],
            pixels + static_cast<size_t>((sourceY + row) * pitch + sourceX),
            static_cast<size_t>(width));
    gDirty = true;
    Present(false);
}

void SetPalette(const Color* colors, int first, int count) {
    for (int i = 0; i < count && first + i < PALETTE_SIZE; i++)
        gPalette[static_cast<size_t>(first + i)] = colors[i];
    gDirty = true;
    Present(false);
}

void Present(bool force) {
    if (gRenderer == nullptr || !gDirty)
        return;
    u32 now = Ticks();
    if (!force && now - gLastPresent < kPresentInterval)
        return;
    gLastPresent = now;
    gDirty = false;
    std::array<u32, PALETTE_SIZE> lookup;
    for (size_t i = 0; i < lookup.size(); i++)
        lookup[i] = Rgb(gPalette[i]);
    const size_t barPixels = static_cast<size_t>(gBar * kWidth);
    for (size_t i = 0; i < barPixels; i++)
        gPixels[i] = lookup[gChrome[i]];
    for (size_t i = 0; i < gIndexed.size(); i++) {
        size_t canvas = barPixels + i;
        gPixels[canvas] = lookup[gChromeMask[canvas] != 0 ? gChrome[canvas] : gIndexed[i]];
    }
    DrawCursor();
    SDL_UpdateTexture(gTexture, nullptr, gPixels.data(), kWidth * static_cast<int>(sizeof(u32)));
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gRenderer);
    SDL_FRect source = {0, 0, static_cast<float>(kWidth), static_cast<float>(kHeight + gBar)};
    SDL_RenderTexture(gRenderer, gTexture, &source, nullptr);
    SDL_RenderPresent(gRenderer);
}

void SetWindowSize(int width, int height) {
    if (gWindow == nullptr || gFullscreen || width <= 0 || height <= 0)
        return;
    SDL_SetWindowSize(gWindow, width, height + gBar * height / kHeight);
    gDirty = true;
}

void DesktopSize(int& width, int& height) {
    SDL_Rect bounds = {0, 0, kWidth, kHeight};
    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    if (display != 0)
        SDL_GetDisplayUsableBounds(display, &bounds);
    width = bounds.w;
    height = bounds.h;
}

void SetChromeBar(int height) {
    height = std::clamp(height, 0, static_cast<int>(CHROME_BAR_MAX));
    if (height == gBar)
        return;
    int windowWidth = 0;
    int windowHeight = 0;
    if (gWindow != nullptr)
        SDL_GetWindowSize(gWindow, &windowWidth, &windowHeight);
    int oldBar = gBar;
    gBar = height;
    gChromeMask.fill(0);
    if (gRenderer != nullptr) {
        SDL_SetRenderLogicalPresentation(gRenderer, kWidth, kHeight + gBar,
                                         SDL_LOGICAL_PRESENTATION_LETTERBOX);
        if (!gFullscreen && windowHeight > 0) {
            int imageHeight = windowHeight * kHeight / (kHeight + oldBar);
            SDL_SetWindowSize(gWindow, windowWidth, imageHeight + gBar * imageHeight / kHeight);
        }
    }
    gDirty = true;
}

int ChromeBar() {
    return gBar;
}

void UpdateChrome(const u8* pixels, const u8* mask) {
    size_t count = static_cast<size_t>(kWidth * (kHeight + gBar));
    std::memcpy(gChrome.data(), pixels, count);
    std::memcpy(gChromeMask.data(), mask, count);
    std::fill(gChromeMask.begin() + static_cast<std::ptrdiff_t>(count), gChromeMask.end(), u8{0});
    gDirty = true;
}

void SetFullscreen(bool fullscreen) {
    gFullscreen = fullscreen;
    if (gWindow != nullptr)
        SDL_SetWindowFullscreen(gWindow, fullscreen);
    gDirty = true;
}

bool Fullscreen() {
    return gFullscreen;
}

void CaptureDisplay(u8* rgb) {
    const int barPixels = gBar * kWidth;
    const int count = kWidth * (kHeight + gBar);
    for (int i = 0; i < count; i++) {
        u8 index = i < barPixels || gChromeMask[static_cast<size_t>(i)] != 0
                       ? gChrome[static_cast<size_t>(i)]
                       : gIndexed[static_cast<size_t>(i - barPixels)];
        const Color& color = gPalette[index];
        rgb[i * 3] = color.r;
        rgb[i * 3 + 1] = color.g;
        rgb[i * 3 + 2] = color.b;
    }
}

bool SaveDisplayBmp(const char* hostPath) {
    const int height = kHeight + gBar;
    std::vector<u8> rgb(static_cast<size_t>(kWidth * height * 3));
    CaptureDisplay(rgb.data());
    SDL_IOStream* file = SDL_IOFromFile(hostPath, "wb");
    if (file == nullptr)
        return false;
    const u32 rowBytes = kWidth * 3;
    const u32 imageBytes = rowBytes * static_cast<u32>(height);
    u8 header[54] = {'B', 'M'};
    auto put32 = [&header](int offset, u32 value) {
        for (int i = 0; i < 4; i++)
            header[offset + i] = static_cast<u8>(value >> (8 * i));
    };
    put32(2, 54 + imageBytes);
    put32(10, 54);
    put32(14, 40);
    put32(18, kWidth);
    put32(22, static_cast<u32>(height));
    header[26] = 1;
    header[28] = 24;
    put32(34, imageBytes);
    bool ok = SDL_WriteIO(file, header, sizeof(header)) == sizeof(header);
    std::vector<u8> row(rowBytes);
    for (int y = height - 1; y >= 0 && ok; y--) {
        for (int x = 0; x < kWidth; x++) {
            const u8* source = &rgb[static_cast<size_t>((y * kWidth + x) * 3)];
            row[static_cast<size_t>(x * 3)] = source[2];
            row[static_cast<size_t>(x * 3 + 1)] = source[1];
            row[static_cast<size_t>(x * 3 + 2)] = source[0];
        }
        ok = SDL_WriteIO(file, row.data(), row.size()) == row.size();
    }
    return SDL_CloseIO(file) && ok;
}

void SetCursorImage(const CursorImage* image) {
    gCursorSet = image != nullptr;
    if (image != nullptr)
        gCursor = *image;
    UpdateSystemCursor();
    gDirty = true;
}

void ShowCursor(bool show) {
    gCursorCount += show ? 1 : -1;
    UpdateSystemCursor();
    gDirty = true;
}

}  // namespace platform

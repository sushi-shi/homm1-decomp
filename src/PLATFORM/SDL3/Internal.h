#ifndef HOMM1_PLATFORM_SDL3_INTERNAL_H
#define HOMM1_PLATFORM_SDL3_INTERNAL_H

// State shared by the SDL3 backend's units.

#include <PLATFORM/Platform.h>

#include <SDL3/SDL.h>

namespace platform::sdl {

SDL_Window* Window();
SDL_Renderer* Renderer();

// The pointer position in display coordinates, kept by the input unit and
// drawn over the display by the video unit.
void SetPointer(int x, int y, bool inside);
void PointerPosition(int& x, int& y, bool& inside);

// Converts window coordinates to display coordinates.
void WindowToDisplay(float windowX, float windowY, int& x, int& y, bool& inside);

void MarkDisplayDirty();

}  // namespace platform::sdl

#endif

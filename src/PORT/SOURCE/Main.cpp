// The native programs' entry point (the original's WinMain is in kbwin.cpp).

#include <H1/Ints.h>

#include "../PortHost.h"

#if defined(_WIN32)
// SDL supplies WinMain, which passes the command line on as UTF-8.
#include <SDL3/SDL_main.h>
#endif

int main(int argc, char** argv) {
    return KBRunProgram(argc, argv);
}

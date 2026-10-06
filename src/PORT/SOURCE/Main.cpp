// The native programs' entry point (the original's WinMain is in kbwin.cpp).

#include <H1/Ints.h>

#include "../PortHost.h"

int main(int argc, char** argv) {
    return KBRunProgram(argc, argv);
}

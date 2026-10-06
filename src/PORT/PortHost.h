#ifndef HOMM1_PORT_PORTHOST_H
#define HOMM1_PORT_PORTHOST_H

// What the port's host units share beyond the game's own host interfaces.

#include <PLATFORM/Platform.h>

#include <SOURCE/kbwin.h>

#include <string>

// Starts the host and the program's early setup, as WinMain and AppInit did:
// finds the game data (dataRoot, else the usual places), reads the settings,
// interprets the original's command-line switches and opens the display.
// fullScreen: 0 or 1 overrides the saved choice, -1 keeps it.
bool KBStartHost(const char* dataRoot, const char* gameArguments, i32 fullScreen);
// The whole program: parses argv, starts the host and runs the game or editor.
i32 KBRunProgram(int argc, char** argv);

// The folder holding the CD's TRACKS folder, or empty when there is none.
const std::string& CdRoot();

// The window menu bar (Menu.cpp). MenuHandleEvent takes the events meant for
// the menu and returns true for them; MenuRefresh redraws the bar after the
// menu, its items or the window mode changed.
bool MenuHandleEvent(const platform::Event& event);
void MenuRefresh();
void MenuShutdown();
void MenuEnableItem(KBMenu menu, i32 command, bool enabled);
// The About box's text, one line per entry.
std::string MenuAboutText();

#endif

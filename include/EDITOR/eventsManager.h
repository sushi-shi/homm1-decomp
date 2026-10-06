#ifndef HOMM1_EDITOR_EVENTSMANAGER_H
#define HOMM1_EDITOR_EVENTSMANAGER_H

// The map-information tool and the editor dialogs of its unit
// (src/EDITOR/EVENTMGR.cpp). The unit name EVENTMGR is descriptive; Open
// stores the class name "eventsManager".

#include <Domains.h>

// Runs the eraser options window (clearwin.bin); on OK with the whole-map
// toggle set it erases the selected classes everywhere. Returns 1 on OK.
i32 ClearOptionsDialog(void);

#endif // HOMM1_EDITOR_EVENTSMANAGER_H

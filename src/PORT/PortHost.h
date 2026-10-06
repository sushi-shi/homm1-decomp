#ifndef HOMM1_PORT_PORTHOST_H
#define HOMM1_PORT_PORTHOST_H

// What the port's host units share beyond the game's own host interfaces.

#include <string>

// The folder holding the CD's TRACKS folder, or empty when there is none.
const std::string& CdRoot();

#endif

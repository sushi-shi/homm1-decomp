#ifndef HOMM1_PLATFORM_GAMEDATA_H
#define HOMM1_PLATFORM_GAMEDATA_H

// Where the game's data and the CD's music are on the host.

#include <string>
#include <vector>

namespace platform {

// The game folder (the one holding DATA\HEROES.AGG, in any case): `requested`
// (--data), $HOMM1_DATA, the executable's folder, the current folder, then
// the folder `nix run .#play` installs to ($XDG_DATA_HOME/homm1-buka/game,
// ~/.local/share/homm1-buka/game; on Windows %LOCALAPPDATA%\homm1-buka\game).
// Paths given by the user are taken through ConfiguredDirectory. Returns the
// first that holds the data, or empty; `searched` lists every folder tried,
// in order.
std::string FindGameRoot(const std::string& requested, std::vector<std::string>& searched);

// The CD's music tracks (Tracks\02-AudioTrack 02.ogg): $HOMM1_CD, a "cd"
// folder beside the game folder (where `nix run .#play` puts it), or the
// game folder itself; empty when none has them.
std::string FindCdRoot(const std::string& gameRoot);

}  // namespace platform

#endif

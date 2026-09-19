#ifndef HOMM1_SOURCE_PLAYERDATA_H
#define HOMM1_SOURCE_PLAYERDATA_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 6 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>

class playerData {
public:
    // --- methods ---
    void Write(int);
    void Read(int);
    int NextHero(int);
    int HasMobileHero(void);
    int BuildingsOwned(int, int, int);
    int NumOfGivenArtifact(int);
};
#endif // HOMM1_SOURCE_PLAYERDATA_H

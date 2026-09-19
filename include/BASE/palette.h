#ifndef HOMM1_BASE_PALETTE_H
#define HOMM1_BASE_PALETTE_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 5 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>

class palette {
public:
    // --- constructors ---
    palette(void);
    palette(unsigned long int);
    virtual ~palette();
    // --- methods ---
    signed char * Data(void);
};
#endif // HOMM1_BASE_PALETTE_H

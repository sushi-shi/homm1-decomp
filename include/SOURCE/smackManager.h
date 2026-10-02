#ifndef HOMM1_SOURCE_SMACKMANAGER_H
#define HOMM1_SOURCE_SMACKMANAGER_H

#include <BASE/baseManager.h>
#include <H1/Macros.h>

struct tag_message;

// InitMainClasses allocates 0x32 bytes: the 0x30-byte baseManager prefix and
// one trailing word.
#pragma pack(push, 1)
class smackManager : public baseManager {
public:
    short m_unknown30;

    smackManager(void);
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message &) OVERRIDE;
};
#pragma pack(pop)

extern smackManager *gpSmackManager;
extern signed char bSmackNum;
extern signed char gbSmackAborted;
extern int gbInSmacker;

H1_C_LINKAGE void *radmalloc(unsigned long);
H1_C_LINKAGE void radfree(void *);
void PlaySmacker(signed char);

// One 0x16-byte row per movie: file name, the window manager's update mode
// while it plays, the fades around it and its SmackOpen flags (Buka
// SMACKMGR.h).
#pragma pack(push, 1)
struct SSmackOptions {
    char fileName[15];
    signed char updateFlags;
    signed char fadeIn;
    signed char fadeOut;
    unsigned long openFlags;
};
#pragma pack(pop)

extern SSmackOptions SmackOptions[];

#endif

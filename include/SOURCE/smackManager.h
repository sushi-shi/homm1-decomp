#ifndef HOMM1_SOURCE_SMACKMANAGER_H
#define HOMM1_SOURCE_SMACKMANAGER_H

#include <BASE/baseManager.h>
#include <SOURCE/smack.h>

struct tag_message;

enum SmackVideo {
    SMACK_NWCLOGO = 0,
    SMACK_NWCLOGO1 = 1,
    SMACK_LOGO_LAST = SMACK_NWCLOGO1,
    SMACK_INTRO02C = 2,
    SMACK_INTRO_FIRST = SMACK_INTRO02C,
    SMACK_INTRO02U = 3,
    SMACK_INTRO_LAST = SMACK_INTRO02U,
    SMACK_WIN01C = 4,
    SMACK_WIN01U = 5,
    SMACK_WIN02 = 6,
    SMACK_LOSE1 = 7
};

#pragma pack(push, 1)
class smackManager : public baseManager {
public:
    i16 m_unknown30;

    smackManager(void);
    virtual i16 Open(i16 priority) ;
    virtual void Close(void) ;
    virtual i16 Main(struct tag_message& msg) ;
};
#pragma pack(pop)

extern i8 gSmackNum;
extern i8 gbSmackAborted;

extern "C" void* radmalloc(u32 numbytes);
extern "C" void radfree(void* ptr);
void PlaySmacker(i8 smackNumber);

#pragma pack(push, 1)
struct SSmackOptions {
    char fileName[15];
    i8 updateFlags;
    i8 fadeIn;
    i8 fadeOut;
    u32 openFlags;
};
#pragma pack(pop)

extern SSmackOptions SmackOptions[];

#endif

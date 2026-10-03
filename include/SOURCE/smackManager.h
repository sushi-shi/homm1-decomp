#ifndef HOMM1_SOURCE_SMACKMANAGER_H
#define HOMM1_SOURCE_SMACKMANAGER_H

#include <BASE/baseManager.h>
#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/smack.h>

struct tag_message;

// SmackOptions rows (retail 0x0049fd08), named by their movie files. Rows
// 0..1 are the publisher logos that draw "Presents...", 2..3 the intro and
// 4..7 the endings; oldmain and the end sequence pick one of each pair
// from gConfig.slowVideo.
H1_ENUM_BEGIN(SmackVideo)
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
H1_ENUM_END(SmackVideo)

// InitMainClasses allocates 0x32 bytes: the 0x30-byte baseManager prefix and
// one trailing word.
#pragma pack(push, 1)
class smackManager : public baseManager {
public:
    i16 m_unknown30;

    smackManager(void);
    virtual i16 Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(struct tag_message& msg) OVERRIDE;
};
#pragma pack(pop)

extern i8 gSmackNum;
extern i8 gbSmackAborted;

H1_C_LINKAGE void* radmalloc(u32 numbytes);
H1_C_LINKAGE void radfree(void* ptr);
void PlaySmacker(H1_ENUM_PARAM(SmackVideo, i8) smackNumber);

// One 0x16-byte row per movie: file name, the window manager's update mode
// while it plays, the fades around it and its SmackOpen flags (Buka
// SMACKMGR.h).
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

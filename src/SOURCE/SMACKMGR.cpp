// HoMM1 smackManager: an executive-driven manager, unlike HoMM2's
// SmackManagerMain loop. The TU begins after int3 alignment at 0x45abe0.

#include <match.h>

#include <BASE/executive.h>
#include <BASE/soundManager.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/smackManager.h>

#include <stdlib.h>
#include <string.h>

// RAD library allocation callbacks; HoMM1 links SMACKW32.DLL, so neither is
// reached.
// @dead-code
// Zero-ref: no effective incoming retail reference.
VA(0x0045abe0, 0x4c)
H1_C_LINKAGE void *radmalloc(unsigned long numbytes) {
    void *mem;
    if (numbytes == 0)
        return 0;
    if (numbytes != 0xffffffff)
        mem = malloc(numbytes);
    else
        mem = 0;
    return mem;
}

// @dead-code
// Zero-ref: no effective incoming retail reference.
VA(0x0045ac2c, 0x1c)
H1_C_LINKAGE void radfree(void *ptr) {
    free(ptr);
}

VA(0x0045ac48, 0x2a)
smackManager::smackManager(void) : baseManager() {
}

VA(0x0045ac72, 0x53)
short smackManager::Open(short priority) {
    gbSmackAborted = 0;
    m_messageMask = BASE_MANAGER_ACCEPT_EXECUTIVE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "smackManager");
    return 0;
}

VA(0x0045acc5, 0x1f)
void smackManager::Close(void) {
    m_active = 0;
}

VA(0x0045b659, 0x93)
void PlaySmacker(signed char smackNumber) {
    gbInSmacker = 1;
    gpSoundManager->m_musicReady = 1;
    gpSoundManager->PlayAmbientMusic(-1, 0, -1);
    bSmackNum = smackNumber;
    if (gpExec->AddManager(gpSmackManager, -1))
        ShutDown("Can't add manager!");
    gpExec->MainLoop();
    gpExec->RemoveManager(gpSmackManager);
    gbInSmacker = 0;
}

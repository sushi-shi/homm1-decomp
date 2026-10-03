#ifndef HOMM1_BASE_EXECUTIVE_H
#define HOMM1_BASE_EXECUTIVE_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 10 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>

// forward declarations:
class baseManager;

extern char gResourceManagerInitError[];
extern char gInputManagerInitError[];
extern char gSoundManagerInitError[];
extern char gMouseManagerInitError[];
extern char gWindowManagerInitError[];
extern char gCallManagerError1[];
extern char gCallManagerError2[];
extern char gDialogManagerError1[];
extern char gDialogManagerError2[];
extern char gDialogManagerError3[];
extern char gDialogManagerError4[];

H1_ENUM_CONST_BEGIN(ExecutiveConstant)
    EXECUTIVE_DIALOG_MANAGER_CAPACITY = 20
H1_ENUM_CONST_END(ExecutiveConstant)

class executive {
public:
    baseManager* m_managerListHead;
    baseManager* m_managerListTail;
    baseManager* m_activeManager;
    int m_result;
    // --- constructors ---
    executive(void);
    // --- methods ---
    short InitSystem(void);
    void ShutDownSystem(void);
    short DoDialog(class baseManager*);
    void PrintManagerList(void);
    short AddManager(class baseManager*, short);
    void RemoveManager(class baseManager*);
    void CallManager(class baseManager*);
    void MainLoop(void);
    void Terminate(void);
};
#endif // HOMM1_BASE_EXECUTIVE_H

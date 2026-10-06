#ifndef HOMM1_BASE_EXECUTIVE_H
#define HOMM1_BASE_EXECUTIVE_H

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

enum ExecutiveConstant {
    EXECUTIVE_DIALOG_MANAGER_CAPACITY = 20
};

class executive {
public:
    baseManager* m_managerListHead;
    baseManager* m_managerListTail;
    baseManager* m_activeManager;
    i32 m_result;
    executive(void);
    i16 InitSystem(void);
    void ShutDownSystem(void);
    i16 DoDialog(class baseManager* manager);
    void PrintManagerList(void);
    i16 AddManager(class baseManager* manager, i16 priority);
    void RemoveManager(class baseManager* manager);
    void CallManager(class baseManager* manager);
    void MainLoop(void);
    void Terminate(void);
};
#endif

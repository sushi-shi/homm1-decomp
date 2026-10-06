#ifndef HOMM1_BASE_EXECUTIVE_H
#define HOMM1_BASE_EXECUTIVE_H

#include <BASE/baseManager.h>

class baseManager;

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
    i16 AddManager(class baseManager* manager, i16 priority);
    void RemoveManager(class baseManager* manager);
    void CallManager(class baseManager* manager);
    void MainLoop(void);
    void Terminate(void);
};
#endif

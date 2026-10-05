// HoMM1 executive manager scheduling, reconstructed against the retail code.

#include <match.h>

#include <BASE/audio.h>
#include <BASE/baseManager.h>
#include <BASE/executive.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/Misc.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>

// Executive start-up and manager-list failure texts (retail .data 0x004a1820..).
DATA(0x004a15cc)
char gResourceManagerInitError[] = localization::Tr("startup.resources.failed");
DATA(0x004a1604)
char gInputManagerInitError[] =
    localization::Tr("startup.input.failed");
DATA(0x004a162c)
char gSoundManagerInitError[] = localization::Tr("startup.sound.failed");
DATA(0x004a1644)
char gMouseManagerInitError[] = localization::Tr("startup.mouse.failed");
DATA(0x004a165c)
char gWindowManagerInitError[] = localization::Tr("startup.windows.failed");
DATA(0x004a1680)
char gDialogManagerError1[] = localization::Tr("startup.manager.failed");
DATA(0x004a169c)
char gDialogManagerError2[] = localization::Tr("startup.manager.failed");
DATA(0x004a16b8)
char gDialogManagerError3[] = localization::Tr("startup.manager.failed");
DATA(0x004a16d4)
char gDialogManagerError4[] = localization::Tr("startup.manager.failed");
DATA(0x004a16f0)
char gCallManagerError1[] = localization::Tr("startup.manager.failed");
DATA(0x004a170c)
char gCallManagerError2[] = localization::Tr("startup.manager.failed");
DATA(0x004a1728)
char gTerminationMessage[] = "Terminated";

VA(0x004729f0, 0x35)
executive::executive(void) {
    m_managerListHead = NULL;
    m_managerListTail = NULL;
    m_activeManager = NULL;
    m_result = 0;
}

// Retail opens sound unconditionally and returns AX.
VA(0x00472a25, 0xbd)
i16 executive::InitSystem(void) {
    if (gpResourceManager->Open(BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(gResourceManagerInitError);
    if (gpInputManager->Open(BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(gInputManagerInitError);
    if (!InitAudio())
        ShutDown(gSoundManagerInitError);
    if (AddManager(gpMouseManager, BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(gMouseManagerInitError);
    if (AddManager(gpWindowManager, BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(gWindowManagerInitError);
    return BASE_MANAGER_SUCCESS;
}

// Retail preserves next before removing a manager, then closes resources/input.
VA(0x00472ae2, 0xb6)
void executive::ShutDownSystem(void) {
    EarlyShutDownSystem();
    ShutdownAudio();
    baseManager* next;
    baseManager* manager = m_managerListHead;
    while (manager != NULL) {
        next = manager->m_next;
        if (manager != gpWindowManager && manager != gpMouseManager)
            RemoveManager(manager);
        manager = next;
    }
    if (gpWindowManager->m_active == 1)
        RemoveManager(gpWindowManager);
    if (gpMouseManager->m_active == 1)
        RemoveManager(gpMouseManager);
    gpResourceManager->Close();
    gpInputManager->Close();
}

// Buka BASE/EXEC DoDialog; retail saves twenty manager links per array.
VA(0x00472b98, 0x189)
i16 executive::DoDialog(baseManager* manager) {
    baseManager* savePrev[EXECUTIVE_DIALOG_MANAGER_CAPACITY];
    i32 idx;
    baseManager* p;
    baseManager* saveMgr[EXECUTIVE_DIALOG_MANAGER_CAPACITY];
    baseManager* saveNext[EXECUTIVE_DIALOG_MANAGER_CAPACITY];
    executive ex;
    i32 count = 0;
    p = m_managerListHead;
    while (p != NULL) {
        saveMgr[count] = p;
        savePrev[count] = p->m_prev;
        saveNext[count] = p->m_next;
        p = p->m_next;
        count++;
    }
    if (AddManager(manager, BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(gDialogManagerError1);
    if (ex.AddManager(gpMouseManager, BASE_MANAGER_PRIORITY_UNASSIGNED)
        != BASE_MANAGER_SUCCESS)
        ShutDown(gDialogManagerError2);
    if (ex.AddManager(gpWindowManager, BASE_MANAGER_PRIORITY_UNASSIGNED)
        != BASE_MANAGER_SUCCESS)
        ShutDown(gDialogManagerError3);
    if (ex.AddManager(manager, BASE_MANAGER_PRIORITY_UNASSIGNED)
        != BASE_MANAGER_SUCCESS)
        ShutDown(gDialogManagerError4);
    ex.MainLoop();
    RemoveManager(manager);
    for (idx = 0; idx < count; idx++) {
        saveMgr[idx]->m_prev = savePrev[idx];
        saveMgr[idx]->m_next = saveNext[idx];
    }
    return ex.m_result;
}

VA(0x00472d21, 0x149)
i16 executive::AddManager(baseManager* manager, i16 priority) {
    if (manager == NULL)
        return BASE_MANAGER_ERROR;
    if (priority == BASE_MANAGER_PRIORITY_UNASSIGNED) {
        if (m_managerListTail == NULL)
            priority = 0;
        else
            priority = m_managerListTail->m_priority + 1;
    }
    if (!manager->m_active && manager->Open(priority) != BASE_MANAGER_SUCCESS)
        return BASE_MANAGER_ERROR;
    baseManager* current = m_managerListTail;
    while (current != NULL && current->m_priority > priority)
        current = current->m_prev;
    if (current == NULL) {
        manager->m_next = m_managerListHead;
        manager->m_prev = NULL;
        if (m_managerListHead != NULL)
            m_managerListHead->m_prev = manager;
        m_managerListHead = manager;
        if (m_managerListTail == NULL)
            m_managerListTail = manager;
    } else if (current->m_next == NULL) {
        manager->m_prev = m_managerListTail;
        manager->m_next = NULL;
        m_managerListTail->m_next = manager;
        m_managerListTail = manager;
    } else {
        manager->m_prev = current;
        manager->m_next = current->m_next;
        current->m_next->m_prev = manager;
        current->m_next = manager;
    }
    return BASE_MANAGER_SUCCESS;
}

VA(0x00472e6a, 0xc3)
void executive::RemoveManager(baseManager* manager) {
    if (manager == NULL)
        return;
    manager->Close();
    baseManager* previous = manager->m_prev;
    if (previous == NULL) {
        if (m_managerListHead == m_managerListTail) {
            m_managerListTail = NULL;
            m_managerListHead = NULL;
        } else {
            m_managerListHead = manager->m_next;
            m_managerListHead->m_prev = NULL;
        }
        manager->m_prev = NULL;
        manager->m_next = NULL;
        return;
    }
    previous->m_next = manager->m_next;
    if (previous->m_next == NULL)
        m_managerListTail = previous;
    else
        previous->m_next->m_prev = previous;
    manager->m_prev = NULL;
    manager->m_next = NULL;
}

VA(0x00472f2d, 0x88)
void executive::CallManager(baseManager* manager) {
    baseManager* saved = m_activeManager;
    RemoveManager(m_activeManager);
    if (AddManager(manager, BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(gCallManagerError1);
    MainLoop();
    RemoveManager(manager);
    if (AddManager(saved, BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(gCallManagerError2);
    m_activeManager = saved;
}

// Retail 0x47a5a0 event loop; Buka BASE/EXEC MainLoop correspondence.
VA(0x00472fb5, 0x16b)
void executive::MainLoop(void) {
    i8 done = 0;
    tag_message message;
    i32 unusedMode; // dead local: retail's /Od frame holds its unreferenced slot
    i8 dispatch = 1;
    if (m_managerListHead == NULL)
        return;
    gpInputManager->Flush();
    while (!done) {
        Process1WindowsMessage();
        message = gpInputManager->GetEvent();
        dispatch = 1;
        m_activeManager = m_managerListHead;
        if (m_activeManager == NULL)
            return;
        while (m_activeManager != NULL && dispatch && !done) {
            if (m_activeManager->m_active == 1) {
                switch (m_activeManager->Main(message)) {
                    case MESSAGE_DISPATCH_CONSUME:
                        dispatch = 0;
                        break;
                    case MESSAGE_DISPATCH_FORWARD:
                        if ((message.type & MESSAGE_EXECUTIVE) != 0) {
                            switch (message.executiveCommand) {
                                case EXECUTIVE_COMMAND_TERMINATE_LOOP:
                                    done++;
                                    break;
                                case EXECUTIVE_COMMAND_RETURN_RESULT:
                                    m_result = message.result;
                                    done++;
                                    break;
                                case EXECUTIVE_COMMAND_REMOVE_MANAGER:
                                    RemoveManager(m_activeManager);
                                    m_activeManager = NULL;
                                    break;
                            }
                        }
                        break;
                }
            }
            if (m_activeManager != NULL)
                m_activeManager = m_activeManager->m_next;
        }
    }
}

// HoMM2 Buka's Terminate has the same body.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00473120, 0x18)
void executive::Terminate(void) {
    ShutDown(gTerminationMessage);
}

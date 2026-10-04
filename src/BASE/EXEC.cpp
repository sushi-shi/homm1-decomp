// HoMM1 executive manager scheduling, reconstructed against the retail code.

#include <match.h>

#include <BASE/baseManager.h>
#include <BASE/executive.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/Misc.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <BASE/audio.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>

// Executive start-up and manager-list failure texts (retail .data 0x004a1820..).
DATA(0x004a3bb4)
char gResourceManagerInitError[] = "Unable to initialize resources - possible disk problem.";
DATA(0x004a3bec)
char gInputManagerInitError[] =
    "Unable to initialize input devices - possible problem with mouse or keyboard.";
DATA(0x004a3c3c)
char gSoundManagerInitError[] = "Unable to initialize sound.";
DATA(0x004a3c58)
char gMouseManagerInitError[] = "Unable to initialize mouse.";
DATA(0x004a3c74)
char gWindowManagerInitError[] = "Unable to initialize windows - possible memory or disk error.";
DATA(0x004a3cb4)
char gDialogManagerError1[] = "Can't add manager!";
DATA(0x004a3cc8)
char gDialogManagerError2[] = "Can't add manager!";
DATA(0x004a3cdc)
char gDialogManagerError3[] = "Can't add manager!";
DATA(0x004a3cf0)
char gDialogManagerError4[] = "Can't add manager!";
// Retail keeps the manager-list dump texts (PoL SExecutiveText names) between the
// dialog and call-manager errors; HoMM1 code no longer references them.
DATA(0x004a3d04)
char gManagerListStart[] = "-----Manager List Start-----";
DATA(0x004a3d24)
char gManagerListDivider1[] = "-----";
DATA(0x004a3d2c)
char gManagerListHeaderFormat[] = "Head %d   Tail %d";
DATA(0x004a3d40)
char gManagerListDivider2[] = "-----";
DATA(0x004a3d48)
char gManagerListEntryFormat[] = "Manager %20s  this %d   prev %d  next %d";
DATA(0x004a3d74)
char gManagerListStop[] = "--*--Manager List Stop --*--\n\n";
DATA(0x004a3d94)
char gCallManagerError1[] = "Can't add manager!";
DATA(0x004a3da8)
char gCallManagerError2[] = "Can't add manager!";

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
VA(0x00478130, 0x10c)
i16 executive::DoDialog(baseManager* manager) {
    baseManager* savedPreviousManagers[EXECUTIVE_DIALOG_MANAGER_CAPACITY];
    i32 index;
    baseManager* savedManagers[EXECUTIVE_DIALOG_MANAGER_CAPACITY];
    baseManager* savedNextManagers[EXECUTIVE_DIALOG_MANAGER_CAPACITY];
    baseManager* currentManager;
    executive dialogExecutive;
    i32 count = 0;
    currentManager = m_managerListHead;
    while (currentManager != NULL) {
        savedManagers[count] = currentManager;
        savedPreviousManagers[count] = currentManager->m_prev;
        savedNextManagers[count] = currentManager->m_next;
        currentManager = currentManager->m_next;
        count++;
    }
    if (AddManager(manager, BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(gDialogManagerError1);
    if (dialogExecutive.AddManager(gpMouseManager, BASE_MANAGER_PRIORITY_UNASSIGNED)
        != BASE_MANAGER_SUCCESS)
        ShutDown(gDialogManagerError2);
    if (dialogExecutive.AddManager(gpWindowManager, BASE_MANAGER_PRIORITY_UNASSIGNED)
        != BASE_MANAGER_SUCCESS)
        ShutDown(gDialogManagerError3);
    if (dialogExecutive.AddManager(manager, BASE_MANAGER_PRIORITY_UNASSIGNED)
        != BASE_MANAGER_SUCCESS)
        ShutDown(gDialogManagerError4);
    dialogExecutive.MainLoop();
    RemoveManager(manager);
    for (index = 0; index < count; index++) {
        savedManagers[index]->m_prev = savedPreviousManagers[index];
        savedManagers[index]->m_next = savedNextManagers[index];
    }
    return dialogExecutive.m_result;
}

VA(0x00478240, 0xd2)
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

VA(0x00478320, 0x76)
void executive::RemoveManager(baseManager* manager) {
    if (manager == NULL)
        return;
    manager->Close();
    baseManager* previous = manager->m_prev;
    if (previous == NULL) {
        if (m_managerListTail == m_managerListHead) {
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

VA(0x004783a0, 0x62)
void executive::CallManager(baseManager* manager) {
    baseManager* saved = m_activeManager;
    RemoveManager(saved);
    if (AddManager(manager, BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(gCallManagerError1);
    MainLoop();
    RemoveManager(manager);
    if (AddManager(saved, BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(gCallManagerError2);
    m_activeManager = saved;
}

// Retail 0x47a5a0 event loop; Buka BASE/EXEC MainLoop correspondence.
VA(0x00478410, 0x108)
void executive::MainLoop(void) {
    i8 done = 0;
    tag_message message;
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

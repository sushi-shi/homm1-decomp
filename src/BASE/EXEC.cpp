// HoMM1 executive manager scheduling, reconstructed against the retail code.

#include <match.h>

#include <BASE/baseManager.h>
#include <BASE/executive.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <BASE/soundManager.h>
#include <H1/KB.h>

VA(0x0047a170, 0x10)
executive::executive(void)
{
    m_managerListHead = NULL;
    m_managerListTail = NULL;
    m_activeManager = NULL;
    m_result = 0;
}

// Retail opens sound unconditionally and returns AX.
VA(0x0047a180, 0xa9)
short executive::InitSystem(void)
{
    if (gpResourceManager->Open(-1) != 0)
        ShutDown(gResourceManagerInitError);
    if (gpInputManager->Open(-1) != 0)
        ShutDown(gInputManagerInitError);
    if (gpSoundManager->Open(-1) != 0)
        ShutDown(gSoundManagerInitError);
    if (AddManager(gpMouseManager, -1) != 0)
        ShutDown(gMouseManagerInitError);
    if (AddManager(gpWindowManager, -1) != 0)
        ShutDown(gWindowManagerInitError);
    return 0;
}

// Retail preserves next before removing a manager, then closes resources/input.
VA(0x0047a230, 0x84)
void executive::ShutDownSystem(void)
{
    EarlyShutDownSystem();
    gpSoundManager->Close();
    baseManager *next;
    baseManager *manager = m_managerListHead;
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
VA(0x0047a2c0, 0x10c)
short executive::DoDialog(baseManager *manager)
{
    baseManager *savedPreviousManagers[EXECUTIVE_DIALOG_MANAGER_CAPACITY];
    int index;
    baseManager *currentManager;
    baseManager *savedManagers[EXECUTIVE_DIALOG_MANAGER_CAPACITY];
    baseManager *savedNextManagers[EXECUTIVE_DIALOG_MANAGER_CAPACITY];
    executive dialogExecutive;
    int count = 0;
    currentManager = m_managerListHead;
    while (currentManager != NULL) {
        savedManagers[count] = currentManager;
        savedPreviousManagers[count] = currentManager->m_prev;
        savedNextManagers[count] = currentManager->m_next;
        currentManager = currentManager->m_next;
        count++;
    }
    if (AddManager(manager, -1) != 0)
        ShutDown(gDialogManagerError1);
    if (dialogExecutive.AddManager(gpMouseManager, -1) != 0)
        ShutDown(gDialogManagerError2);
    if (dialogExecutive.AddManager(gpWindowManager, -1) != 0)
        ShutDown(gDialogManagerError3);
    if (dialogExecutive.AddManager(manager, -1) != 0)
        ShutDown(gDialogManagerError4);
    dialogExecutive.MainLoop();
    RemoveManager(manager);
    for (index = 0; index < count; index++) {
        savedManagers[index]->m_prev = savedPreviousManagers[index];
        savedManagers[index]->m_next = savedNextManagers[index];
    }
    return dialogExecutive.m_result;
}

VA(0x0047a3d0, 0xd2)
short executive::AddManager(baseManager *manager, short priority)
{
    if (manager == NULL)
        return 3;
    if (priority == -1) {
        if (m_managerListTail == NULL)
            priority = 0;
        else
            priority = m_managerListTail->m_priority + 1;
    }
    if (!manager->m_active && manager->Open(priority) != 0)
        return 3;
    baseManager *current = m_managerListTail;
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
    return 0;
}

VA(0x0047a4b0, 0x76)
void executive::RemoveManager(baseManager *manager)
{
    if (manager == 0)
        return;
    manager->Close();
    baseManager *previous = manager->m_prev;
    if (previous == 0) {
        if (m_managerListTail == m_managerListHead) {
            m_managerListTail = 0;
            m_managerListHead = 0;
        } else {
            m_managerListHead = manager->m_next;
            m_managerListHead->m_prev = 0;
        }
        manager->m_prev = 0;
        manager->m_next = 0;
        return;
    }
    previous->m_next = manager->m_next;
    if (previous->m_next == 0)
        m_managerListTail = previous;
    else
        previous->m_next->m_prev = previous;
    manager->m_prev = 0;
    manager->m_next = 0;
}

VA(0x0047a530, 0x62)
void executive::CallManager(baseManager *manager)
{
    baseManager *saved = m_activeManager;
    RemoveManager(saved);
    if (AddManager(manager, -1) != 0)
        ShutDown(gCallManagerError1);
    MainLoop();
    RemoveManager(manager);
    if (AddManager(saved, -1) != 0)
        ShutDown(gCallManagerError2);
    m_activeManager = saved;
}

// Retail 0x47a5a0 event loop; Buka BASE/EXEC MainLoop correspondence.
VA(0x0047a5a0, 0x108)
void executive::MainLoop(void)
{
    signed char done = 0;
    tag_message message;
    signed char dispatch = 1;
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

// Executive start-up and manager-list failure texts (retail .data 0x004a1820..).
DATA(0x004a1820) char gResourceManagerInitError[] = "Unable to initialize resources - possible disk problem.";
DATA(0x004a1858) char gInputManagerInitError[] =
    "Unable to initialize input devices - possible problem with mouse or keyboard.";
DATA(0x004a18a8) char gSoundManagerInitError[] = "Unable to initialize sound.";
DATA(0x004a18c4) char gMouseManagerInitError[] = "Unable to initialize mouse.";
DATA(0x004a18e0) char gWindowManagerInitError[] =
    "Unable to initialize windows - possible memory or disk error.";
DATA(0x004a1920) char gDialogManagerError1[] = "Can't add manager!";
DATA(0x004a1934) char gDialogManagerError2[] = "Can't add manager!";
DATA(0x004a1948) char gDialogManagerError3[] = "Can't add manager!";
DATA(0x004a195c) char gDialogManagerError4[] = "Can't add manager!";
DATA(0x004a1a00) char gCallManagerError1[] = "Can't add manager!";
DATA(0x004a1a14) char gCallManagerError2[] = "Can't add manager!";

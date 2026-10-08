#include <H1/Ints.h>

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

executive::executive(void) {
    m_managerListHead = NULL;
    m_managerListTail = NULL;
    m_activeManager = NULL;
    m_result = 0;
}

i16 executive::InitSystem(void) {
    if (gResourceManager->Open(BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(localization::Tr("startup.resources.failed"));
    if (gInputManager->Open(BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(localization::Tr("startup.input.failed"));
    if (!InitAudio())
        ShutDown(localization::Tr("startup.sound.failed"));
    if (AddManager(gMouseManager, BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(localization::Tr("startup.mouse.failed"));
    if (AddManager(gWindowManager, BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(localization::Tr("startup.windows.failed"));
    return BASE_MANAGER_SUCCESS;
}

void executive::ShutDownSystem(void) {
    EarlyShutDownSystem();
    ShutdownAudio();
    baseManager* next;
    baseManager* manager = m_managerListHead;
    while (manager != NULL) {
        next = manager->m_next;
        if (manager != gWindowManager && manager != gMouseManager)
            RemoveManager(manager);
        manager = next;
    }
    if (gWindowManager->m_active == 1)
        RemoveManager(gWindowManager);
    if (gMouseManager->m_active == 1)
        RemoveManager(gMouseManager);
    gResourceManager->Close();
    gInputManager->Close();
}

i16 executive::DoDialog(baseManager* manager) {
    baseManager* savedPrevLinks[EXECUTIVE_DIALOG_MANAGER_CAPACITY];
    i32 index;
    baseManager* node;
    baseManager* savedManagers[EXECUTIVE_DIALOG_MANAGER_CAPACITY];
    baseManager* savedNextLinks[EXECUTIVE_DIALOG_MANAGER_CAPACITY];
    executive nestedExecutive;
    i32 count = 0;
    node = m_managerListHead;
    while (node != NULL) {
        savedManagers[count] = node;
        savedPrevLinks[count] = node->m_prev;
        savedNextLinks[count] = node->m_next;
        node = node->m_next;
        count++;
    }
    if (AddManager(manager, BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(localization::Tr("startup.manager.failed"));
    if (nestedExecutive.AddManager(gMouseManager, BASE_MANAGER_PRIORITY_UNASSIGNED)
        != BASE_MANAGER_SUCCESS)
        ShutDown(localization::Tr("startup.manager.failed"));
    if (nestedExecutive.AddManager(gWindowManager, BASE_MANAGER_PRIORITY_UNASSIGNED)
        != BASE_MANAGER_SUCCESS)
        ShutDown(localization::Tr("startup.manager.failed"));
    if (nestedExecutive.AddManager(manager, BASE_MANAGER_PRIORITY_UNASSIGNED)
        != BASE_MANAGER_SUCCESS)
        ShutDown(localization::Tr("startup.manager.failed"));
    nestedExecutive.MainLoop();
    RemoveManager(manager);
    for (index = 0; index < count; index++) {
        savedManagers[index]->m_prev = savedPrevLinks[index];
        savedManagers[index]->m_next = savedNextLinks[index];
    }
    return nestedExecutive.m_result;
}

i16 executive::AddManager(baseManager* manager, i16 priority) {
    if (manager == NULL)
        return BASE_MANAGER_ERROR;
    if (priority == BASE_MANAGER_PRIORITY_UNASSIGNED) {
        if (m_managerListTail == NULL)
            priority = BASE_MANAGER_PRIORITY_FIRST;
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

void executive::CallManager(baseManager* manager) {
    baseManager* saved = m_activeManager;
    RemoveManager(m_activeManager);
    if (AddManager(manager, BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(localization::Tr("startup.manager.failed"));
    MainLoop();
    RemoveManager(manager);
    if (AddManager(saved, BASE_MANAGER_PRIORITY_UNASSIGNED) != BASE_MANAGER_SUCCESS)
        ShutDown(localization::Tr("startup.manager.failed"));
    m_activeManager = saved;
}

void executive::MainLoop(void) {
    i8 done = 0;
    tag_message message;
    i32 unusedMode;
    b8 dispatch = true;
    if (m_managerListHead == NULL)
        return;
    gInputManager->Flush();
    while (!done) {
        Process1WindowsMessage();
        message = gInputManager->GetEvent();
        dispatch = true;
        m_activeManager = m_managerListHead;
        if (m_activeManager == NULL)
            return;
        while (m_activeManager != NULL && dispatch && !done) {
            if (m_activeManager->m_active == 1) {
                switch (m_activeManager->Main(message)) {
                    case MESSAGE_DISPATCH_CONSUME:
                        dispatch = false;
                        break;
                    case MESSAGE_DISPATCH_FORWARD:
                        if ((message.type & MESSAGE_EXECUTIVE) != MESSAGE_NONE) {
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

void executive::Terminate(void) {
    ShutDown("Terminated");
}

#include <match.h>

#include <BASE/baseManager.h>
#include <BASE/bitmap.h>
#include <BASE/font.h>
#include <BASE/icon.h>
#include <BASE/MAKEFILEID.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/palette.h>
#include <BASE/resource.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <BASE/tileset.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Retail assertion paths: each program's BASE objects were compiled in its own
// checkout (HEROES.EXE and EDITOR.EXE assertion strings).
#ifdef HOMM1_EDITOR
#define RESMGR_CPP_PATH "U:\\HMM\\VSS\\HMM1\\Source\\Base\\RESMGR.CPP"
#else
#define RESMGR_CPP_PATH "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Base\\RESMGR.CPP"
#endif

// The manager owns a single aggregate descriptor.
VA(0x0046c0e0, 0x7a)
resourceManager::resourceManager(void) : baseManager() {
    m_active = 0;
    m_resourceListHead = NULL;
    m_aggregateDir = NULL;
    m_aggregateFd = FILE_DESCRIPTOR_INVALID;
    m_aggregateEntryCount = 0;
    m_expunging = false;
    strcpy(m_lastFileName, "");
    m_lastFileId = 0;
}

VA(0x0046c15a, 0x7a)
void resourceManager::GetBackdrop(char* name, class bitmap* backdrop) {
    PointToFile(MakeId(name));
    ReadWord();
    ReadWord();
    ReadWord();
    ReadBlock(backdrop->m_pixels, backdrop->m_width * backdrop->m_height);
    PostprocessBitmap(backdrop->m_pixels, backdrop->m_width, backdrop->m_height);
}

VA(0x0046c1d4, 0x87)
void resourceManager::GetBackdropAtLoc(
    char* filename,
    class bitmap* destination,
    i32 destinationX,
    i32 destinationY
) {
    i32 curRow;
    i16 imageHeight;
    i16 width;
    PointToFile(MakeId(filename));
    ReadWord();
    width = ReadWord();
    imageHeight = ReadWord();
    for (curRow = destinationY; curRow < destinationY + imageHeight; curRow++) {
        ReadBlock(destination->m_pixels + curRow * LOGICAL_SCREEN_WIDTH + destinationX, width);
    }
}

VA(0x0046c25b, 0xc0)
palette* resourceManager::GetPalette(char* name) {
    i16 id = MakeId(name);
    resource* resourceEntry = Query(id);
    if (resourceEntry != NULL) {
        resourceEntry->m_refCount++;
        return static_cast<palette*>(resourceEntry);
    } else {
        resourceEntry = new palette(id);
        AddResource(resourceEntry);
        return static_cast<palette*>(resourceEntry);
    }
}

VA(0x0046c31b, 0xc0)
bitmap* resourceManager::GetBitmap(char* name) {
    i16 id = MakeId(name);
    resource* resourceEntry = Query(id);
    if (resourceEntry != NULL) {
        resourceEntry->m_refCount++;
        return static_cast<bitmap*>(resourceEntry);
    } else {
        resourceEntry = new bitmap(id);
        AddResource(resourceEntry);
        return static_cast<bitmap*>(resourceEntry);
    }
}

// Forwards the name's 16-bit ID to the ID overload below.
VA(0x0046c3db, 0x2c)
icon* resourceManager::GetIcon(char* name) {
    i16 fileId = MakeId(name);
    return GetIcon(fileId);
}

// Same cache/refcount path as the neighboring palette and tileset getters.
VA(0x0046c407, 0xb0)
icon* resourceManager::GetIcon(i16 fileId) {
    icon* iconEntry = static_cast<icon*>(Query(fileId));
    if (iconEntry != NULL) {
        iconEntry->m_refCount++;
        return iconEntry;
    } else {
        iconEntry = new icon(fileId);
        AddResource(iconEntry);
        return iconEntry;
    }
}

VA(0x0046c4b7, 0xc0)
tileset* resourceManager::GetTileset(char* name) {
    i16 id = MakeId(name);
    resource* resourceEntry = Query(id);
    if (resourceEntry != NULL) {
        resourceEntry->m_refCount++;
        return static_cast<tileset*>(resourceEntry);
    } else {
        resourceEntry = new tileset(id);
        AddResource(resourceEntry);
        return static_cast<tileset*>(resourceEntry);
    }
}

// The Windows build loads no mouse resources.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0046c577, 0xf)
mouse* resourceManager::GetMouse(char* name) {
    return NULL;
}

VA(0x0046c586, 0xc0)
font* resourceManager::GetFont(char* name) {
    i16 resourceId = MakeId(name);
    resource* fontEntry = Query(resourceId);
    if (fontEntry != NULL) {
        fontEntry->m_refCount++;
        return static_cast<font*>(fontEntry);
    } else {
        fontEntry = new font(resourceId);
        AddResource(fontEntry);
        return static_cast<font*>(fontEntry);
    }
}

// Keeps the cached resource references and uses the filename-only loader.
VA(0x0046c646, 0xbf)
class sample* resourceManager::GetSample(char* name) {
    i16 fileId = MakeId(name);
    resource* resourceEntry = Query(fileId);
    if (resourceEntry != NULL) {
        resourceEntry->m_refCount++;
        return static_cast<sample*>(resourceEntry);
    } else {
        resourceEntry = new sample(name);
        AddResource(resourceEntry);
        return static_cast<sample*>(resourceEntry);
    }
}

VA(0x0046c705, 0x75)
void resourceManager::Dispose(class resource* resourceToDispose) {
    if (m_expunging != false)
        return;
    if (resourceToDispose != NULL) {
        resourceToDispose->m_refCount--;
        if (resourceToDispose->m_refCount > RESOURCE_REFERENCE_EMPTY) {
            return;
        } else {
            RemoveResource(resourceToDispose);
            delete resourceToDispose;
        }
    }
}

VA(0x0046c77a, 0x43)
void resourceManager::AddResource(class resource* newResource) {
    if (m_resourceListHead == NULL) {
        m_resourceListHead = newResource;
        m_resourceListHead->m_next = NULL;
    } else {
        newResource->m_next = m_resourceListHead;
        m_resourceListHead = newResource;
    }
}

#define current cur // frame-slot spelling
VA(0x0046c7bd, 0x7e)
void resourceManager::Expunge(void) {
    m_expunging = true;
    resource* current = m_resourceListHead;
    resource* next = NULL;
    while (current != NULL) {
        next = current->m_next;
        RemoveResource(current);
        delete current;
        current = next;
    }
    m_expunging = false;
}
#undef current

VA(0x0046c83b, 0x3b)
class resource* resourceManager::Query(i16 resourceId) {
    resource* cursorResource = m_resourceListHead;
    while (cursorResource != NULL && cursorResource->m_id != resourceId)
        cursorResource = cursorResource->m_next;
    return cursorResource;
}

VA(0x0046c876, 0x10)
H1_ENUM_RETURN(MessageDispatchResult, i16) resourceManager::Main(tag_message& message) {
    return MESSAGE_DISPATCH_CONTINUE;
}

// Loads only the default aggregate.
VA(0x0046c886, 0x66)
H1_ENUM_RETURN(BaseManagerStatus, i16) resourceManager::Open(i16 priority) {
    if (LoadAggregateHeader(gDefaultAggregateName) != RESOURCE_MANAGER_LOAD_SUCCESS)
        return BASE_MANAGER_ERROR;
    m_messageMask = BASE_MANAGER_ACCEPT_RESOURCE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "resourceManager");
    m_resourceListHead = NULL;
    return BASE_MANAGER_SUCCESS;
}

VA(0x0046c8ec, 0x61)
void resourceManager::RemoveResource(class resource* resourceToRemove) {
    if (m_resourceListHead == resourceToRemove) {
        m_resourceListHead = resourceToRemove->m_next;
        return;
    }
    resource* previousResource = m_resourceListHead;
    while (previousResource != NULL && previousResource->m_next != resourceToRemove)
        previousResource = previousResource->m_next;
    if (previousResource == NULL) {
        return;
    } else {
        previousResource->m_next = resourceToRemove->m_next;
    }
}

// Closes the single aggregate.
VA(0x0046c94d, 0x6e)
void resourceManager::Close(void) {
    if (m_active != 1)
        return;
    Expunge();
    m_resourceListHead = NULL;
    if (m_aggregateDir != NULL)
        free(m_aggregateDir);
    if (m_aggregateFd != FILE_DESCRIPTOR_INVALID) {
        close(m_aggregateFd);
        m_aggregateFd = FILE_DESCRIPTOR_INVALID;
    }
    m_active = 0;
}

// Replaces the single packed aggregate directory.
VA(0x0046c9bb, 0xe5)
i16 resourceManager::LoadAggregateHeader(char* aggregateName) {
    i16 directoryBytes;
    i32 aggregateFd = open(aggregateName, RESOURCE_MANAGER_BINARY_OPEN_MODE);
    if (aggregateFd == FILE_DESCRIPTOR_INVALID) {
        sprintf(gText, localization::Tr("file.aggregate.open_failed"), aggregateName);
        ShutDown(gText);
        return RESOURCE_MANAGER_LOAD_ERROR;
    }
    if (m_aggregateFd != FILE_DESCRIPTOR_INVALID)
        close(m_aggregateFd);
    if (m_aggregateDir != NULL)
        free(m_aggregateDir);
    m_aggregateFd = aggregateFd;
    read(m_aggregateFd, &m_aggregateEntryCount, sizeof(m_aggregateEntryCount));
    directoryBytes = m_aggregateEntryCount * sizeof(aggEntry);
    m_aggregateDir = static_cast<aggEntry*>(malloc(directoryBytes));
    read(m_aggregateFd, m_aggregateDir, directoryBytes);
    return RESOURCE_MANAGER_LOAD_SUCCESS;
}

// One packed directory, indexed by a signed 16-bit resource ID.
VA(0x0046caa0, 0xd2)
void resourceManager::PointToFile(i16 fileId) {
    i16 entry;
    if (m_aggregateDir == NULL)
        ShutDown("File Error: .AGG File not valid");
    entry = 0;
    while (entry < m_aggregateEntryCount && m_aggregateDir[entry].id != fileId)
        entry++;
    if (m_aggregateDir[entry].id != fileId) {
        sprintf(
            gText,
            "ResMgr::PointToFile failure!  ThisFileId:%d  LastFileId:%d  LastFileName:%s",
            fileId,
            m_lastFileId,
            m_lastFileName
        );
        ShutDown(gText);
    }
    lseek(m_aggregateFd, m_aggregateDir[entry].offset, SEEK_SET);
}

// Single-aggregate directory lookup.
VA(0x0046cb72, 0xc1)
u32 resourceManager::GetFileSize(i16 fileId) {
    if (m_aggregateDir == NULL)
        return 0;
    i16 entry = 0;
    while (entry < m_aggregateEntryCount && m_aggregateDir[entry].id != fileId)
        entry++;
    if (m_aggregateDir[entry].id != fileId) {
        sprintf(
            gText,
            "ResMgr::PointToFile(GetFileSize) failure!  ThisFileId:%d  LastFileId:%d  "
            "LastFileName:%s",
            fileId,
            m_lastFileId,
            m_lastFileName
        );
        ShutDown(gText);
        return RESOURCE_MANAGER_LOAD_ERROR;
    }
    return m_aggregateDir[entry].size;
}

VA(0x0046cc33, 0x20)
void resourceManager::SavePosition(void) {
    m_savedPosition = tell(m_aggregateFd);
}

VA(0x0046cc53, 0x23)
void resourceManager::RestorePosition(void) {
    lseek(m_aggregateFd, m_savedPosition, SEEK_SET);
}

VA(0x0046cc76, 0x48)
#line 598 RESMGR_CPP_PATH
i8 resourceManager::ReadByte(void) {
#line 599
    H1_ASSERT(m_aggregateFd != FILE_DESCRIPTOR_INVALID);
    i8 value = 0;
    read(m_aggregateFd, &value, sizeof(value));
    return value;
}

VA(0x0046ccbe, 0x4b)
#line 619 RESMGR_CPP_PATH
i16 resourceManager::ReadWord(void) {
#line 620
    H1_ASSERT(m_aggregateFd != FILE_DESCRIPTOR_INVALID);
    i16 value = 0;
    read(m_aggregateFd, &value, sizeof(value));
    return value;
}

VA(0x0046cd09, 0x4b)
#line 639 RESMGR_CPP_PATH
i32 resourceManager::ReadLong(void) {
#line 640
    H1_ASSERT(m_aggregateFd != FILE_DESCRIPTOR_INVALID);
    i32 value = 0;
    read(m_aggregateFd, &value, sizeof(value));
    return value;
}

VA(0x0046cd54, 0x41)
i16 resourceManager::MakeId(char* name) {
    u32 result = MAKEFILEID(name);
    strcpy(m_lastFileName, name);
    m_lastFileId = result;
    return result;
}

VA(0x0046cd95, 0x1b)
void resourceManager::Read13(char* destination) {
    ReadBlock(destination, RESOURCE_NAME_CAPACITY);
}

VA(0x0046cdb0, 0x52)
#line 679 RESMGR_CPP_PATH
void resourceManager::ReadBlock(void* destination, u32 size) {
#line 680
    H1_ASSERT(m_aggregateFd != FILE_DESCRIPTOR_INVALID);
    PollSound();
    i32 bytesRead = read(m_aggregateFd, destination, size);
    PollSound();
}

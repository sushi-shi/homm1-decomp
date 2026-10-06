#include <H1/Ints.h>

#include <BASE/baseManager.h>
#include <BASE/bitmap.h>
#include <BASE/font.h>
#include <BASE/icon.h>
#include <BASE/MAKEFILEID.h>
#include <BASE/MIDIWrap.h>
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

resourceManager::resourceManager(void) : baseManager() {
    m_active = 0;
    m_resourceListHead = NULL;
    m_aggregateDir = NULL;
    m_aggregateFd = RESOURCE_MANAGER_INVALID_FILE;
    m_aggregateEntryCount = 0;
    m_expunging = 0;
    strcpy(m_lastFileName, "");
    m_lastFileId = 0;
}

void resourceManager::GetBackdrop(char* name, class bitmap* backdrop) {
    PointToFile(MakeId(name));
    ReadWord();
    ReadWord();
    ReadWord();
    ReadBlock(backdrop->m_pixels, backdrop->m_width * backdrop->m_height);
    PostprocessBitmap(backdrop->m_pixels, backdrop->m_width, backdrop->m_height);
}

void resourceManager::GetBackdropAtLoc(
    char* filename,
    class bitmap* destination,
    i32 destinationX,
    i32 destinationY
) {
    i32 curRow;
    {
        i16 imageHeight;
        {
            i16 width;
            PointToFile(MakeId(filename));
            ReadWord();
            width = ReadWord();
            imageHeight = ReadWord();
            for (curRow = destinationY; curRow < destinationY + imageHeight; curRow++) {
                ReadBlock(
                    destination->m_pixels + curRow * RESOURCE_MANAGER_BACKDROP_ROW_BYTES
                        + destinationX,
                    width
                );
            }
        }
    }
}

palette* resourceManager::GetPalette(char* name) {
    i16 fileId = MakeId(name);
    resource* resourceEntry = Query(fileId);
    if (resourceEntry != NULL) {
        resourceEntry->m_refCount++;
        return static_cast<palette*>(resourceEntry);
    } else {
        resourceEntry = new palette(fileId);
        AddResource(resourceEntry);
        return static_cast<palette*>(resourceEntry);
    }
}

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

icon* resourceManager::GetIcon(char* name) {
    i16 fileId = MakeId(name);
    return GetIcon(fileId);
}

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

tileset* resourceManager::GetTileset(char* name) {
    i16 fileId = MakeId(name);
    resource* resourceEntry = Query(fileId);
    if (resourceEntry != NULL) {
        resourceEntry->m_refCount++;
        return static_cast<tileset*>(resourceEntry);
    } else {
        resourceEntry = new tileset(fileId);
        AddResource(resourceEntry);
        return static_cast<tileset*>(resourceEntry);
    }
}

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

class sample* resourceManager::GetSample(char* name) {
    i16 fileId = MakeId(name);
    resource* resourceEntry = Query(fileId);
    if (resourceEntry != NULL) {
        resourceEntry->m_refCount++;
        return static_cast<sample*>(resourceEntry);
    } else {
        resourceEntry =
            new sample(name, SAMPLE_PLAYBACK_CHANNEL_MUSIC, SAMPLE_VOLUME_FULL, SAMPLE_LOOP_ONCE);
        AddResource(resourceEntry);
        return static_cast<sample*>(resourceEntry);
    }
}

void resourceManager::Dispose(class resource* resourceToDispose) {
    if (m_expunging != 0)
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

void resourceManager::AddResource(class resource* newResource) {
    if (m_resourceListHead == NULL) {
        m_resourceListHead = newResource;
        m_resourceListHead->m_next = NULL;
    } else {
        newResource->m_next = m_resourceListHead;
        m_resourceListHead = newResource;
    }
}

void resourceManager::Expunge(void) {
    m_expunging = 1;
    resource* cursor[2];
    cursor[1] = m_resourceListHead;
    cursor[0] = NULL;
    while (cursor[1] != NULL) {
        cursor[0] = cursor[1]->m_next;
        RemoveResource(cursor[1]);
        delete cursor[1];
        cursor[1] = cursor[0];
    }
    m_expunging = 0;
}

class resource* resourceManager::Query(i16 resourceId) {
    resource* cursorResource = m_resourceListHead;
    while (cursorResource != NULL && cursorResource->m_id != resourceId)
        cursorResource = cursorResource->m_next;
    return cursorResource;
}

i16 resourceManager::Main(tag_message&) {
    return 0;
}

i16 resourceManager::Open(i16 priority) {
    if (LoadAggregateHeader(DEFAULT_AGGREGATE_NAME) != 0)
        return RESOURCE_MANAGER_LOAD_ERROR;
    m_messageMask = BASE_MANAGER_ACCEPT_RESOURCE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "resourceManager");
    m_resourceListHead = NULL;
    return BASE_MANAGER_SUCCESS;
}

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

void resourceManager::Close(void) {
    if (m_active != 1)
        return;
    Expunge();
    m_resourceListHead = NULL;
    if (m_aggregateDir != NULL)
        free(m_aggregateDir);
    if (m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE) {
        _close(m_aggregateFd);
        m_aggregateFd = RESOURCE_MANAGER_INVALID_FILE;
    }
    m_active = 0;
}

i16 resourceManager::LoadAggregateHeader(char* aggregateName) {
    i16 directoryBytes;
    i32 aggregateFp = _open(aggregateName, RESOURCE_MANAGER_BINARY_OPEN_MODE);
    if (aggregateFp == RESOURCE_MANAGER_INVALID_FILE) {
        sprintf(gText, "Can't open file: %s", aggregateName);
        ShutDown(gText);
        return RESOURCE_MANAGER_LOAD_ERROR;
    }
    if (m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE)
        _close(m_aggregateFd);
    if (m_aggregateDir != NULL)
        free(m_aggregateDir);
    m_aggregateFd = aggregateFp;
    _read(m_aggregateFd, &m_aggregateEntryCount, sizeof(m_aggregateEntryCount));
    directoryBytes = m_aggregateEntryCount * sizeof(aggEntry);
    m_aggregateDir = static_cast<aggEntry*>(malloc(directoryBytes));
    _read(m_aggregateFd, m_aggregateDir, directoryBytes);
    return 0;
}

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
    _lseek(m_aggregateFd, m_aggregateDir[entry].offset, SEEK_SET);
}

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

void resourceManager::SavePosition(void) {
    m_savedPosition = tell(m_aggregateFd);
}

void resourceManager::RestorePosition(void) {
    _lseek(m_aggregateFd, m_savedPosition, SEEK_SET);
}

i8 resourceManager::ReadByte(void) {
    H1_ASSERT(m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE);
    i8 value = 0;
    _read(m_aggregateFd, &value, sizeof(value));
    return value;
}

i16 resourceManager::ReadWord(void) {
    H1_ASSERT(m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE);
    i16 value = 0;
    _read(m_aggregateFd, &value, sizeof(value));
    return value;
}

i32 resourceManager::ReadLong(void) {
    H1_ASSERT(m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE);
    i32 value = 0;
    _read(m_aggregateFd, &value, sizeof(value));
    return value;
}

i16 resourceManager::MakeId(char* name) {
    u32 result = MAKEFILEID(name);
    strcpy(m_lastFileName, name);
    m_lastFileId = result;
    return result;
}

void resourceManager::Read13(i8* destination) {
    ReadBlock(destination, RESOURCE_NAME_CAPACITY);
}

void resourceManager::ReadBlock(i8* destination, u32 size) {
    H1_ASSERT(m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE);
    PollSound();
    i32 bytesRead = _read(m_aggregateFd, destination, size);
    PollSound();
}

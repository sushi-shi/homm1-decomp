// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

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

// HoMM1 owns one aggregate descriptor rather than Buka's descriptor array.
VA(0x0046c0e0, 0x7a)
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

// HoMM1 has only the raw-backdrop path of the Buka donor overload.
VA(0x0046c15a, 0x7a)
void resourceManager::GetBackdrop(char* name, class bitmap* backdrop) {
    PointToFile(MakeId(name));
    ReadWord();
    ReadWord();
    ReadWord();
    ReadBlock(backdrop->m_pixels, backdrop->m_width * backdrop->m_height);
    PostprocessBitmap(backdrop->m_pixels, backdrop->m_width, backdrop->m_height);
}

// HoMM1 likewise omits Buka's useIcon branch and keeps its row-copy loop.
VA(0x0046c1d4, 0x87)
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

// The resource cache and its miss path follow Buka 2.1 RESMGR. Retail's
// 16-bit MakeId/Query pair and the derived constructors identify each member.
VA(0x0046c25b, 0xc0)
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

// Retail forwards the 16-bit name ID to the cache overload below.
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

// Buka keeps the cached resource references and uses the filename-only loader.
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

// donor PoL RVA 0x000c86b0; preferred Buka symbol ?Dispose@resourceManager@@QAEXPAVresource@@@Z
// donor Buka TU BASE/RESMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.520865;margin=0.403030;shape=0.400;size=0.812;calls=1.000;alternate=pol20:void resourceManager::Dispose(class resource *)@0x000c86b0
VA(0x0046c705, 0x75)
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

// donor PoL RVA 0x000c8740; preferred Buka symbol ?AddResource@resourceManager@@QAEXPAVresource@@@Z
// donor Buka TU BASE/RESMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.451638;margin=0.545401;shape=0.346;size=0.698;calls=1.000;alternate=pol20:void resourceManager::AddResource(class resource *)@0x000c8740
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

// donor Buka RVA 0x000b8740; PoL 2.0 has the same list walk and deletion order
VA(0x0046c7bd, 0x7e)
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

// donor PoL RVA 0x000c8830; preferred Buka symbol ?Query@resourceManager@@QAEPAVresource@@K@Z
// donor Buka TU BASE/RESMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.434784;margin=0.589664;shape=0.276;size=0.688;calls=1.000;alternate=pol20:class resource * resourceManager::Query(unsigned long int)@0x000c8830
VA(0x0046c83b, 0x3b)
class resource* resourceManager::Query(i16 resourceId) {
    resource* cursorResource = m_resourceListHead;
    while (cursorResource != NULL && cursorResource->m_id != resourceId)
        cursorResource = cursorResource->m_next;
    return cursorResource;
}

// donor Buka RVA 0x000b8800; HoMM1 returns its dispatch result through AX
VA(0x0046c876, 0x10)
i16 resourceManager::Main(tag_message&) {
    return 0;
}

// donor Buka RVA 0x000b8810; HoMM1 loads only the default aggregate
VA(0x0046c886, 0x66)
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

// donor Buka RVA 0x000b8890; PoL 2.0 is source-identical
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

// HoMM1 has one aggregate, while Buka's later Close loops over several.
VA(0x0046c94d, 0x6e)
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

// donor Buka RVA 0x000b89b0; HoMM1 replaces one packed aggregate directory
VA(0x0046c9bb, 0xe5)
i16 resourceManager::LoadAggregateHeader(char* aggregateName) {
    i16 directoryBytes;
    i32 aggregateFp = _open(aggregateName, RESOURCE_MANAGER_BINARY_OPEN_MODE);
    if (aggregateFp == RESOURCE_MANAGER_INVALID_FILE) {
        sprintf(gText, localization::Tr("file.aggregate.open_failed"), aggregateName);
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

// donor Buka uses the same lookup and failure path across multiple aggregates;
// HoMM1 has one packed directory and a signed 16-bit resource ID.
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
    _lseek(m_aggregateFd, m_aggregateDir[entry].offset, SEEK_SET);
}

// Single-aggregate variant of the Buka 2.1 directory lookup.
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

// donor PoL RVA 0x000c8e20; preferred Buka symbol ?SavePosition@resourceManager@@QAEXXZ
// donor Buka TU BASE/RESMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.435067;margin=0.395891;shape=0.259;size=0.600;calls=1.000;alternate=pol20:void resourceManager::SavePosition(void)@0x000c8e20
VA(0x0046cc33, 0x20)
void resourceManager::SavePosition(void) {
    m_savedPosition = tell(m_aggregateFd);
}

// donor PoL RVA 0x000c8e80; preferred Buka symbol ?RestorePosition@resourceManager@@QAEXXZ
// donor Buka TU BASE/RESMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.425713;margin=0.238867;shape=0.231;size=0.593;calls=1.000;alternate=pol20:void resourceManager::RestorePosition(void)@0x000c8e80
VA(0x0046cc53, 0x23)
void resourceManager::RestorePosition(void) {
    _lseek(m_aggregateFd, m_savedPosition, SEEK_SET);
}

// donor Buka RVA 0x000b8d80; HoMM1 uses its single aggregate descriptor
VA(0x0046cc76, 0x48)
#line 598 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Base\\RESMGR.CPP"
i8 resourceManager::ReadByte(void) {
#line 599
    H1_ASSERT(m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE);
    i8 value = 0;
    _read(m_aggregateFd, &value, sizeof(value));
    return value;
}

// donor PoL RVA 0x000c8f70; preferred Buka symbol ?ReadWord@resourceManager@@QAEFXZ
// donor Buka TU BASE/RESMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.481320;margin=0.600000;shape=0.261;size=0.958;calls=1.000;alternate=pol20:short int resourceManager::ReadWord(void)@0x000c8f70
VA(0x0046ccbe, 0x4b)
#line 619 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Base\\RESMGR.CPP"
i16 resourceManager::ReadWord(void) {
#line 620
    H1_ASSERT(m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE);
    i16 value = 0;
    _read(m_aggregateFd, &value, sizeof(value));
    return value;
}

// donor Buka RVA 0x000b8e40; HoMM1 uses its single aggregate descriptor
VA(0x0046cd09, 0x4b)
#line 639 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Base\\RESMGR.CPP"
i32 resourceManager::ReadLong(void) {
#line 640
    H1_ASSERT(m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE);
    i32 value = 0;
    _read(m_aggregateFd, &value, sizeof(value));
    return value;
}

// donor Buka RVA 0x000b8ea0; HoMM1 has no translation argument and uses 16-bit IDs
VA(0x0046cd54, 0x41)
i16 resourceManager::MakeId(char* name) {
    u32 result = MAKEFILEID(name);
    strcpy(m_lastFileName, name);
    m_lastFileId = result;
    return result;
}

// donor Buka RVA 0x000b8f40; constant and call shape are identical in HoMM1
VA(0x0046cd95, 0x1b)
void resourceManager::Read13(i8* destination) {
    ReadBlock(destination, RESOURCE_NAME_CAPACITY);
}

// donor Buka RVA 0x000b8f60; HoMM1 omits the later error-reporting branch
VA(0x0046cdb0, 0x52)
#line 679 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Base\\RESMGR.CPP"
void resourceManager::ReadBlock(i8* destination, u32 size) {
#line 680
    H1_ASSERT(m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE);
    PollSound();
    i32 bytesRead = _read(m_aggregateFd, destination, size);
    PollSound();
}

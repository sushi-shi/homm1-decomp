#ifndef HOMM1_BASE_RESOURCEMANAGER_H
#define HOMM1_BASE_RESOURCEMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/icon.h>
#include <BASE/message.h>
#include <BASE/resource.h>

class MIDIWrap;
class bitmap;
class font;
class icon;
class mouse;
class palette;
class sample;
class tileset;
H1_ENUM_CONST_BEGIN(ResourceManagerConstant)
    RESOURCE_MANAGER_INVALID_FILE = -1,
    RESOURCE_MANAGER_LOAD_ERROR = 3,
    RESOURCE_MANAGER_BINARY_OPEN_MODE = 0x8000,
    RESOURCE_MANAGER_FILENAME_CAPACITY = 60,
    // Read13's fixed-width resource name field (name plus terminator).
    RESOURCE_NAME_CAPACITY = 13
H1_ENUM_CONST_END(ResourceManagerConstant)

#pragma pack(push, 1)
struct aggEntry {
    i16 id;
    i32 offset;
    u32 size;
};

class resourceManager : public baseManager {
public:
    resource* m_resourceListHead;
    i32 m_aggregateFd;
    aggEntry* m_aggregateDir;
    i16 m_aggregateEntryCount;
    i32 m_expunging;
    i32 m_savedPosition;
    char m_lastFileName[RESOURCE_MANAGER_FILENAME_CAPACITY];
    i32 m_lastFileId;

    resourceManager();
    virtual i16 Open(i16 priority);
    virtual void Close();
    virtual H1_ENUM_RETURN(MessageDispatchResult, i16) Main(tag_message&);
    void GetBackdrop(char* name, bitmap* backdrop);
    void GetBackdropAtLoc(char* filename, bitmap* destination, i32 destinationX, i32 destinationY);
    palette* GetPalette(char* name);
    bitmap* GetBitmap(char* name);
    icon* GetIcon(char* name);
    icon* GetIcon(i16 fileId);
    tileset* GetTileset(char* name);
    mouse* GetMouse(char*);
    font* GetFont(char* name);
    sample* GetSample(char* name);
    MIDIWrap* GetMIDIWrap(char* name);
    void Dispose(resource* resourceToDispose);
    void AddResource(resource* newResource);
    void Expunge();
    resource* Query(i16 resourceId);
    void RemoveResource(resource* resourceToRemove);
    i16 LoadAggregateHeader(char* aggregateName);
    void PointToFile(i16 fileId);
    u32 GetFileSize(i16 fileId);
    void SavePosition();
    void RestorePosition();
    i8 ReadByte();
    i16 ReadWord();
    i32 ReadLong();
    i16 MakeId(char* name);
    void Read13(char* destination);
    void ReadBlock(void* destination, u32 size);
};
#pragma pack(pop)

#endif

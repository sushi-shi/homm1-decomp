#ifndef HOMM1_BASE_RESOURCEMANAGER_H
#define HOMM1_BASE_RESOURCEMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/icon.h>
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
    RESOURCE_MANAGER_BACKDROP_ROW_BYTES = 640,
    RESOURCE_MANAGER_FILENAME_CAPACITY = 60,
    // Read13's fixed-width resource name field (name plus terminator).
    RESOURCE_NAME_CAPACITY = 13
H1_ENUM_CONST_END(ResourceManagerConstant)

#pragma pack(push, 1)
struct aggEntry {
    i16 id;
    i32 offset;
    u32 size;
    u32 unpackedSize;
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
    virtual i16 Open(i16);
    virtual void Close();
    virtual i16 Main(tag_message&);
    void GetBackdrop(char*, bitmap*);
    void GetBackdropAtLoc(char*, bitmap*, i32, i32);
    palette* GetPalette(char*);
    bitmap* GetBitmap(char*);
    icon* GetIcon(char*);
    icon* GetIcon(i16);
    tileset* GetTileset(char*);
    mouse* GetMouse(char*);
    font* GetFont(char*);
    sample* GetSample(char*);
    MIDIWrap* GetMIDIWrap(char* name);
    void Dispose(resource*);
    void AddResource(resource*);
    void Expunge();
    resource* Query(i16);
    void RemoveResource(resource*);
    i16 LoadAggregateHeader(char*);
    void PointToFile(i16);
    u32 GetFileSize(i16);
    void SavePosition();
    void RestorePosition();
    i8 ReadByte();
    i16 ReadWord();
    i32 ReadLong();
    i16 MakeId(char*);
    void Read13(i8*);
    void ReadBlock(i8*, u32);
};
#pragma pack(pop)

#endif

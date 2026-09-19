#ifndef HOMM1_BASE_RESOURCEMANAGER_H
#define HOMM1_BASE_RESOURCEMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/resource.h>

class MIDIWrap;
class bitmap;
class font;
class icon;
class mouse;
class palette;
class sample;
class tileset;
H1_ENUM_BEGIN(ResourceManagerConstant)
    RESOURCE_MANAGER_INVALID_FILE = -1,
    RESOURCE_MANAGER_LOAD_ERROR = 3,
    RESOURCE_MANAGER_BINARY_OPEN_MODE = 0x8000,
    RESOURCE_MANAGER_FILENAME_CAPACITY = 60,
    RESOURCE_MANAGER_SIZE = 0x86
H1_ENUM_END(ResourceManagerConstant)

#pragma pack(push, 1)
struct aggEntry {
    short id;
    long offset;
    unsigned long size;
    unsigned long unpackedSize;
};

class resourceManager : public baseManager {
public:
    resource *m_resourceListHead;
    int m_aggregateFd;
    aggEntry *m_aggregateDir;
    short m_aggregateEntryCount;
    int m_expunging;
    long m_savedPosition;
    char m_lastFileName[RESOURCE_MANAGER_FILENAME_CAPACITY];
    int m_lastFileId;

    resourceManager();
    virtual short Open(short);
    virtual void Close();
    virtual short Main(tag_message &);
    void GetBackdrop(char *, bitmap *, int);
    void GetBackdropAtLoc(char *, bitmap *, int, int, int);
    palette *GetPalette(char *);
    bitmap *GetBitmap(char *);
    icon *GetIcon(char *);
    icon *GetIcon(unsigned long);
    tileset *GetTileset(char *);
    mouse *GetMouse(char *);
    font *GetFont(char *);
    sample *GetSample(char *);
    MIDIWrap *GetMIDIWrap(char *);
    void Dispose(resource *);
    void AddResource(resource *);
    void Expunge();
    resource *Query(short);
    void RemoveResource(resource *);
    short LoadAggregateHeader(char *);
    void PointToFile(unsigned long);
    unsigned long GetFileSize(unsigned long);
    void SavePosition();
    void RestorePosition();
    signed char ReadByte();
    short ReadWord();
    long ReadLong();
    short MakeId(char *);
    void Read13(signed char *);
    void ReadBlock(signed char *, unsigned long);
};
#pragma pack(pop)

typedef char ResourceManagerSizeCheck[
    sizeof(resourceManager) == RESOURCE_MANAGER_SIZE ? 1 : -1];

#endif

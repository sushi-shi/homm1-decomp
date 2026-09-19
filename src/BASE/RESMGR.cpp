// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/MAKEFILEID.h>
#include <BASE/Misc.h>
#include <BASE/resourceManager.h>
#include <H1/All.h>
#include <H1/KB.h>

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

short gReadByteAssertLine = 598;
char gReadByteAssertFile[] = "D:\\Heroes\\Base\\RESMGR.CPP";
short gReadWordAssertLine = 619;
char gReadWordAssertFile[] = "D:\\Heroes\\Base\\RESMGR.CPP";
short gReadLongAssertLine = 639;
char gReadLongAssertFile[] = "D:\\Heroes\\Base\\RESMGR.CPP";
short gReadBlockAssertLine = 679;
char gReadBlockAssertFile[] = "D:\\Heroes\\Base\\RESMGR.CPP";

// donor PoL RVA 0x000c8080; preferred Buka symbol ?GetBackdrop@resourceManager@@QAEXPADPAVbitmap@@H@Z
// donor Buka TU BASE/RESMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.523311;margin=0.276556;shape=0.370;size=0.986;calls=0.778;alternate=pol20:void resourceManager::GetBackdrop(char *, class bitmap *, int)@0x000c8080
VA(0x004758d0, 0x90)
void resourceManager::GetBackdrop(char *, class bitmap *, int) {}

// donor PoL RVA 0x000c8130; preferred Buka symbol ?GetBackdropAtLoc@resourceManager@@QAEXPADPAVbitmap@@HHH@Z
// donor Buka TU BASE/RESMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.388383;margin=0.191324;shape=0.224;size=0.750;calls=0.667;alternate=pol20:void resourceManager::GetBackdropAtLoc(char *, class bitmap *, int, int, int)@0x000c8130
VA(0x00475960, 0x90)
void resourceManager::GetBackdropAtLoc(char *, class bitmap *, int, int, int) {}

// donor PoL RVA 0x000c8570; preferred Buka symbol ?GetSample@resourceManager@@QAEPAVsample@@PAD@Z
// donor Buka TU BASE/RESMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.514578;margin=0.400000;shape=0.333;size=0.838;calls=1.000;alternate=pol20:class sample * resourceManager::GetSample(char *)@0x000c8570
VA(0x00475d40, 0xa0)
class sample * resourceManager::GetSample(char *) { return 0; }

// donor PoL RVA 0x000c86b0; preferred Buka symbol ?Dispose@resourceManager@@QAEXPAVresource@@@Z
// donor Buka TU BASE/RESMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.520865;margin=0.403030;shape=0.400;size=0.812;calls=1.000;alternate=pol20:void resourceManager::Dispose(class resource *)@0x000c86b0
VA(0x00475de0, 0x87)
void resourceManager::Dispose(class resource *resourceToDispose)
{
    if (m_expunging != 0)
        return;
    if (resourceToDispose != 0) {
        resourceToDispose->m_refCount--;
        if (resourceToDispose->m_refCount > 0) {
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
VA(0x00475e70, 0x55)
void resourceManager::AddResource(class resource *newResource)
{
    if (m_resourceListHead == 0) {
        m_resourceListHead = newResource;
        m_resourceListHead->m_next = 0;
    } else {
        newResource->m_next = m_resourceListHead;
        m_resourceListHead = newResource;
    }
}

// donor Buka RVA 0x000b8740; PoL 2.0 has the same list walk and deletion order
VA(0x00475ed0, 0x8b)
void resourceManager::Expunge(void)
{
    m_expunging = 1;
    resource *cursor[2];
    cursor[1] = m_resourceListHead;
    cursor[0] = 0;
    while (cursor[1] != 0) {
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
VA(0x00475f60, 0x4f)
class resource *resourceManager::Query(short resourceId)
{
    resource *cursorResource = m_resourceListHead;
    while (cursorResource != 0 && cursorResource->m_id != resourceId)
        cursorResource = cursorResource->m_next;
    return cursorResource;
}

// donor Buka RVA 0x000b8800; HoMM1 returns its dispatch result through AX
VA(0x00475fb0, 0x1b)
short resourceManager::Main(tag_message &)
{
    return 0;
}

// donor Buka RVA 0x000b8810; HoMM1 loads only the default aggregate
VA(0x00475fd0, 0x8e)
short resourceManager::Open(short priority)
{
    if (LoadAggregateHeader(DEFAULT_AGGREGATE_NAME) != 0)
        return 3;
    m_messageMask = BASE_MANAGER_ACCEPT_RESOURCE;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "resourceManager");
    m_resourceListHead = 0;
    return 0;
}

// donor Buka RVA 0x000b8890; PoL 2.0 is source-identical
VA(0x00476060, 0x88)
void resourceManager::RemoveResource(class resource *resourceToRemove)
{
    if (m_resourceListHead == resourceToRemove) {
        m_resourceListHead = resourceToRemove->m_next;
        return;
    }
    resource *previousResource = m_resourceListHead;
    while (previousResource != 0 && previousResource->m_next != resourceToRemove)
        previousResource = previousResource->m_next;
    if (previousResource == 0) {
        return;
    } else {
        previousResource->m_next = resourceToRemove->m_next;
    }
}

// donor Buka RVA 0x000b89b0; HoMM1 replaces one packed aggregate directory
VA(0x00476180, 0x100)
short resourceManager::LoadAggregateHeader(char *aggregateName)
{
    short directoryBytes;
    int aggregateFp = open(aggregateName, RESOURCE_MANAGER_BINARY_OPEN_MODE);
    if (aggregateFp == RESOURCE_MANAGER_INVALID_FILE) {
        sprintf(gText, "Can't open file: %s", aggregateName);
        ShutDown(gText);
        return RESOURCE_MANAGER_LOAD_ERROR;
    }
    if (m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE)
        close(m_aggregateFd);
    if (m_aggregateDir != 0)
        free(m_aggregateDir);
    m_aggregateFd = aggregateFp;
    read(m_aggregateFd, &m_aggregateEntryCount, sizeof(m_aggregateEntryCount));
    directoryBytes = m_aggregateEntryCount * sizeof(aggEntry);
    m_aggregateDir = static_cast<aggEntry *>(malloc(directoryBytes));
    read(m_aggregateFd, m_aggregateDir, directoryBytes);
    return 0;
}

// donor PoL RVA 0x000c8c00; preferred Buka symbol ?PointToFile@resourceManager@@QAEXK@Z
// donor Buka TU BASE/RESMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.595306;margin=0.474349;shape=0.195;size=0.969;calls=0.750;strings=ResMgr::PointToFile failure!  ThisFileId:%d  LastFileId:%d  LastFileName:%s;alternate=pol20:void resourceManager::PointToFile(unsigned long int)@0x000c8c00
VA(0x00476280, 0x100)
void resourceManager::PointToFile(unsigned long int) {}

// donor PoL RVA 0x000c8e20; preferred Buka symbol ?SavePosition@resourceManager@@QAEXXZ
// donor Buka TU BASE/RESMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.435067;margin=0.395891;shape=0.259;size=0.600;calls=1.000;alternate=pol20:void resourceManager::SavePosition(void)@0x000c8e20
VA(0x00476470, 0x2b)
void resourceManager::SavePosition(void)
{
    m_savedPosition = tell(m_aggregateFd);
}

// donor PoL RVA 0x000c8e80; preferred Buka symbol ?RestorePosition@resourceManager@@QAEXXZ
// donor Buka TU BASE/RESMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.425713;margin=0.238867;shape=0.231;size=0.593;calls=1.000;alternate=pol20:void resourceManager::RestorePosition(void)@0x000c8e80
VA(0x004764a0, 0x2e)
void resourceManager::RestorePosition(void)
{
    lseek(m_aggregateFd, m_savedPosition, 0);
}

// donor Buka RVA 0x000b8d80; HoMM1 uses its single aggregate descriptor
VA(0x004764d0, 0x55)
signed char resourceManager::ReadByte(void)
{
    ProcessAssert(
        m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE,
        gReadByteAssertFile,
        gReadByteAssertLine + 1);
    signed char value = 0;
    read(m_aggregateFd, &value, sizeof(value));
    return value;
}

// donor PoL RVA 0x000c8f70; preferred Buka symbol ?ReadWord@resourceManager@@QAEFXZ
// donor Buka TU BASE/RESMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.481320;margin=0.600000;shape=0.261;size=0.958;calls=1.000;alternate=pol20:short int resourceManager::ReadWord(void)@0x000c8f70
VA(0x00476530, 0x58)
short int resourceManager::ReadWord(void)
{
    ProcessAssert(
        m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE,
        gReadWordAssertFile,
        gReadWordAssertLine + 1);
    short value = 0;
    read(m_aggregateFd, &value, sizeof(value));
    return value;
}

// donor Buka RVA 0x000b8e40; HoMM1 uses its single aggregate descriptor
VA(0x00476590, 0x58)
long resourceManager::ReadLong(void)
{
    ProcessAssert(
        m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE,
        gReadLongAssertFile,
        gReadLongAssertLine + 1);
    long value = 0;
    read(m_aggregateFd, &value, sizeof(value));
    return value;
}

// donor Buka RVA 0x000b8ea0; HoMM1 has no translation argument and uses 16-bit IDs
VA(0x004765f0, 0x5f)
short resourceManager::MakeId(char *name)
{
    unsigned long result = MAKEFILEID(name);
    strcpy(m_lastFileName, name);
    m_lastFileId = result;
    return result;
}

// donor Buka RVA 0x000b8f40; constant and call shape are identical in HoMM1
VA(0x00476650, 0x26)
void resourceManager::Read13(signed char *destination)
{
    ReadBlock(destination, 13);
}

// donor Buka RVA 0x000b8f60; HoMM1 omits the later error-reporting branch
VA(0x00476680, 0x5f)
void resourceManager::ReadBlock(signed char *destination, unsigned long size)
{
    ProcessAssert(
        m_aggregateFd != RESOURCE_MANAGER_INVALID_FILE,
        gReadBlockAssertFile,
        gReadBlockAssertLine + 1);
    PollSound();
    int bytesRead = read(m_aggregateFd, destination, size);
    PollSound();
}

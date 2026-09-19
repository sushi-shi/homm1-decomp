// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/Misc.h>
#include <BASE/resourceManager.h>
#include <H2/_all.h>

#include <io.h>

short gReadWordAssertLine = 619;
char gReadWordAssertFile[] = "D:\\Heroes\\Base\\RESMGR.CPP";

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
VA(0x00475de0, 0x90)
void resourceManager::Dispose(class resource *) {}

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

// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/X_GLOBAL.h>

// donor PoL RVA 0x00032c00; preferred Buka symbol ??0town@@QAE@XZ
// donor Buka TU SOURCE/TOWN; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.468183;margin=0.431232;shape=0.273;size=0.841;calls=1.000;alternate=pol20:void town::constructor(void)@0x00032c00
VA(0x00463f10, 0x6b)
town::town(void) {
    m_type = 0;
    m_threat = 0;
    m_id = 0;
    m_owner = 0;
    m_x = 0;
    m_y = 0;
    m_occupyingHeroId = -1;
    m_buildings = 0x20;
    m_buildState = 0;
    m_unknown19 = 0;
}

// donor PoL RVA 0x00032c65; preferred Buka symbol ?HasGarrison@town@@QAEHXZ
// donor Buka TU SOURCE/TOWN; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.410733;margin=0.365325;shape=0.175;size=0.741;calls=1.000;alternate=pol20:int town::HasGarrison(void)@0x00032c65
// HoMM1 retail returns in AL; the HoMM2 int return is a later signature.
VA(0x00463f7b, 0x55)
signed char town::HasGarrison(void) {
    for (short slot = 0; slot < 5; ++slot) {
        if (m_army.m_creatureTypes[slot] != -1)
            return 1;
    }
    return 0;
}

// donor PoL RVA 0x00032cb9; preferred Buka symbol ?GiveSpells@town@@QAEXPAVhero@@@Z
// donor Buka TU SOURCE/TOWN; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.430933;margin=0.601053;shape=0.167;size=0.987;calls=0.667;alternate=pol20:void town::GiveSpells(class hero *)@0x00032cb9
VA(0x00463fd0, 0xe1)
void town::GiveSpells(void) {}

// donor PoL RVA 0x00032e74; preferred Buka symbol ?View@town@@QAEXH@Z
// donor Buka TU SOURCE/TOWN; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.510810;margin=0.878787;shape=0.327;size=0.842;calls=1.000;alternate=pol20:void town::View(int)@0x00032e74
// HoMM1's callee returns with `ret` and always fades; the donor's noFade
// argument and memory-limit calculation belong to its later revision.
VA(0x0046422d, 0xa5)
void town::View(void) {
    if (giHighMemBuffer > 200)
        gAdvDisposeLevel = 2;
    else
        gAdvDisposeLevel = 1;

    townManager *manager = gpTownManager;
    manager->SetTown(this);
    gpWindowManager->FadeScreen(1, 8, 0);
    gpExec->CallManager(gpTownManager);
    if (m_occupyingHeroId != -1)
        gpAdvManager->SetHeroContext(m_occupyingHeroId, 0);
    gAdvDisposeLevel = 0;
}

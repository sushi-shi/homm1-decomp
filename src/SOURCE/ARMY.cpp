// Combat stacks and the computer's combat moves. HoMM1 keeps Buka's
// SOURCE/ARMY methods after the SOURCE/AI combat helpers in one object
// (retail 0x00464520-0x0046ba8f; hero::hero starts the next one).

#include <match.h>

#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// donor PoL RVA 0x000c0790; preferred Buka symbol ?AICheckRetreat@combatManager@@QAEHXZ
// donor Buka TU SOURCE/AI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.550932;margin=0.199965;shape=0.407;size=0.920;calls=0.857;alternate=pol20:int combatManager::AICheckRetreat(void)@0x000c0790
VA(0x00464520, 0x783)
int combatManager::AICheckRetreat(void) { return 0; }

// The Windows build waits on no sample channel.
VA(0x00466559, 0x18)
void army::WaitSample(int) {
    return;
}

VA(0x00466571, 0x71)
void army::InitClean(void) {
    int i;

    for (i = 0; i < 4; i++)
        m_samples[i] = 0;
    m_unknown2f = -1;
    m_unknown33 = 1;
    m_attackIcon = 0;
    m_walkIcon = 0;
    m_standIcon = 0;
}

// The commanding hero's attack and defense raise the copied creature stats.
VA(0x004665e2, 0x122)
void army::Init(signed char type, short quantity, signed char side, signed char index) {
    hero* commander;

    InitClean();
    m_monsterType = type;
    memcpy(&m_stats, &gMonsterDatabase[type].stats, sizeof(tag_monsterStats));
    m_unknown29 = 6;
    m_unknown51 = -1;
    m_unknown52 = -1;
    commander = gpCombatManager->m_heroes[side];
    if (commander) {
        m_stats.attack = commander->m_primaryStats[0] + m_stats.attack;
        m_stats.defense = commander->m_primaryStats[1] + m_stats.defense;
    }
    m_facing = side ^ 1;
    m_unknown0b = 0;
    m_unknown08 = 0;
    m_unknown09 = 1;
    m_speed = m_stats.speed;
    m_quantity = quantity;
    m_initialQuantity = m_quantity;
    m_hitPointsLost = 0;
    m_unknown13 = 0;
    m_unknown2b = -1;
    m_side = side;
    m_index = index;
}

VA(0x00466704, 0x237)
void army::LoadResources(void) {
    char sprite[16];
    int i;
    char buf[16];

    if (m_monsterType != 3)
        strcpy(sprite, gArmyNames[m_monsterType]);
    else
        strcpy(sprite, "swrdsman");
    sprintf(gText, "%s.std", sprite);
    giMonoIconSkip = 0;
    m_standIcon = gpResourceManager->GetIcon(gText);
    giMonoIconSkip = -1;
    sprintf(gText, "%s.wlk", sprite);
    m_walkIcon = gpResourceManager->GetIcon(gText);
    sprintf(gText, "move%02d.82M", m_monsterType);
    m_samples[0] = gpResourceManager->GetSample(gText);
    sprintf(gText, "atksnd%02d.82M", m_monsterType);
    m_samples[1] = gpResourceManager->GetSample(gText);
    sprintf(gText, "wince%02d.82M", m_monsterType);
    m_samples[2] = gpResourceManager->GetSample(gText);
    if (m_stats.attributes & 4) {
        sprintf(gText, "%s.atk", sprite);
        m_attackIcon = gpResourceManager->GetIcon(gText);
        sprintf(gText, "shoot%02d.82M", m_monsterType);
        m_samples[3] = gpResourceManager->GetSample(gText);
    } else {
        m_attackIcon = 0;
        m_samples[3] = 0;
    }
    for (i = 0; i < 4; i++) {
        if (m_samples[i]) {
            m_samples[i]->m_playbackData.volume = 0x40;
            m_samples[i]->m_playbackData.channelType = 3;
            m_samples[i]->m_playbackData.loopCount = 1;
        }
    }
}

// donor PoL RVA 0x0004b36e; preferred Buka symbol ?FreeResources@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.463954;margin=0.140795;shape=0.318;size=0.842;calls=0.750;alternate=pol20:void army::FreeResources(void)@0x0004b36e
VA(0x0046693b, 0xf1)
void army::FreeResources(void) {
    int i;

    if (m_standIcon) {
        gpResourceManager->Dispose(m_standIcon);
        m_standIcon = 0;
    }
    if (m_walkIcon) {
        gpResourceManager->Dispose(m_walkIcon);
        m_walkIcon = 0;
    }
    if ((m_stats.attributes & 4) && m_attackIcon) {
        gpResourceManager->Dispose(m_attackIcon);
        m_attackIcon = 0;
    }
    for (i = 0; i < 4; i++) {
        if (m_samples[i]) {
            gpResourceManager->Dispose(m_samples[i]);
            m_samples[i] = 0;
        }
    }
}

// donor PoL RVA 0x0004c7e5; preferred Buka symbol ?SpecialAttack@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.357822;margin=0.373223;shape=0.266;size=0.572;calls=0.585;alternate=pol20:void army::SpecialAttack(void)@0x0004c7e5
VA(0x00467b97, 0xcca)
void army::SpecialAttack(void) {}

// donor PoL RVA 0x0004e1a1; preferred Buka symbol ?DoAttack@army@@QAEXH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.427643;margin=0.977018;shape=0.217;size=0.817;calls=0.724;alternate=pol20:void army::DoAttack(int)@0x0004e1a1
VA(0x00468ff3, 0x108d)
void army::DoAttack(int) {}

VA(0x0046a080, 0x49)
void army::ResetPath(void) {
    short i;

    for (i = 0; i < COMBAT_HEX_COUNT; i++)
        gpCombatManager->m_hexCells[i].m_pathFlag = 0;
}

VA(0x0046a0c9, 0x27)
short army::WalkTo(void) {
    return WalkTo(m_targetHex);
}

VA(0x0046a1ec, 0x27)
short army::AttackTo(void) {
    return AttackTo(m_targetHex);
}

// donor PoL RVA 0x0004f93e; preferred Buka symbol ?CheckLuck@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.723107;margin=0.234495;shape=0.473;size=0.925;calls=0.875;strings=badluck.82m|goodluck.82m;alternate=pol20:void army::CheckLuck(void)@0x0004f93e
VA(0x0046a3dc, 0x249)
void army::CheckLuck(void) {}

// donor PoL RVA 0x0004fbc0; preferred Buka symbol ?DamageEnemy@army@@QAEXPAV1@PAH1HH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.387284;margin=0.228483;shape=0.296;size=0.628;calls=0.714;alternate=pol20:void army::DamageEnemy(class army *, int *, int *, int, int)@0x0004fbc0
VA(0x0046a625, 0x2ae)
void army::DamageEnemy(class army *, int *, int *, int, int) {}

// donor PoL RVA 0x0005012e; preferred Buka symbol ?Damage@army@@QAEHJH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.419408;margin=1.157007;shape=0.216;size=0.693;calls=1.000;alternate=pol20:int army::Damage(long int, int)@0x0005012e
VA(0x0046a8d3, 0x176)
int army::Damage(long int, int) { return 0; }

VA(0x0046b2f2, 0x34)
unsigned long int army::Strength(void) {
    return gMonsterDatabase[m_monsterType].fightValue * m_quantity;
}

// donor PoL RVA 0x00052ad9; preferred Buka symbol ?MoveAttack@army@@QAEXHH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.522810;margin=0.525862;shape=0.325;size=0.919;calls=0.929;alternate=pol20:void army::MoveAttack(int, int)@0x00052ad9
VA(0x0046b6e7, 0x3a9)
void army::MoveAttack(int, int) {}


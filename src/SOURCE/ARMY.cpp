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

// donor PoL RVA 0x0004b36e; preferred Buka symbol ?FreeResources@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.463954;margin=0.140795;shape=0.318;size=0.842;calls=0.750;alternate=pol20:void army::FreeResources(void)@0x0004b36e
VA(0x0046693b, 0xf1)
void army::FreeResources(void) {}

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

// donor PoL RVA 0x00052ad9; preferred Buka symbol ?MoveAttack@army@@QAEXHH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.522810;margin=0.525862;shape=0.325;size=0.919;calls=0.929;alternate=pol20:void army::MoveAttack(int, int)@0x00052ad9
VA(0x0046b6e7, 0x3a9)
void army::MoveAttack(int, int) {}


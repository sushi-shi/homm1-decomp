#ifndef HOMM1_SOURCE_NOOPT_H
#define HOMM1_SOURCE_NOOPT_H

extern short gNooptAssertLine;
// Stale alias of DelayTil's NOOPT.CPP literal (0x4a0804): unreferenced, kept so later symbol handles stay put.
extern char gNooptAssertFile[];

void DelayTicks(int);
void DelayTil(int *);
void DelayMilli(long);
void DelayTilMilli(long);

#endif

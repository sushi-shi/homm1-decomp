#ifndef HOMM1_SOURCE_NOOPT_H
#define HOMM1_SOURCE_NOOPT_H

extern short gNooptAssertLine;
extern char gNooptAssertFile[];

void DelayTicks(int);
void DelayTil(long *);
void DelayMilli(long);
void DelayTilMilli(long);

#endif

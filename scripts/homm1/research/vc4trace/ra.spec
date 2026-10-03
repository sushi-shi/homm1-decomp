# select: register chosen for a node (ecx = reg number from table 0x47f390)
0x40cbe1 | "C node=%x reg=%x f0=%x f4=%x f8=%x fc=%x f10=%x f14=%x w16=%x f18=%x\n" | d[esp+0x14] ecx d[d[esp+0x14]] d[d[esp+0x14]+4] d[d[esp+0x14]+8] d[d[esp+0x14]+0xc] d[d[esp+0x14]+0x10] d[d[esp+0x14]+0x14] sw[d[esp+0x14]+0x16] d[d[esp+0x14]+0x18]

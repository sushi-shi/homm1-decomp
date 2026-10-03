# --- C2 /O2 register allocation: range ids, simplify worklist/removals/edges, select ---
0x4151f5 | "F %x\n" | d[d[d[0x48173c]+4]+0x24]
# range id assignment (0x4113de walk): id, bucket slot address (0x48a540 + 4*bucket), symbol type word
0x411460 | "A %x %x %x\n" | eax d[esp+0x10] w[esi+4]
# simplify worklist build: node prepended (id degree weight)
0x40cd01 | "W %x %x %x\n" | w[eax+0x18] sw[eax+0x14] sw[eax+0x16]
# simplify removes a node (id degree weight)
0x40cd33 | "P %x %x %x\n" | w[d[ebx]+0x18] sw[d[ebx]+0x14] sw[d[ebx]+0x16]
# edge of the removed node (removed id, neighbour id) as its neighbour's degree drops
0x40cd47 | "E %x %x\n" | w[eax+0x18] w[d[edx+4]+0x18]
# select: node coloured (id register weight)
0x40cbe1 | "C %x %x %x\n" | w[d[esp+0x14]+0x18] ecx sw[d[esp+0x14]+0x16]

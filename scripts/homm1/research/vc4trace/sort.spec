# --- C2 sortnode tracer (VC4 C2.EXE) ---
# /Od statement root (0x475755 path) and /O2 tree root (0x408b40 driver); fn = current function's C1 handle
0x47576f | "S %x %x\n" | esi d[d[d[0x48173c]+4]+0x24]
0x408b61 | "S %x %x\n" | edi d[d[d[0x48173c]+4]+0x24]
0x408bc4 | "S %x %x\n" | edi d[d[d[0x48173c]+4]+0x24]
0x408bec | "S %x %x\n" | edi d[d[d[0x48173c]+4]+0x24]
# node weight computed (before exchange): node op w L R kid0 kid1 type-word
0x408daf | "N %x %x %x %x %x %x %x %x\n" | ebx d[ebx] eax ebp edi d[ebx+0xc] d[ebx+0x10] w[ebx+4]
# symbol leaf: node handle sym-kind-byte
0x408588 | "L %x %x %x\n" | edi d[eax+0x24] b[eax+0xc]
# exchange(node, L, R): swap iff R > L
0x413ded | "X %x %x %x\n" | ecx edx d[esp+4]
# reassociation fires (0x413e27 decided to rotate): node lc Rw prevW
0x413e5c | "R %x %x %x %x\n" | ecx esi edx d[esp+0x10]

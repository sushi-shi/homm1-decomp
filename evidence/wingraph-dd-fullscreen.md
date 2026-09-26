# DirectDraw mode switch

RetailRVA0x490e size0x2ea, previous empty mapped body5.68%, reconstructed within graphics-family continuation. Buka SOURCE/wingraph.cpp635-713 and PoL516-600 preserve complete body. Five real locals width/windowHeight/x/y/hres retained from preferred source. PoL windowHeight0/result0 spelling measured99.86%; preferred spelling99.89%. Both complete body states, preferred retained; allocation residue first+0x6d remains open.

Retail746bytes/197instructions/17calls/16branches/67relocations agree. Existing real config gfx stride24 and geometry fields(+0x1c..28) save/restore windowed dimensions. Uses real COM mode changes, surface release/recreation, palette assignment, WritePrefs, menu/window resize and clip setup. Diagnostic signed wordVA0x48e948 +21/+27/+34/+39/+51 independently loaded; identity/layout only, no initializer coverage.

Uses root's real kbwin.h for menu/window declarations; earlier temporary H1/KB declaration removed. WritePrefs is donor BASE/Misc.cpp1486, declared in Misc.h; its strict retail0x5d740 identity already reviewed at root for CD playback and mirrored privately, excluded patch. Full body never filled with speculative callees. No byte masking or tooling policy changes.

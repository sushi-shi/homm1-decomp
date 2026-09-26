# DirectDraw cleanup

Retail RVA0x478f, size0x17f, existing empty body baseline11.07%, reconstructed under the active graphics backend assignment. Buka SOURCE/wingraph.cpp591-632 and PoL481-519 preserve its full body and two locals restoreResult/result. The restoreResult value is unobserved but genuinely stored by both donors and retail at +0x26, not stack padding.

Retail restores display mode, conditionally detaches/releases clipper, releases each surface and palette, selects DDSCL_NORMAL, and releases the DirectDraw owner. Error DDSD calls use source-line-base wordVA0x48e904 +14/+38 and the retail filename. Genuine DDERR_NOCLIPPERATTACHED is MAKE_DDHRESULT(568), value0x88760238.

Microsoft donor DDRAW.H704-718 supplies the real nine-method IDirectDrawClipper interface; pure external declarations, no game body. lpDDSOneVA0x48e5b8 and lpClipperVA0x48e5bc are independent four-byte owner pointers, observed by calls and null stores here; no initializer/data-byte coverage.

Intended source measures99.92%, with candidate and retail matching383bytes,103instructions,10calls,10branches,34relocations. First differing frame-local slot at +0x28; donor names retained. Root must reverify its TU state. Private snapshot data-identity gate may report WGInitGraphics width/height same-object operand swaps; root already has the complete address-multiset proof fix. No policy changed here.

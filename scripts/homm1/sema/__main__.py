"""`python3 -m homm1.sema <view> ...` - the same dispatch `homm1 sema` uses."""

from __future__ import annotations

import sys

from homm1.sema import main

if __name__ == "__main__":
    sys.exit(main())

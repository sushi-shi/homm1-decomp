"""`python3 -m homm1.ghidra <verb> ...` - the same dispatch `homm1 ghidra` uses."""

from __future__ import annotations

import sys

from homm1.ghidra import main

if __name__ == "__main__":
    sys.exit(main())

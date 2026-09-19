"""homm1.retail_labels - every label, as typed records.

One concern from two directions: the committed provider tables and censuses
are PARSED (censuses/providers/fragments - parse-only, zero policy), the
source macros are EXTRACTED (source.py - clang IR/AST + the text-only
channels, the base-obj authority join, and the spelling repairs: extraction
is the package's second, join-bearing concern and its CLI is public as
`homm1 labels`). Both produce the same Claim record; the MODEL is the only
consumer of the records.

A module here enforces syntax (columns, hex spelling, ranges) and NOTHING
else: no joins, no confidence filtering, no precedence - all policy lives in
the model. Nothing outside the model imports this package (a verify check
enforces the import graph).

The one record every provider/extraction parser emits:

    Claim(rva, name, kind, channel, size, unit, meta)

kind is 'func' | 'data'; channel names the source table/mechanism; size is
the claimed exact extent or None (label-only channels); meta carries the
channel's extra columns verbatim.
"""

from __future__ import annotations

from typing import NamedTuple


class Claim(NamedTuple):
    rva: int
    name: str
    kind: str          # 'func' | 'data'
    channel: str       # 'src' | 'src_compgen' | 'src_dyninit'
                       # | 'src_data_compgen' | table stem
    size: int | None
    unit: str
    meta: dict

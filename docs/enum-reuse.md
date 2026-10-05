# Enum reuse review

`homm1 verify enum-reuse` evaluates every project enum member and groups
declarations by integer value. Equal numbers are review leads, not evidence
that two domains are one type.

The command combines a lexical inventory of `include/` and `src/` (every
`H1_ENUM_*` form from `include/Domains.h` and raw `enum` blocks; Domains.h's
own macro machinery is skipped) with libclang evaluation of every translation
unit in `build/clangd/compile_commands.json`. A source member that is not
evaluated, or an evaluated member that is not inventoried, is fatal. It writes
derived reports to ignored `build/gen/`:

- `enum_reuse.tsv`: evaluated members and their source contexts.
- `enum_value_collisions.tsv`: declarations grouped by value, joined with bare
  function literals of the same value from `homm1 verify constants`.
- `enum_domain_pairs.tsv`: pairs with overlapping value sets.
- `enum_role_pairs.tsv`: pairs where at least two equal values also have equal
  member-name suffixes after each enum's common prefix. A search aid only.

Every evaluated member also records its use contexts: the declaration
identity (field, parameter, comparison operand, switch subject, array, return)
that receives each reference to it. `enum_value_collisions.tsv` lists contexts
shared by two declarations of one value (`shared_named_contexts`) or by a
declaration and a bare literal of that value (`shared_literal_contexts`, read
from `build/gen/bare_constants.tsv`, so run `homm1 verify constants` first);
`enum_domain_pairs.tsv` ranks pairs by shared direct contexts before numeric
overlap. A shared destination is a lead for one domain; a transport that
carries several domains (the `tag_message::id` widget id, each window's own
controls) is not.

Use `--value N`, `--duplicates`, `--json` and `--no-report` to inspect.
`--extend-ledger` appends wholly new domains as `pending` rows.

## Decisions

`config/reviews/enum-reuse.tsv` snapshots each starting enum block with its
evaluated `name=value` members and one decision:

- `retain`: the enum keeps its domain; its values select a distinct quantity,
  table, operation, representation or state machine.
- `canonical`: this enum owns values that another reviewed block reuses.
- `reuse`: the members moved to the canonical enum; `member_reuse` maps every
  moved member to `source-enum::MEMBER`.
  A member that no code names any more maps to `-` (retired); the check
  requires its identifier to be absent from every file under `include/` and
  `src/`.
- `pending`: the producers, consumers and encodings still need review, or the
  reuse is decided but the source move has not landed. Pending rows keep the
  command nonzero.

Follow both value paths through their fields, callers and tables. Direct
transport of one quantity supports reuse (HoMM1's `combatManager::GetPointer`
returns its command as the pointer code). Two zero-based tables that only share
an order support retention (`MoraleInfoText` rows and `ArtifactType` medals).
`BaseManagerMessageMask` stays apart from `MessageType`, and per-window dialog
roles are spelled as aliases of the reserved slots (`FILE_REQUESTER_OK = DIALOG_BUTTON_2`), so role names stay with their
window while slot-named copies move to `DialogButtonId`. Constant groups
(`H1_ENUM_CONST`) are not value domains, but a member that repeats a domain's
quantity (the 640x480 logical screen, `RESOURCE_GOLD`, a widget command) is a
reuse lead like any other.

Every starting member needs a current home with the same value. New, removed
or changed members require a new decision: add the row when an enum is added.
Moving members changes C1 symbol numbering in the TU, so do source moves as a
reviewed batch and re-check edited functions with `homm1 match`.

## Buka ledger

The Buka branch inherited the NWC ledger. Its starting snapshot was rebased on
the Buka tree: a row whose current members and values are unchanged kept its
reviewed `retain`/`canonical` decision; every other current block started
`pending` (with the NWC reason quoted as a lead), and NWC rows for blocks that
do not exist in Buka were dropped.

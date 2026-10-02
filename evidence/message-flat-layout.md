# Flat `tag_message` layout

HoMM1 accesses every message word directly off `tag_message`; the earlier
Buka-style nesting (`message.payload.widget.id`) is not retail's member path.
The 16-byte packed layout is `type` (+0), the command/key/x word (+2), the
widget-id/y word (+4), `modifiers` (+6), four unknown bytes (+8) and the
data/result dword (+0xC). The anonymous unions in `include/BASE/message.h`
only provide per-message-type names for those words; codegen is the same
as with plain fields.

## Retail evidence

All five hover filters that compare the widget id with
`heroWindowManager::m_lastHoverId` (a `signed char` at +0x56) load the global
side first:

```
mov eax, ds:gpWindowManager
movsx eax, byte ptr [eax+56h]
mov ecx, [ebp+8]
movsx ecx, word ptr [ecx+4]
cmp eax, ecx
```

They are at VA 0x415660 (`CombatSpecialHandler`), 0x438966
(`HandleViewGeneral`), 0x43F6B9 (`ViewSpellsHandler`), 0x43F781
(`ViewSpecialHandler`) and 0x46DF26 (`HeroHandler`). `CastleHandler`
(0x40E89C) compares against the `short` `townManager::m_lastHoverId` and
loads the message first.

## VC4 measurements

Disposable single-function TUs (`/Od /G5 /Ob1 /GX`) with
`if (m.<path> == g->last)` were compiled with 0-16 unrelated preceding
`extern int` declarations:

- If `last` is a `signed char` and `<path>` is a direct member (`m.id`), the
  global was always loaded first. This was also true for members reached
  through anonymous unions or anonymous structs.
- If `<path>` contains any named aggregate level (`m.widget.id`, `m.u.id`, or
  `m.payload.widget.id`), the message was always loaded first. Neither source
  operand order nor casts changed this.
- If `last` is a `short` and `m.id` is direct, the order depended on the
  preceding declaration count and cycled with period 17. With the nested path,
  the message was always first.

In the full `SOURCE/HERO` TU, the unchanged nested source stayed at 99.80% with
0-16 inserted declarations. A flat view of the one compare made
`HeroHandler` exact.

These measurements do not identify HoMM1's original field names.

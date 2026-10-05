# Source markers

Addresses and symbols use `include/match.h`: `VA`, `VA_DECL`, `DATA` and
`VA_COMPGEN`. Comments do not create claims. `homm1 verify label-style` checks
canonical spelling and the closed leading `// @name` vocabulary.

| Marker | Meaning |
| --- | --- |
| `@early-stop` | Complete body with an evidence-bounded remaining difference; revalidate against current pairs |
| `@identity-TODO` | Unproved semantic identity; record what would resolve it |
| `@interleaver` | Evidence for code placed inside another contribution |
| `@dead-code` | Proven zero-ref body; still requires reconstruction |
| `@stub`, `@confidence`, `@source` | Source correspondence annotations; no byte-match claim |

Other notes are plain prose. The accepted vocabulary is defined in
`scripts/homm1/verify/label_style.py`. Do not add ad-hoc markers or treat a
historical score comment as proof. `walls recheck` revalidates written claims.

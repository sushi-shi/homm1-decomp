# playerData layout findings (not a closure)

Retail Read RVA0x38df8 and Write RVA0x38c00 agree on these transfer
addresses and sizes, apart from the reserved stack scratch handling:

| Offset | Size | Identity |
|---|---:|---|
| 0x00 | 17 | leading player name bytes (identity inferred, transfer proved) |
| 0x11 | 1 | color (donor correspondence) |
| 0x12 | 1 | AI difficulty (GetTurnAIVars signed comparisons 0/1/2/3) |
| 0x13 | 1 | heroCount |
| 0x14 | 1 | currentHero |
| 0x15 | 1 | heroLocatorPage |
| 0x16 | 8 | heroIds |
| 0x1e | 2 | availableHeroIds |
| 0x20..0x52 | 50 | unused legacy save interval; transferred to/from stack scratch, not member |
| 0x52 | 1 | ultimateArtifactHintChance (donor correspondence) |
| 0x53 | 1 | ultimateArtifactHintX (donor correspondence) |
| 0x54 | 1 | ultimateArtifactHintY (donor correspondence) |
| 0x55 | 1 | daysLeft (donor correspondence) |
| 0x56 | 1 | townCount (GetTurnAIVars signed read) |
| 0x57 | 1 | currentTown (donor correspondence) |
| 0x58 | 1 | townLocatorPage (donor correspondence) |
| 0x59 | 36 | townIds (serializer extent) |
| 0x7d | 28 | resources (MaxBuyableCreatures0x1e4a5 and TurnsToBuy0x1ec81) |
| 0x99 | 1 | evilInterface (donor correspondence; no direct serializer transfer) |
| 0x9a | 1 | barrierTents (serialized twice as in Buka) |
| 0x9b | 6 | barrier state bytes (serializer transfer) |
| 0xa1 | 24 | attention weights (six real floats) |
| 0xb9..0xd5 | 28 | unresolved runtime interval; no typed evidence found |
| 0xd5 | 28 | income[7] (serializer + TurnsToBuy) |
| 0xf1 | 4 | obeliskValue (GetTurnAIVars conversion return store) |
| 0xf5 | 4 | unexploredValue (GetTurnAIVars MeanRVOfUnexploredTerritory return) |
| 0xf9 | 4 | upgradeValueWeight (GetTurnAIVars FP computation/store) |
| 0xfd | 4 | artifactValue (GetTurnAIVars FP computation/store) |
| 0x101..0x105 | 4 | unresolved terminal runtime interval |

Attention functions at0x1e91b and0x1eaee derive game+0x2ad+player*0x105.
Given confirmed player base game+0x20c, this is player+0xa1. Their six
float fields are gameWeightA, gameRemainder, gameWeightB, buildingValue,
upgradeBase, heroValue at offsets0xa1,0xa5,0xa9,0xad,0xb1,0xb5.
GetTurnAIVars0x1ba7f also stores a float at0xb2, overlapping upgradeBase;
that retail fact is unresolved rather than hidden by an invented union.

Buka2.1 and PoL2.0 playerAIData contain m_unknown18[28] solely to span
the inherited runtime interval. This is secondary layout evidence, not
a demonstrated HoMM1 source aggregate. No new class declaration or
serializer body has been retained to manufacture these offsets.

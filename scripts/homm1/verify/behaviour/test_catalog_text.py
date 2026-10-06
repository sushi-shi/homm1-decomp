"""The combat screen's catalog sentences, in every language.

army::DoAttack, combatManager::KeepAttack and the luck and morale checks
compose these messages from catalog fragments and creature names with the
format strings below; the rendered sentences are snapshots of the shipped
catalog. Completeness, argument signatures and fixed-width fits are
`homm1 verify localization`'s.
"""
import unittest

from homm1.core.paths import REPO
from homm1.graph.catalog import Catalog

#: army::DoAttack and combatManager::KeepAttack (ARMY.cpp, CMBTMGR.cpp).
ATTACK = "%s %s %s %d %s.  %d %s %s."
TOWER = "%s %d %s. %d %s %s."
SWORDSMEN, GOBLIN, GOBLINS = "table.gArmyNamesPlural.3", "table.gArmyNames.6", "table.gArmyNamesPlural.6"

EXPECTED = {
    "en": (
        "The swordsmen do 64 Damage.  6 goblins perish.",
        "The swordsmen do 9 Damage.  1 goblin perishes.",
        "Garrison does 40 Damage. 13 goblins perish.",
        "Good luck shines on the swordsmen",
        "Bad luck descends on the swordsmen",
        "High morale enables the goblins to attack again.",
        "Low morale causes the goblins to freeze in panic.",
    ),
    # The Russian fragments carry their own full stop, so the sentence the
    # format closes ends in two (as the retail game shows it).
    "ru": (
        "Атака мечников наносит 64 ед. урона.  6 гоблинов умирает..",
        "Атака мечников наносит 9 ед. урона.  1 гоблин умирает..",
        "Гарнизон наносит 40 ед. урона. 13 гоблинов умирает..",
        "Удача! Отряд мечников наносит двойной урон!",
        "Неудача! Атака мечников наносит меньший урон.",
        "Высокая мораль дает отряду гоблинов еще одну атаку.",
        "Низкая мораль заставляет гоблинов замереть в панике.",
    ),
}


def sentences(messages: dict) -> tuple[str, ...]:
    def m(key):
        return messages[key]

    # DoAttack lowers the first letter of a single killed creature's name.
    goblin = m(GOBLIN)[:1].lower() + m(GOBLIN)[1:]
    return (
        ATTACK % (m("combat.fragment.attack"), m(SWORDSMEN), m("combat.fragment.does_damage"), 64,
                  m("combat.fragment.damage_points"), 6, m(GOBLINS), m("combat.fragment.killed")),
        ATTACK % (m("combat.fragment.attack"), m(SWORDSMEN), m("combat.fragment.does_damage"), 9,
                  m("combat.fragment.damage_points"), 1, goblin, m("combat.fragment.dies")),
        TOWER % (m("combat.tower.garrison.damage.prefix"), 40, m("combat.fragment.damage_points"),
                 13, m(GOBLINS), m("combat.fragment.killed")),
        m("combat.luck.good.buka") % m(SWORDSMEN),
        m("combat.luck.bad.buka") % m(SWORDSMEN),
        m("combat.morale.good") % m(GOBLINS),
        m("combat.morale.bad") % m(GOBLINS),
    )


class CatalogTextTest(unittest.TestCase):
    maxDiff = None

    def test_combat_sentences(self):
        locales = Catalog.load(REPO).locales
        self.assertEqual(sorted(locales), sorted(EXPECTED))
        for code, expected in EXPECTED.items():
            with self.subTest(locale=code):
                self.assertEqual(sentences(locales[code].messages), expected)


if __name__ == "__main__":
    unittest.main()

# Localization

The program has one code path, Buka's. Its text is resolved per language at
build time; nothing is translated at run time.

## Layout

| File | Contents |
| --- | --- |
| `locales/messages.pot` | The message IDs: one entry per `localization::Tr`/`Chars` ID in `src/` and `include/`, with the files that use it. Generated; do not edit. |
| `locales/<lang>.po` | One translation per language. `msgid` is the semantic ID, `msgstr` the text. |
| `locales/<lang>.json` | The language's descriptor (below). |
| `config/retail/localization*.tsv` | Retail provenance of the Russian text: pointer slots, string addresses and hashes. |

Every language is a translation, English included. `ru.po` holds the text
recovered from the retail executable. `en.po` keeps the original English
wording wherever it fits Buka's code. Where Buka changed the arguments, the
English is reworded for them: the shooting message has no plural suffix
argument, and the attack messages name the plural creature table.

A descriptor holds:

| Field | Meaning | `ru` | `en` |
| --- | --- | --- | --- |
| `name` | language name | Russian | English |
| `codepage` | single-byte Windows code page of the C++ literals | 1251 | 1252 |
| `resource_language` | Windows LANGID of `Heroes.rc` | `0x0419` | `0x0409` |
| `system_locale` | POSIX locale for Wine, so window titles and message boxes use the code page | `ru_RU.UTF-8` | `en_US.UTF-8` |
| `glyphs` | characters beyond ASCII that the text uses (`ascii` or `cyrillic`) | `cyrillic` | `ascii` |
| `keyboard` | `keys` (US-layout characters) and the characters they `typed` | ЙЦУКЕН | none |

## Source and build

Authored source names text by ID: `localization::Tr("adventure.confirm_quit")`
for a string, and `localization::Chars("id")` for a fixed-width `char` array
(a brace initializer of exactly the catalog's bytes, no terminator). Padding
spaces belong in the catalog text.

`locale.*` IDs are descriptor data, not messages.
`localization::Chars("locale.keyboard")` is INPUTMGR's 128-entry key
translation table. Each `keys` character maps to its `typed` character, and
every other code maps to itself.

The renderer replaces each call before compilation:

- **C++.** The matching build compiles a byte-preserving view in which calls
  become padded macro names. Every offset and line stays where the authored
  source has it: Clang gets a VFS overlay and VC6 a mirror. A generated
  forced-include header defines the macros as octal-escaped code page
  literals. Clang checks every printf call against the expanded literals.
- **Resources.** The renderer writes wide `L"\x...."` literals and defines
  `HOMM1_RESOURCE_LANGUAGE`/`HOMM1_RESOURCE_SUBLANGUAGE` from the descriptor.

The pinned target (`config/retail/targets.json`, `ru`) is the matching
language; its generated files are under `build/localization`. Other
languages build only into `build/ordinary/<lang>`. Catalog edits invalidate
the compile, label and resource edges, and function fingerprints hash the
matching language's expanded literals.

## Commands

```sh
homm1 localization update          # regenerate messages.pot; rewrite each .po in its order
homm1 localization update --check  # fail if any file is out of date
homm1 verify localization          # the gate (fast tier of `homm1 build verify`)
python3 -m unittest homm1.graph.test_localization
```

`update` sorts the template by ID, numbered parts by number. It keeps
translations and translator comments, drops IDs the source no longer uses, and
gives a new ID an empty `msgstr`. A descriptor without a `.po` gets one.

`homm1 verify localization` fails on:

- a template or `.po` that `update` would change;
- a language without both files, or an invalid descriptor;
- an ID missing from a language, a stale ID, a duplicate, an empty or fuzzy
  entry;
- printf arguments that differ between languages for one ID;
- a `Chars` text whose byte length differs between languages;
- text that cannot be encoded in the language's code page;
- outside `Heroes.rc`, a character beyond the descriptor's `glyphs`;
- in authored source, inline non-ASCII text or numeric escapes.

## Adding a language

1. Add `locales/<lang>.json` with the fields above.
2. Run `homm1 localization update` to create `locales/<lang>.po`. Fill every
   `msgstr`, keeping each entry's printf arguments and fixed-width byte
   lengths.
3. Run `homm1 verify localization`. Then build with
   `homm1 clean --working-tree`, and run
   `nix develop -c python3 build.py --locale <lang>` in the generated tree, or
   `nix run .#play -- --locale <lang>` there.

The flakes build their Wine locale archive from the descriptors'
`system_locale` values.

## Game data

The game draws its text with the fonts in `HEROES.AGG`. FONT.cpp maps
Windows-1251 Cyrillic to the glyph order of Buka's fonts. Every other byte
above `0x7F` draws as a blank. Text may therefore use printable ASCII and,
with Buka's data, Cyrillic: these are the `ascii` and `cyrillic` glyph sets.
The play runner requires Buka's `HEROES.AGG`, so both work. A language that
needs other glyphs, such as Latin accents, needs new font data and a different
glyph map in FONT.cpp. That is out of scope, and validation rejects such text.

Text entry is table-driven. INPUTMGR converts scan codes to US-layout
characters and then looks each one up in the keyboard table. For Russian, this
is the descriptor's ЙЦУКЕН layout, and the Russian table reproduces the retail
data at `0x4A1388`. Case conversion (`CyrillicToLower`/`CyrillicToUpper` in
`KB.h`) is Buka's code and stays as it is.

## Clean exports

`homm1 clean` copies `locales/` and `catalog.py` into the source tree. There,
`python3 build.py --locale <lang>` builds any language of the catalog, and
`python3 catalog.py check|update` validates and maintains it.
`homm1 clean --working-tree --verify` builds every language standalone. The
classic view spells out the matching language.

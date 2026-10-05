# Static localization

Buka text lives in UTF-8 catalogs that are resolved at build time.
`locales/messages.def` preserves the exact original English; `locales/ru.po`
uses the semantic ID as `msgctxt` and repeats that English as `msgid`.
Russian text is recovered from the retail executable, not newly translated.

Authored C++ uses `localization::Tr("semantic.id")`. The existing build
pipeline substitutes literal macros before compilation: no runtime translator,
allocation, or initialization is introduced. Generated headers alone encode
Windows-1251 bytes as fixed-width octal escapes. Unknown or stale IDs, missing
translations, fuzzy entries, mismatched printf arguments and unencodable text
fail validation.

Fixed-width character fields use `localization::Chars("semantic.id")`.
It produces an ordinary character-array initializer with exactly the catalog's
bytes and no implicit NUL. The declared C++ array still controls storage size;
both compilers reject excess characters. Original padding spaces belong in the
catalog entry. Only the character macros used by a unit or its headers are
emitted. Campaign town names use this form; their fixed-width provenance and
whole-table controls are in `config/retail/buka-localized-tables.json`.

When retail changes a format argument list, `locales/format-variants.json`
records the exact English and Russian signatures for that ID. Unknown IDs,
stale signatures and unnecessary variants fail validation. `HOMM1_RUSSIAN`
selects the corresponding source call at build time; the renderer replaces it
with `1` or `0` before either compiler sees the file. Both compiler views retain
source offsets. Clean exports carry the same manifest and renderer. This is
used by the combat shooting message, whose English suffix argument is absent
from Buka. Other catalog entries still require identical format signatures.

Clang sees a VFS overlay retaining original filenames, byte offsets and lines.
VC6 sees a generated source/header mirror. Catalog edits invalidate compile and
extraction edges; source fingerprints include the pinned target's expanded literals.
Actual CRT format arguments are checked by Clang before localized compilation.
For a Russian matching target, English objects are confined to
`build/ordinary/en`, with independent generated headers. Clean exports carry
the catalogs and portable renderer; `python3 build.py --locale en` or `--locale ru`
selects their language. A nonmatching export build writes to
`build/ordinary/<locale>`, separately from the matching-language output.
The transitional English control passes clean-export verification: all 69
control objects and the linked executable reproduce the matching build
(executable timestamps excluded).

Run the catalog/source check with `python3 -m homm1.graph.localization`, and the
portable catalog/overlay tests with
`python3 -m unittest homm1.graph.test_localization` inside `nix develop .#build`.

Migrated table entries retain exact 1.2 English. Their source commit, old and
new pointer slots, and literal hashes are recorded in
`config/retail/buka-localization.tsv`. The pinned Buka target selects Russian;
nonmatching English objects use `build/ordinary/en`.

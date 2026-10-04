# Static localization

Buka text lives in UTF-8 catalogs, following HoMM2's build-time design.
`locales/messages.def` preserves the exact original English; `locales/ru.po`
uses the semantic ID as `msgctxt` and repeats that English as `msgid`.
Russian text is recovered from the retail executable, not newly translated.

Authored C++ uses `localization::Tr("semantic.id")`. The existing build
pipeline substitutes literal macros before compilation: no runtime translator,
allocation, or initialization is introduced. Generated headers alone encode
Windows-1251 bytes as fixed-width octal escapes. Unknown or stale IDs, missing
translations, fuzzy entries, mismatched printf arguments and unencodable text
fail validation.

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

The 565 migrated table entries retain exact 1.2 English. Their source
commit, old and new pointer slots, and literal hashes are recorded in
`config/retail/buka-localization.tsv`. The transitional 1.2 control explicitly selects English in targets.json; Buka
will select Russian. Nonmatching locale objects must use build/ordinary/<locale>.
Other candidate correspondences remain
in the migration evidence until reviewed.

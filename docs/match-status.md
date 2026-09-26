# Match tracking

Run inside `nix develop .#build`:

```sh
homm1 build
homm1 verify status
homm1 walls inventory
```

[config/match_baseline.tsv](../config/match_baseline.tsv) records banked progress.
The current report is generated from a real build, not maintained in prose.

- `cur_pct`: score at the banked snapshot.
- `best_pct`: best observed score associated with that implementation fingerprint.
- `hist_pct`: historical peak across implementations, indicating known headroom.
- `src_hash`: source fingerprint; `cpp:` denotes a coarser fallback, not proof of a function edit.
- `state=absent`: a retained historical row currently unscored, not silently erased.

The [verifier](../scripts/homm1/verify/verbs.py) separates fresh below-bank
regressions from carried ones, unbanked losses, and hard report failures.
`homm1 verify check` runs the gate; `--strict` also rejects carried
regressions. Use its actual output rather than inferring a verdict from the
aggregate exact count or fuzzy percentage.

The [fingerprinter](../scripts/homm1/verify/fingerprints.py) writes
`build/gen/func_fingerprints.tsv`. Its old `build/clangd/` path is only a seed
for migration, not the active cache location.

`homm1 verify bank` is an explicit ledger-writing operation. Review and stage
the corresponding source snapshot first; do not bank an unexplained mismatch.
A normal build can refresh the README score block without banking the ledger.

An exact score is evidence about the compared body and referents, not proof of
all source identities, callers, or runtime behavior. A historical peak is not a
current-source proof. Keep per-function work state in the derived inventory and
the existing ledgers rather than adding another report to `docs/`.

README headlines and module scores use MAX, computed with the same source-edit
rules as banking. A separate CUR / MAX / HIST line shows current emission,
current-source peak and historical peak. README refresh previews those rules
without changing the ledger. An unedited dip keeps MAX; a genuine source edit
resets MAX to CUR while HIST retains earlier headroom.

`verify status` always reports without failing. `verify check` fails for fresh
source regressions, unbanked missing bodies and hard report failures; `--strict`
also fails on carried regressions. Unedited DIP rows are summarized unless
`--all` is requested. Stale reports and fingerprint caches produce warnings.
Banks refuse hard report failures and unstaged build inputs unless `--dirty`
explicitly accepts the latter with a warning. Renames and unit moves migrate
history by unique retail RVA; absent bodies retain their ledger row. Switching
comparison mode requires an explicit rebase; MAX and HIST are reset to that
mode's current scores.

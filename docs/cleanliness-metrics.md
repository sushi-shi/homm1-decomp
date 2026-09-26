# Cleanliness checks

`homm1 verify check` runs the MAX score gate and fast/normal source/model checks.
The registry in `scripts/homm1/verify/tiers.py` is authoritative. Committed debt
floors live under `config/cleanliness`; generated findings belong in `build`.

Direct gate commands expose their evidence. Updating a debt floor is an explicit
reviewed operation, not a routine response to a failing build. Existing source
gate failures remain visible during the tooling port. Code-first relaxation
changes data-reference scoring; it does not license false code, wrong callees,
contradictory identities or silent gate exceptions.

Run `homm1 test` for tooling changes. The imported `verify selftest` is a separate,
currently incomplete whole-donor validation suite; see the inheritance review.

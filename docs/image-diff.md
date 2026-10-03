# Candidate-image checks

Per-function COFF comparison does not establish that the linked program has
correct placement, references, startup, or behavior.

Inside `nix develop .#build`:

```sh
homm1 link
homm1 verify link-tier
homm1 verify link-tier --census
```

The implementation is [link_tier.py](../scripts/homm1/verify/link_tier.py).
It reads the candidate EXE/map produced under `build/exe/` and checks:

- unresolved externals and pre-link symbol closure;
- candidate versus retail section sizes and missing sections;
- linked bytes of functions scored exact, paired through symbols and the
  link-assigned addresses, with relocation masking.

The section-only view is a size census, not a proof of byte or semantic equality.
A masked address field also does not prove that its target is correct. Inspect
ordered referents and the applicable data/ABI checks when identity matters.

The link tier is opt-in. A regular `homm1 build` does not certify a candidate
image, and an exact object function does not certify all its callers.

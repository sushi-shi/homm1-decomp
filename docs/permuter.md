# Classified permutation experiments

`homm1 permute` searches a complete, credible function only after
`homm1 walls diagnose` identifies a register-allocation/scheduling residue.
The public command also refuses a function whose historical MAX is already
100%. It is an evidence generator, not a substitute for reconstruction.

Two bounded modes are supported:

```sh
homm1 permute state --source <source.cpp> --rva <target-rva> \
  --trials 60 --jobs 4 --state-summary build/permute/probe-head-states.json

homm1 permute variants <source.cpp> <target-rva> \
  --axes-from build/permute/probe-head-axes.json --min-depth 0 --max-depth 2 \
  --state-trials 32 --state-insertion target --jobs 4 \
  --wall-time-seconds 900 -o build/permute/probe-head-manifest.json --run
```

The campaign front end derives and classifies the source-owned population, then
runs the approximation loop directly:

```sh
homm1 permute candidates --output build/permute-candidates.json
homm1 permute campaign --targets 3 --islands 32 --frontier 4 \
  --output build/permute-campaign
```

This is a bounded N-island/M-frontier search, not a claim that fuzzy score proves
source correctness. Each island is a deterministic compiler-state sample crossed
with class-appropriate source shapes. The batch retains the best representative
of the M highest-scoring distinct normalized target states. An agent compares
those states with retail, identifies a repeated source-level mechanism, makes one
defensible source A/B, checks it with `homm1 match <unit>`, and starts another
round. The live inventory is
re-derived for every round; there is no hand-kept queue.

`state` leaves the target body unchanged and inserts deterministic parser-visible
declaration forests either beside it or after the leading directive block.
`--only-trial N` replays an indexed state. `--state-summary` records unique
byte/extent/relocation states even when none is exact. `--retain-best` preserves
the best fuzzy or structural-topology clue and snippet; otherwise sub-100
artifacts are discarded. `--jobs N` compiles disposable sibling sources in
parallel without editing the authored TU.

`variants` forms one explicit product from:

- byte-exact, hand-reviewed axes supplied by `--axes-from`;
- conservative libclang edit trees up to `--max-depth N`;
- optional deterministic TU-state candidates from `--state-trials`.

These are a real Cartesian matrix: each selected source shape is tested in the
baseline compiler state and in every requested TU-state island. Exact-span axis
options may include atomic `extra_edits`, allowing a helper definition and its
call-site rewrite to remain one reviewed choice. The source-tree ceiling defaults
to three mutations; use `--min-depth 0 --max-depth 0` for axes/state only. There
is no regex rewriter or random hill-climber. Use one
manifest containing the complete legal candidate family per site so interactions
are measured rather than laddered.

The syntax-aware families cover value-neutral operand order, independent
assignment order, terminal return-pair inversion, declaration split/merge/hoist,
reviewed inline-helper extraction, cursor read/advance, and identity-safe local
renaming. The declaration forest varies typedef, class, packing, prototype,
calling-convention, and inline-function shapes; it is broader than a flat count
sweep to test whether VC4 front-end state responds to declaration kind and order.

Every compile has a timeout; a batch may also have a total wall-time bound.
Source restoration is guarded by a process lock and exact source bytes; `state`
also rechecks the per-function fingerprint. The first audited exact candidate
normally stops a direct search. Campaigns continue after exact so the M-solution
frontier remains available for pattern extraction.

`variants --jobs N` compiles N disposable sibling copies of the complete TU in
parallel. It does not use a reduced function harness: the real include closure,
declaration population, inline candidates, and sibling functions remain present.
Each disposable object and the retail object are canonicalized through the same
same-function jump-table and compiler-private-symbol transform as `homm1 build`
before scoring; otherwise raw `$L...` labels can hide an exact result. Parallel
results are still scored and audited one target at a time, and only the authored, probe-free source can bank MAX. A target-only harness changes compiler context and is only a non-authoritative
prefilter, not a replacement for the complete-TU comparison.
Exact closure requires all of:

1. unrounded objdiff score exactly 100%;
2. target extent equal to retail;
3. completely decoded ordered relocation streams;
4. equal ordered relocation offsets/types/identities/addends in the active
   comparison mode. Data targets are normalized in code mode, so this is not
   proof of strict data-reference identity.

Instruction/branch/return topology is recorded as a separate ranking signal for
sub-100 candidates. It is only a clue: it can select a more structurally useful
candidate for inspection, but it cannot satisfy the exact-closure gate.

A source-only exact candidate is written as `exact.cpp` for review. A candidate
containing disposable TU state is written as `exact-disposable.cpp`; never apply
its probes. `permute state --record-max` can retain an audited exact
compiler-state peak for the unchanged function: unrounded score 100, exact size,
complete ordered relocations, restored source hash, a unique existing bank row,
and the same comparison mode are required. Only MAX/HIST change; banked CUR,
source hash and other rows remain unchanged. Sub-100 trials never update the bank.
Remove probes and run `homm1 build` after applying ordinary source or tooling changes.

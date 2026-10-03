# vc4trace — tracing and replaying the pinned VC4 C2.EXE

Research tooling. It is not part of the `homm1` CLI, has no tests and does not
touch `src/`: compiles go to `build/research/vc4trace/`. The findings are in
`docs/patterns/vc4-sortnode-is-a-replayable-function-of-handles.md`.

## Why no debugger

The development host sets `kernel.yama.ptrace_scope = 2`, so nothing can ptrace:

- `winedbg PROG` cannot launch anything, not even notepad. The debuggee dies with
  `server_init_process_done: Assertion '!status' failed`.
- `winedbg --gdb PID` (attach) fails with `Can't attach process ...: error 5`.
- `gdb -p` on the wine process is refused.
- Classic (non-wow64) wine is not in the binary cache.

`tracer.py` replaces the debugger. It writes a copy of C2.EXE with an added `.trc`
section. Each tracepoint becomes a 5-byte `jmp` into a stub that runs `pushad`/`pushfd`,
calls `printf(fmt, args...)` through the MSVCRT40 import, restores the registers,
replays the displaced instructions and jumps back. Displaced `jmp`, `jcc` and `call`
instructions are re-targeted.

## Commands

Run everything inside `nix develop .#build` from the worktree root.

    # one compile split into C1XX + C2 with the CL /Bd flags of the unit's profile,
    # C2 patched with the sortnode tracer (sort.spec); log + obj + IL in build/research/vc4trace/tu/
    python3 -m homm1.research.vc4trace.trace SOURCE/ARMY
    python3 -m homm1.research.vc4trace.trace --source X.cpp --mode od|odeh|o2|o2inline [--spec ra.spec]

    # patch C2.EXE with your own tracepoints
    python3 -m homm1.research.vc4trace.tracer SPEC OUT.EXE
    # SPEC lines:  ADDR | "printf format" | arg arg ...   [@limit N] [@post]
    #   args: eax..edi, esp, NUM, d[X+n] w[X+n] sw[X+n] b[X+n] sb[X+n] (nestable)
    #   @post: print after the displaced instructions, just before their final ret

    # sortnode replay
    python3 -m homm1.research.vc4trace.sortsim check TRACE              # must be exact
    python3 -m homm1.research.vc4trace.sortsim diff TRACE_A TRACE_B +N@LO  # predict B from A

    # planners
    python3 -m homm1.research.vc4trace.sortplan TRACE ILPREFIX FUNC --od   # declaration-order classes
    python3 -m homm1.research.vc4trace.fnshift TRACE ILPREFIX FUNC LO      # class per handle shift
    python3 -m homm1.research.vc4trace.solver SOURCE/UNIT [--funcs A,B] [--probes 64] [--verify]

    # listings and retail distance
    python3 -m homm1.research.vc4trace.objdis OBJ [FUNC]
    python3 -m homm1.research.vc4trace.objdis --distance SOURCE/UNIT FUNC [OBJ]
    python3 -m homm1.research.vc4trace.pedis VA [COUNT]                    # C2.EXE disassembly

`ILPREFIX` is `build/research/vc4trace/tu/il/<stem>`. Its `sy` stream holds the
locals per function in `/Od` slot order, and its `gl` stream holds the global and
function handles.

## Tracepoint addresses (pinned C2.EXE)

| address | what |
|---|---|
| `0x40846e` | sortnode weight(node=ecx, L=edx, R=[esp+4]) |
| `0x408daf` | walker after the weight (node ebx, weight eax, L ebp, R edi) |
| `0x408588` | symbol leaf (C1 handle `[eax+0x24]`) |
| `0x413ded` | exchange (node ecx, L edx, R [esp+4]): swap if R > L; comparisons mirror via table `0x481c0c` |
| `0x413e5c` | reassociation fires (`(a op b) op c -> (a op c) op b`) |
| `0x47576f` | `/Od` tree root, one per statement |
| `0x408b61` / `0x408bc4` / `0x408bec` | `/O2` tree roots, one per walk pass |
| `[[[0x48173c]+4]+0x24]` | current function's C1 handle |
| `0x40cbe1` | `/O2` select: node `[esp+0x14]` (id `+0x18`, weight `+0x16`), register code ecx |
| `0x484828` | op flag words (class bits 0..1, 8 commutative, 0x10 comparison, 4 associative, 0x2000 binary) |

## Solver

`solver` traces the unit once. It then compiles probes with N typedefs at the top
of the TU and measures each function's distance to retail. The replay predicts each
function's outcome class at every probe, so every probe labels classes with
distances. The solver then searches with the replay alone over:

- local declaration orders that keep the `/Od` slots (sortplan representatives);
- handles inserted between functions (stand-ins for control-flow spellings or
  declarations);
- a whole-TU shift.

States whose class no compile has measured are compiled and added to the tables,
over at most `--rounds` compiles per tier. Candidates are ranked by authenticity:
tier 1 is local order only, tier 2 adds between-function insertions with
control-flow spelling hints, and tier 3 adds the whole-TU shift. `--verify`
compiles each tier's result. The realisation is in scratch only and uses typedef
runs as stand-ins for the authentic edits; nothing is written back to `src/`.

"""homm1.research.vc4trace - trace and replay the pinned VC4 C2.EXE.

ptrace is unavailable on the development host (yama ptrace_scope=2), so
tracepoints are compiled into a patched copy of C2.EXE (tracer.py).  The
sortnode replay (sortsim.py) and the planners built on it predict operand
order from C1 symbol handles; see README.md and
docs/patterns/vc4-sortnode-is-a-replayable-function-of-handles.md.
"""

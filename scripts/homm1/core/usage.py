"""Append-only usage events shared by every tooling entry point.

Logging is best effort and never changes a command's result. Start events survive
crashes/termination; only a matching finish event establishes completion.
"""
from __future__ import annotations

from contextvars import ContextVar
from datetime import datetime, timezone
from functools import wraps
import fcntl
import json
import os
import shlex
import sys
import time
import uuid

_active: ContextVar[str | None] = ContextVar("homm1_usage", default=None)
_installed = False
_warned = False


def _write(event: dict) -> None:
    global _warned
    try:
        from homm1.core.paths import BUILD
        path = BUILD / "homm1_usage.jsonl"
        path.parent.mkdir(parents=True, exist_ok=True)
        row = {"schema": 1, "time": datetime.now(timezone.utc).isoformat(),
               "pid": os.getpid(), "ppid": os.getppid(), **event}
        line = json.dumps(row, ensure_ascii=True) + "\n"
        with path.open("a", encoding="utf-8") as handle:
            fcntl.flock(handle, fcntl.LOCK_EX)
            handle.write(line)
            handle.flush()
    except Exception as exc:
        if not _warned:
            _warned = True
            try:
                print(f"[usage] cannot append usage log: {exc}", file=sys.stderr)
            except Exception:
                pass


def _audit(event: str, args: tuple) -> None:
    # Python emits this for run/check_output/Popen alike. Do not log the
    # environment (which may contain credentials), stdin or file contents.
    if event != "subprocess.Popen" or _active.get() is None:
        return
    try:
        executable, argv, cwd, _env = args
        _write({"event": "subprocess", "parent_id": _active.get(),
                "executable": os.fsdecode(executable),
                "argv": ([os.fsdecode(a) for a in argv]
                         if isinstance(argv, (list, tuple)) else os.fsdecode(argv)),
                "cwd": os.fsdecode(cwd) if cwd is not None else os.getcwd()})
    except Exception:
        pass  # audit hooks must never prevent a subprocess from launching


def logged(fn):
    """Record a main(argv=None) entry, including nested and batch invocations."""
    spec = fn.__globals__.get("__spec__")
    module = spec.name if spec is not None else fn.__module__

    @wraps(fn)
    def invoke(*args, **kwargs):
        global _installed
        if not _installed:
            sys.addaudithook(_audit)
            _installed = True
        argv = args[0] if args else kwargs.get("argv")
        argv = list(sys.argv[1:] if argv is None else argv)
        prefix = ["homm1"] if module == "homm1.cli" else ["python3", "-m", module]
        invocation = uuid.uuid4().hex
        parent = _active.get()
        started = time.monotonic()
        _write({"event": "start", "id": invocation, "parent_id": parent,
                "module": module, "argv": argv, "cwd": os.getcwd(),
                "command": shlex.join([*prefix, *argv])})
        token = _active.set(invocation)
        rc, error = 1, None
        try:
            result = fn(*args, **kwargs)
            rc = 0 if result is None else int(result)
            return result
        except BaseException as exc:
            error = {"type": type(exc).__name__, "message": str(exc)}
            if isinstance(exc, SystemExit):
                rc = (0 if exc.code is None else exc.code
                      if isinstance(exc.code, int) else 1)
            elif isinstance(exc, KeyboardInterrupt):
                rc = 130
            raise
        finally:
            _active.reset(token)
            _write({"event": "finish", "id": invocation, "parent_id": parent,
                    "module": module, "exit_code": rc,
                    "duration_s": round(time.monotonic() - started, 6),
                    "error": error})
    return invoke

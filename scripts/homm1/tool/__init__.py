"""homm1.tool - every external program this tree runs.

Rules of the layer:
  * a tool module RUNS a program and returns artifacts; it never interprets
    them (reading an output is inputs/' business, policy is the caller's);
  * only tool/ spawns processes - nothing else imports subprocess;
  * era tools (cl, link, rc) share the wine plumbing in wine.py; callers see
    "the compiler", never wine;
  * each module is one library function + a __main__ shim, so ninja and
    in-process python callers share one surface.
"""


class ToolError(RuntimeError):
    """A driven tool failed; the message carries the output tail."""

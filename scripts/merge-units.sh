#!/bin/sh
# Shared module entry point preserves usage logging outside a Nix shell too.
if command -v python3 >/dev/null 2>&1; then
    export PYTHONPATH="$PWD/scripts${PYTHONPATH:+:$PYTHONPATH}"
    exec python3 -m homm1.tool.merge_units "$1" "$2" "$3"
fi
exec git merge-file -L ours -L base -L theirs "$2" "$1" "$3"

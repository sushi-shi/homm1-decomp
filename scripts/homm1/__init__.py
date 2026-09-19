"""homm1 - the HoMM1 reconstruction toolchain (2026-08 rebuild).

Data flow, one direction only:

    src/+include/ labels ----extract----> claims
    config/retail/ censuses+providers --> rows
    claims x rows -------model----------> bindings (+ violations)
    bindings -----------delink----------> target objs -> objdiff report
    bindings+objs+report --verify-------> gates;  --query--> answers

Layers (imports point strictly downward):

    core/    semantically blind primitives: formats, bytes, tool driving
    inputs/  file -> typed records (parse-only; PRIVATE to the model)
    model    the one join; every downstream consumer takes a Model
    extract/ delink/ verify/ query/ bank/ cli

The old tree lives frozen at scripts/homm1-old/ as reference material.
"""

# Validation

Date: 2026-10-23. TinyCC: 0.9.28rc (`PIN`). llzk-opt: LLZK 2.1.2 with
LLVM 20.1.8. Host executable: `build/langc` at 5a23582 with the slice
Z8d change.

`make check` passes: it builds `build/langc`, `build/langc-256`,
`build/fronttool` and `build/llzkprint`, and runs `test/gate.sh`,
`test/review.sh` and `build/fronttool`. The gate checks 29 examples,
12 parse refusals, 123 check refusals, 33 `langc-256` rows, 205 evals,
6 LLZK rows, 9 LLZK refusals, 29 IR lowerings, 32 Wasm modules and
5008 differential cases with 0 mismatches. It has 0 failures. The test
runner reported 351.3 seconds.

`make zk-check` passes: `llzk-opt` accepts 12 modules and rejects 2
mutants, and it prints `zk-check: 12 accepted, 2 rejected`. The run
took 1.3 seconds.

> Write here: one paragraph for each slice, in this form.

The Z8 review adds 15 cases in `test/gate.sh`. The 6 golden rows
(`llzk: 6 checked`) give the words of `matmul2Words`, `residual2Words`,
`seq2Words`, `zeroWords` and `andWords` (`examples/llzk.lang`) and
`test/llzk/decimal.words` to `build/llzkprint`. Each module is equal to
`test/llzk/CASE.mlir`. The decimal case has the consts 0, 1, r - 1 and
2^32. The 9 refusal rows (`llzk refusals: 9 checked`) cover lookup, a
bad version, a bad field id, extra words, a short block, a forward value
number, an out value in an add, an output written twice and a const
equal to r. `make zk-check` adds 14 `llzk-opt` runs. It accepts the 6
golden modules and the 6 fresh printer outputs. It rejects
`test/llzk/reject/matmul2-m1-type.mlir` (a type mismatch) and
`test/llzk/reject/matmul2-m2-undef.mlir` (an undefined value). Three
mutants of `src/llzk.c` (no eq line, the add operands swapped, a result
number plus 1) each make the golden rows fail.

The Z8d staged review adds 3 recipe checks in `test/zk-check-runner.sh`,
run by `make check` without LLVM. A successful mock run must pass.
A word evaluator or a words copy that leaves usable words but returns
failure must make `zk-check` fail.

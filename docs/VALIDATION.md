# Validation

Date: 2026-10-24. TinyCC: 0.9.28rc (`PIN`). llzk-opt: LLZK 2.1.2 with
LLVM 20.1.8. halo2_proofs: 0.3.0 (`test/halo2/Cargo.lock`). Host
executable: `build/langc` at 2f53fd4 with the slice Z9d change.

`make check` passes: it builds `build/langc`, `build/langc-256`,
`build/fronttool`, `build/llzkprint`, `build/halo2print` and
`build/halo2-reader`, and runs `build/halo2-reader`, `test/gate.sh`,
`test/review.sh`, `test/zk-check-runner.sh` and `build/fronttool`. The
reader prints `halo2 reader: 5 checked` and
`halo2 writer refusals: 6 checked`. The gate checks 30 examples, 12
parse refusals, 128 check refusals, 34 `langc-256` rows, 213 evals,
6 LLZK rows, 9 LLZK refusals, 5 Halo2 rows, 9 Halo2 refusals, 30 IR
lowerings, 33 Wasm modules and 5016 differential cases with 0
mismatches. It has 0 failures. The runner prints
`zk-check runner: 7 checked`. The run took 694 seconds in the sandbox
(420 seconds outside it).

`make zk-check` passes: `llzk-opt` accepts 12 modules and rejects 2
mutants, and it prints `zk-check: 12 accepted, 2 rejected`. The run
took 1.3 seconds.

`make halo2-check` passes outside the sandbox: `cargo test --offline
--locked -j 2` passes for the 5 crates, and it prints
`halo2-check: 5 accepted, 9 rejected`. The first run took 20.1 seconds
with a peak RSS of 256 MiB, because it builds the dependencies in
`build/halo2/target` (434 MB). A second run took 5 seconds. Run it
outside the sandbox, with the dependencies in the cargo cache: it uses
`--offline` and fetches nothing. `CARGO_TEST` is the test command
(default `testledger -- cargo test`).

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

The Z9 review adds 14 cases in `test/gate.sh`. The 5 golden rows
(`halo2: 5 checked`) give the words of `matmul2Halo2`,
`residual2Halo2`, `seq2Halo2`, `zeroHalo2` and `wrapHalo2`
(`examples/halo2.lang`) to `build/halo2print`. Each printed crate is
equal to `test/halo2/CASE/Cargo.toml` and `test/halo2/CASE/src/lib.rs`.
The 9 refusal rows (`halo2 refusals: 9 checked`) cover a field id that
is not 2, a matrix index that is not below the matrix count, a matrix
map that does not give the weight count, an input count that is not the
circuit input count, a last stage that does not give the output count,
an element that is not below r, a word after the witness block, a bad
name and a directory that the printer cannot make. Each refusal makes
no directory. `build/halo2-reader` (`halo2 reader: 5 checked`) checks 5
stage shapes of the word reader. Its 6 writer rows
(`halo2 writer refusals: 6 checked`) make the write fail at 6 different
points. Each row checks that the existing crate files do not change and
that no stage directory stays.

The Z9d review adds 4 recipe checks for `halo2-check` in
`test/zk-check-runner.sh` (`zk-check runner: 7 checked`), with a mock
printer and a mock test command. The mock test command fails if the
lock file is not a copy of `test/halo2/Cargo.lock` or if
`--offline --locked` is not in its arguments. A mock run must print
`halo2-check: 5 accepted, 9 rejected`. A failed word evaluation, a
failed crate print and a failed test run must make `halo2-check` fail.
Each row that sets a failure variable runs in a subshell (flag 85), so
the next row does not see the variable. A mutant that ignores the
status of the words copy in `zk-check` makes the runner fail. Before
flag 85 the runner passed with this mutant.

# Validation

Date: 2026-10-24. TinyCC: 0.9.28rc (`PIN`). llzk-opt: LLZK 2.1.2 with
LLVM 20.1.8. halo2_proofs: 0.3.0 (`test/halo2/Cargo.lock`). Plonky3:
rev 5fcf8d7c (`test/plonky3/Cargo.lock`). Host executable:
`build/langc` at 24d67eb with the slice Z10d change.

`make check` passes: it builds `build/langc`, `build/langc-256`,
`build/fronttool`, `build/llzkprint`, `build/halo2print`,
`build/halo2-reader`, `build/plonky3print` and `build/plonky3-reader`,
and runs `build/halo2-reader`, `build/plonky3-reader`, `test/gate.sh`,
`test/review.sh`, `test/zk-check-runner.sh` and `build/fronttool`. The
Halo2 reader prints `halo2 reader: 5 checked` and
`halo2 writer refusals: 6 checked`. The Plonky3 reader prints
`plonky3 reader: 6 checked`, `plonky3 writer refusals: 6 checked` and
`plonky3 render: 9 checked`. The gate checks 31 examples, 12 parse
refusals, 132 check refusals, 35 `langc-256` rows, 220 evals, 6 LLZK
rows, 9 LLZK refusals, 5 Halo2 rows, 9 Halo2 refusals, 5 Plonky3 rows,
17 Plonky3 refusals, 31 IR lowerings, 34 Wasm modules and 5025
differential cases with 0 mismatches. It has 0 failures. The runner
prints `zk-check runner: 11 checked`. The run took 719 seconds in the
sandbox.

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

`make plonky3-check` passes outside the sandbox: `cargo test --offline
--locked -j 2` passes for the 5 crates, and it prints
`plonky3-check: 5 accepted, 11 rejected`. The first run took 46.4
seconds with a peak RSS of 291 MiB, because it builds the dependencies
in `build/plonky3/target` (529 MB). It ran at the same time as the
baseline gate. A second run took 12.0 seconds with a peak RSS of
143 MiB. The run fetches nothing. The lock file `test/plonky3/Cargo.lock`
is equal, byte for byte, to the lock that `cargo metadata --offline`
writes from the Z10a lock. The tests do not use `--release`, because the
debug constraint check runs only with debug assertions.

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

The Z10 review adds 22 cases in `test/gate.sh`. The 5 golden rows
(`plonky3: 5 checked`) give the words of `matmul2Plonky3`,
`residual2Plonky3`, `seq2Plonky3`, `zeroPlonky3` and `wrapPlonky3`
(`examples/plonky3.lang`) to `build/plonky3print`. Each printed crate is
equal to `test/plonky3/CASE/Cargo.toml` and
`test/plonky3/CASE/src/lib.rs`. The 17 refusal rows
(`plonky3 refusals: 17 checked`) cover a field id that is not 1, a
witness header that is not 1 1 1, a witness stage count that is not the
stage count, a stage tag that is not known, a lookup stage, a source
that is not below its stage number, a matrix index that is not below the
matrix count, a matrix rank that is not 2, a matmul stage rank that is
not 1, 2 matmul shape errors, a stage aux word that is not 0, an element
that is not below p, a word after the witness block, a residual shape
error, a bad name and a directory that the printer cannot make. The
first 16 refusals make no directory. `build/plonky3-reader` checks 5
stage shapes and 1 failed read of the word reader
(`plonky3 reader: 6 checked`). Its 6 writer rows
(`plonky3 writer refusals: 6 checked`) make the write fail at 6
different points. Each row checks that the existing crate files do not
change and that no stage directory stays. Its 9 render rows
(`plonky3 render: 9 checked`) print the crates of 7 edge entries, for
example an empty trace, a zero stage and 0 inputs. Each row checks a
line that must be in the crate, and 8 rows also check a line that must
not be in it. These entries gave crate tests that failed: an empty
trace, the `assert_zero` panic text and a mutation that changes nothing.
Slice Z10b adds `examples/plonky3.lang` and 4 check refusal files
(`test/check/plonky3-*.lang`). With them the gate checks 1 more example,
4 more check refusals, 1 more `langc-256` row, 7 more evals, 1 more IR
lowering, 1 more Wasm module and 9 more differential cases than in slice
Z9d.

The Z10d review adds 4 recipe checks for `plonky3-check` in
`test/zk-check-runner.sh` (`zk-check runner: 11 checked`), with a mock
printer and a mock test command. The mock test command fails if the
lock file is not a copy of `test/plonky3/Cargo.lock` or if
`--offline --locked` is not in its arguments. A mock run must print
`plonky3-check: 5 accepted, 11 rejected`. A failed word evaluation, a
failed crate print and a failed test run must make `plonky3-check` fail.
Each row that sets a failure variable runs in a subshell (flag 85). Each
of 4 recipe mutants makes the runner fail: a rejected count plus 1, no
copy of the lock file, a recipe that ignores the status of `langc eval`
and a final check that always passes.

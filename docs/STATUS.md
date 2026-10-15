# zkml-lang status

The build implements slice Z0 (the tcc-wasm kit with the formers F1 to
F15) and slice Z1a of `docs/PLAN.md`: the interval C1 without faces, and
PathP and Path (C2). It does not complete slice Z1. Faces, partial
elements and systems come in slice Z1b. Slices Z2 to Z11 are not started.

## Implemented

- F1 to F15 from the tcc-wasm kit. `formers/tcc-wasm.md` gives the status
  of each former.
- C1 interval: the pretype `I` (sort `SSet`), `i0`, `i1`, `~`, `/\` and
  `\/`. The checker compares interval terms by a De Morgan normal form.
- C2 PathP and Path: `PathP A a b`, `Path A a b` (PathP over a constant
  line), `<i> t`, `p @ r`, the endpoint rules and path eta.
- Path forms of `refl`, `symm` and `cong`, and `funExt`. The laws in
  `examples/paths.lang` hold by `refl`.
- The refusal `REFUSE_RUNTIME_PATH` in the lowering.

## Remaining work

1. Slice Z1b: faces as interval terms, partial elements and systems. It
   needs the Z1a commit first.
2. Slice Z2: `transp` and `hcomp` (C3). Then `trans` and `transport` take
   a path.
3. Slices Z3 to Z11 as `docs/PLAN.md` gives them. Slice Z6 needs a ruling
   on item O5 (attention shapes) first.
4. The LLZK target needs the LLZK toolchain. llzk-lib needs LLVM 23.1.3,
   and the toolchain is not installed yet.

## Known limits

- The interval normal form holds at most 16 meets of at most 8 literals.
  A larger term stays unreduced, so the checker can fail to equate two
  large terms that are equal. Cause: fixed-size arrays in
  `src/front/eval.c`.
- `<i> t` and `funExt h` do not infer a type. They need a declared
  `PathP` type. Cause: the checker cannot find the line from the body.
- `refl` and `cong` on a path need a constant line. Cause: a dependent
  line needs `transp` (slice Z2).
- `trans` and `transport` take an `Eq` proof only. Cause: path
  composition needs `hcomp` (slice Z2).
- An interval or path value cannot exist at run time
  (`REFUSE_RUNTIME_PATH`). Cause: the IR has no interval and no path.
- The kit limits stay. `docs/host/CAPABILITY.md` gives them.

## Internal boundaries

`src/front/lexer.c` and `src/front/parser.c` read the path syntax. `<i> t`,
`p @ r`, `~`, `/\` and `\/` become applications of builtin heads that no
identifier can spell, so the parser adds no new term kind.
`test/parse/expect.txt` gives the parse refusals.

`src/front/check.c` holds the builtin rows for C1 and C2, the pretype
mode, the endpoint checks and the path forms of `refl`, `symm`, `cong`
and `funExt`. `test/check/expect.txt` gives the refusal code of each
reject case (`TYPE_ENDPOINT`, `TYPE_PRETYPE`, `TYPE_MISMATCH`).

`src/front/eval.c` holds the interval normal form, the reduction of
`p @ r` (the value keeps the ends of the type) and path eta in the
conversion check. Neutral interval terms participate in the same algebra
as interval variables. The printer emits source syntax for interval and
path normal forms. `test/fronttool.c` checks these round trips, neutral
interval laws and type-directed checking of the argument to `funExt`.

`src/front/lower.c` refuses interval and path values at run time. A
program cannot use `I`, `PathP` or `Path` values in an entry. The gate
checks `test/ir/path.lang`.

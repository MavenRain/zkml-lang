# zkml-lang status

The build implements slice Z0 (the tcc-wasm kit with the formers F1 to
F15) and slice Z1 of `docs/PLAN.md`: the interval C1 with faces, partial
elements and systems, and PathP and Path (C2). It also implements
slice Z2 (C3): `transp`, `hcomp`, `comp` and `fill`, and transport and
`trans` over a path, slice Z3 (C4): higher inductive types in the
domain, path constructor values, the formal `hcomp` and the `fold`
eliminator, and slice Z4 (C5): indexed sorts with the motive fold (part
Z4a) and mutual blocks (part Z4b). It implements part Z5a of slice Z5:
`Nat` induction, the `natAdd` neutral rules, `natMax`, `natIsSet` and
record eta. It implements part Z5b: the `natMax` neutral rules and, in
the domain, `Resource`, `Inst`, `Layer`, the h-level library and the
category laws of `Layer`. It implements part Z5c: `isSetHom`, the
category `LayerCat` and the functor `denote`. Slice Z5 is DONE. It
implements part Z6a: the `List` motive fold (C7), part Z6b: the
split witness of `instSeqSat` (flag 16), and part Z6c: the tensor
library (`isSetList`, `isSetTensor`, `zeroT`, `mapT`, `zipWithT`,
`sumT` and the stage round trip). Slice Z6 is in progress. It
implements part Z7a: an entry with a `List Nat` result gives a words
block (flag 56 (a)), part Z7b: the BabyBear trace and its witness
words (flag 62 (a)), and part Z7c: the BN254 trace and its witness
words. It implements part Z7d: the witness format in `SPEC.md`.
Slices Z7 to Z9 are DONE. Slices Z10 and Z11 are not started.

## Implemented

- F1 to F15 from the tcc-wasm kit. `formers/tcc-wasm.md` gives the status
  of each former.
- C1 interval: the pretype `I` (sort `SSet`), `i0`, `i1`, `~`, `/\` and
  `\/`. The checker compares interval terms by a De Morgan normal form.
- C2 PathP and Path: `PathP A a b`, `Path A a b` (PathP over a constant
  line), `<i> t`, `p @ r`, the endpoint rules and path eta.
- C1 faces, partial elements and systems: `(r = 0)`, `(r = 1)`,
  `Partial phi A`, `[ f |-> u , ... ]` and `fromPartial`. The checker
  examines the cover and the overlaps of each system.
  `examples/systems.lang` gives the accept cases.
- Path forms of `refl`, `symm` and `cong`, and `funExt`. The laws in
  `examples/paths.lang` hold by `refl`.
- C3 part Z2a: `transp A r u` (the line, its constancy where `r` holds,
  and the reductions along Pi, Sigma, Prod and the kit data types), and
  `transport P p u` over a path. `examples/transp.lang` derives `J` and
  its computation rule.
- C3 part Z2b-1: `hcomp side u0` (`phi` from the faces, the faces in the
  outer context, the base condition, and the reductions where `phi` holds
  and at Pi, Prod, PathP and the constants of the kit data types).
  `examples/hcomp.lang` gives the accept cases.
- C3 part Z2b-2: `comp A side u0` and `fill A side u0 r` (the `hcomp`
  forms over a constant line, the CCHM forms over a varying line),
  `hcomp` at a Sigma type, `transp` along a line of `PathP` types, and
  `trans` over a path. `examples/kan.lang` gives the accept cases and the
  laws `rUnit`, `lUnit` and `assoc`.
- C4 part Z3a: higher inductive types in `domain/domain.lang` (`Circle`,
  `Trunc A` and `Quot A R`; path constructors of dimension 1 and 2 with
  point-term ends and uniform recursion), path constructor values with
  their end rules, the formal `hcomp` at a higher inductive type, and
  `transp` along a constant line. A program cannot declare one
  (`REFUSE_DATA`). Part Z3b adds the `fold` eliminator, with one case
  for each constructor and its laws by `refl`; Z3 is done.
  `examples/hit.lang` gives the accept cases.
- C5 part Z4a: indexed sorts in `domain/domain.lang` (`Circuit (a b : Shape)`
  with the point constructors `circId`, `circSeq`, `circOp` and
  `circResidual` and the path constructors `circIdl`, `circIdr`,
  `circAssoc` and `circSquash`), the
  `fold` eliminator with a motive over the indices, and cases over the
  fields only. `examples/qiit.lang` gives the accept cases and the laws
  of `circMap` and `circFlip` by `refl`.
- C5 part Z4b: mutual blocks (`family Rose := ... and Kids := ...` in
  `domain/domain.lang`) and the fold over a block, with one motive for
  each sort. `examples/rose.lang` gives the accept cases and the laws of
  `roseMap`, `kidsMap` and `kidsMap2` by `refl`. A fold over a block at
  run time fails with `REFUSE_RUNTIME_TREE`. Z4 is done.
- Z5 part Z5a: the motive form of the `Nat` fold (induction), the
  `natAdd` neutral rules R1 to R3, `natMax`, `natIsSet` and record eta
  (FORMERS F6 and C6). `examples/nat.lang` gives a `Nat`-valued fold
  out of `Circuit` (`circCount`). Part Z5b moved `natAddAssoc` (by
  induction) to the domain.
- Z5 part Z5b: the `natMax` neutral rules (FORMERS C6, flag 12). In
  `domain/domain.lang`: `natAddAssoc` and `natMaxAssoc` by induction;
  the `Resource` monoid (`zeroR`, `resAdd`, `resUnitL` and `resUnitR`
  by `refl`, `resAssoc`), the `Monoid` record and `ResourceMonoid`;
  `Tensor`; the backend record `Inst` and the record `Layer I inS outS`
  (circuit, denotation, soundness, completeness and resources); `idL`
  and `layerSeq` with `soundnessSeq`, `completenessSeq` and
  `resourcesSeq`. The h-level library gives `isSet`, `isProp`,
  `isPropPi`, `isPropPathInSet`, `isPropTrunc` and `isPropPathP`.
  `layerExt` gives a Path in `Layer` from a Path of the circuits, a Path
  of the denotations and a Path of the resources, because the soundness
  and completeness fields are propositions. The category laws
  `layerIdl`, `layerIdr` and `layerAssoc` are `layerExt` proofs, not
  `refl` (flag 8). `examples/resource.lang` and `examples/layer.lang`
  give the accept cases.
- Z5 part Z5c: in `domain/domain.lang`, the h-level lemmas `isSetPi`,
  `isSetResource`, `isPropIsSet` and `isPropSquareP` (a square of
  propositions has a filler for any four sides). `layerExt2` gives a
  square in `Layer` from squares of the circuits, the denotations and
  the resources. `isSetHom` (N4) shows that `Layer I a b` is a set. The
  record `Category` and `LayerCat I` give the category of layers.
  `catSeq` takes the order of `∘` (`g` after `f`), not the diagrammatic
  order (flag 7, `SPEC.md` O2, RULED 2026-10-16). The record `TypeFunctor C` and
  `denote I ws` give the denotation functor: `Tensor (instF I)` on the
  shapes and `layerDen L ws` on the layers. The identity and
  composition laws hold by `refl`. `examples/category.lang` gives the
  accept cases. Six check rows give the refusals: `cat-seq-order`,
  `cat-idl-swap`, `tfun-seq-order`, `isset-hom-square`, `denote-hom`
  and `isprop-isset-side`.
- The refusal `REFUSE_RUNTIME_PATH` in the lowering.
- Part Z7a (flag 56 (a)): an entry with a `List Nat` result is a words
  entry. The lowering makes a words block [n, w0 .. w(n-1)], with one
  word for each element, and the entry returns the address of the
  block. `langc abi` gives the line `NAME words` for a words entry. The
  Wasm module exports memory 0 as `$memory`, and `test/run-wasm.mjs`
  reads the block from this export. `langc eval` and
  `test/run-wasm.mjs` print the n words with one space between them,
  or `nil` for the empty list. `examples/witness.lang` gives the accept
  cases. The gate compares `langc eval` and the Wasm module on each
  words entry of the examples.
- Flag 61 (RULED (a) 2026-10-21): the name `$memory` has a `$`, and a
  guest name cannot have a `$` (`src/front/lexer.c`). Thus no entry
  export has the name of the memory export. With the name `memory`, an
  entry named `memory` gave two exports with one name, and the module
  did not validate.
- Part Z7b (flag 62 (a)): the domain has the BabyBear trace and its
  witness words. `bbTrace` gives the stage words of each op and each
  residual of a circuit, in run order, as a `List (List Nat)`.
  `stageWords` gives the words of one stage: the rank, the dims, the
  aux word 0, then the entries in row-major order. `bbWitnessWords`
  gives the three header words, the matrix count, the words of each
  matrix, the stage count, then the words of each stage.
  `examples/witness.lang` gives the words entries `bbWitQuant`,
  `bbWitResidual`, `bbWitSeq`, `bbWitWide`, `bbWitMatMul` and
  `bbWitLookup`, and the law `bbTraceLast`: the last stage of the trace
  is the stage of the `bbRun` result. The check row `trace-order`
  refuses a law that puts the stages in the reverse order.
  `test/grid.awk` gives the BabyBear p - 1 and p values to a `Nat`
  parameter of an entry with no loop.
- Flag 62 (RULED (a) 2026-10-21): `bbTraceK` folds the circuit in
  continuation-passing style. An op or a residual puts its stage words
  in front of the words that the continuation gives. With this form,
  the `idl`, `idr` and `assoc` cases of the fold hold by beta and eta.
- Flag 63 (RULED (a) 2026-10-21): each words entry of
  `examples/witness.lang` has a loop in `langc ir`. Thus the grid gives
  it only small values, and the differential does not run a `bbWit`
  entry at p - 1 or p. The eval row `bbWitResidual 2013265920 1` gives
  the p - 1 case in `langc eval` only. A gate row makes sure that the
  grid gives p - 1 and p to an entry with no loop.
- Part Z7c: the domain has the BN254 trace and its witness words.
  `frTrace` gives the stage words of each op and each residual of a
  circuit over `Fr`, in run order, as a `List (List Nat)`. `frWords`
  gives the 8 limbs of each element, low limb first. `frWitnessWords`
  gives the header words 1, 2 and 8 (the version, the field id 2 for
  BN254 and 8 words for each element), the matrix count, the words of
  each matrix, the stage count, then the words of each stage.
  `examples/witness.lang` gives the words entries `frWitLookup`,
  `frWitResidual` and `frWitWrap`, and the law `frTraceLast`: the last
  stage of the trace is the stage of the `frRun` result. `frWitWrap`
  has no parameter, and its input is r - 1 and 2^32 - 1. Thus the
  differential runs these boundary values one time. Size: the
  `frWit` entries use 102 to 558 locals (the limit is 50000) and 4958
  to 16616 code bytes. The largest `bbWit` entry uses 61 locals and
  1734 code bytes. The module `witness.wasm` has 40066 bytes. Thus
  slice Z7 does not need a `frMul` jet.
- Part Z7d: `SPEC.md` section 7 (`Witness format`) gives the witness
  words, and item O11 gives the rulings of flags 56 to 63.
  `probe/CAPABILITY.md` section 5 gives the Wasm result path.
  `docs/PLAN.md` says that the field arithmetic is guest code (flag 57
  (a)).
- The refusal `REFUSE_LOCALS` in the lowering (HOST-LIMIT): an entry
  that needs more than `LOWER_LOCALS_MAX` (50000) locals. 50000 is the
  V8 limit for the locals of one Wasm function. The count includes the
  3 scratch locals of the Wasm target. `test/ir/locals.lang` gives the
  cases.

## Remaining work

1. Slices Z6 to Z11 as `docs/PLAN.md` gives them. Item O5 (attention
   shapes) is RULED. Parts Z6a to Z6e are DONE. Part Z6f session 1 is
   DONE: the `Op` family has `opQuantize` and `opMatMul`, `bbRun` takes
   the weights, and `circResidualLoop` gives n residual steps with a cost
   law for each n. Flags 22 to 25 are open, and the build uses the
   defaults. Part Z6f session 2 is DONE: the `Op` family also has
   `opLookup` (a table lookup for each entry), `opSoftmax` and
   `opLayerNorm` (on each row of the last dim), each with a cost in
   `opRes`. Flags 26 to 30 are open, and the build uses the defaults.
   Part Z6g session 1 is DONE: the `Op` family also has `opAttention`
   (one head that maps [h, d] to [h, d]), with a cost in `opRes`. Flags
   31 to 35 are open, and the build uses the defaults.
   Part Z6h session 1b is DONE: the domain has the BN254 scalar field
   `Fr` (8 `Nat` limbs, Montgomery CIOS), the canonical predicate
   `frCanon` and the `Inst` `frInst`, with `examples/bn254.lang`. Flags
   38 to 42 are RULED. Part Z6i session 1b is DONE: `frOpRun` runs the
   lookup operator over `Fr` in `frRun`, and `frDot` and `frMatVec` are
   the matrix operators, which `examples/bn254.lang` tests directly.
   Flags 44 to 50 are RULED. Flag 50 is DONE in slice ZL: the matmul
   case of `frOpRun` is `frMatVec`. Part Z6j session
   1b (the kit sharing slice, flags 36 and 37) is DONE: the lowering
   shares the values in each block (flags 51 to 55). Part Z7a (words
   entries, flag 56 (a)) is DONE. Flag 61 is RULED (a). Part Z7b (the
   BabyBear trace and its witness words, flag 62 (a)) is DONE. Flag 63
   is RULED (a). Part Z7c (the BN254 trace and its witness words) is
   DONE. Part Z7d (the docs and the final gate of slice Z7) is DONE.
   Slice Z7 is DONE. Slice ZL (the lazier checker, `quote_lazy` and
   `same_closure`) is DONE. Flag 64 is RULED (a): the gate checks the
   examples with `build/langc-256` (a 256 MiB arena). Slice Z8 (the
   LLZK target) is DONE. Slice Z9 (the Halo2 target) is DONE.
2. The LLZK target is DONE in slice Z8. `make check` checks the LLZK
   words and the printer, and needs no LLVM tool. `make zk-check` needs
   `llzk-opt` from llzk-lib v2.1.2 (LLVM 20.1.8, a nix build in part
   Z8a), because llzk-lib main needs LLVM 23.1.3. Set `LLZK_OPT` to its
   path. The lookup op is refused (flag 73 (a)).
3. The Halo2 target is DONE in slice Z9. `make check` checks the Halo2
   words, the printer `build/halo2print` and the crate writer, and needs
   no Rust tool. `make halo2-check` prints the 5 crates and runs
   `cargo test --offline --locked -j 2` on them (halo2_proofs
   MockProver), outside the sandbox. The lock file is
   `test/halo2/Cargo.lock`. Only the last trace stage maps to circuit
   cells, and the lookup op is refused (O13).

## Known limits

- The interval normal form holds at most 16 meets of at most 8 literals.
  A larger term stays unreduced, so the checker can fail to equate two
  large terms that are equal. Cause: fixed-size arrays in
  `src/front/eval.c`.
- `<i> t` and `funExt h` do not infer a type. They need a declared
  `PathP` type. Cause: the checker cannot find the line from the body.
- `refl` and `cong` on a path need a constant line. Cause: `<i> a` over
  a line that varies is ill typed, because a fixed `a` is not in `A i`.
  A dependent `cong` needs the checker to infer the line (a later item).
- `trans` over a path needs a constant line. A path over a varying line
  fails with `TYPE_MISMATCH`. Cause: the path form of `trans` is an
  `hcomp`, and `hcomp` composes in one type.
- `trans p refl` fails with `TYPE_INFER`. Cause: `trans` infers its
  second argument, and `refl` does not infer a type. Use `<i> b` with a
  declared type (`rfl` in `examples/kan.lang`).
- `transp` stays unreduced along a family whose parameters vary on the
  line, and along a constant line of a neutral type. Cause: the core has
  no regularity at a neutral type, and no rule for a family constructor
  with varying parameters.
- `transp` of `refl` along a varying `Eq` line stays unreduced. Cause:
  the endpoint indices need not remain definitionally equal at `i1`.
- A system holds at most 16 cells. Cause: fixed-size arrays in
  `src/front/check.c`.
- An interval, path or system value cannot exist at run time
  (`REFUSE_RUNTIME_PATH`). Cause: the IR has no interval, no path and no
  system. `fromPartial` of a total system reduces before the lowering.
  Elimination of a neutral partial element stays unreduced.
- A `transp` that stays unreduced lowers to its payload `u`. Cause: the
  core has no Glue and no univalence, so `transp` cannot change the data.
  Revisit this lowering when Glue or `ua` lands.
- `hcomp` needs a side of the form `fun j => [ ... ]`. A neutral side
  fails with `TYPE_SYSTEM`. Cause: the checker reads `phi` from the faces
  of the system.
- `hcomp` at a constructor with fields (`cons`, `some`, `inl`, `inr` and
  a family constructor) or at `refl` stays unreduced. So at a Sigma type
  with an `Eq` second part, only the first part moves. Cause: no rule
  moves `hcomp` into the fields or along the `Eq` indices yet.
- `hcomp` at a universe stays unreduced. Cause: the core has no Glue.
- An empty `hcomp` at a neutral base stays unreduced, as in CCHM.
- An `hcomp`, `comp` or `fill` that stays unreduced at run time gives
  `REFUSE_RUNTIME_PATH`. Cause: the IR has no system.
- `PathP` constructors, a dimension of 3 or more and `transp` at a
  higher inductive type whose parameters vary are later slices. A `fold`
  over a higher inductive type has no motive (no dependent elimination).
- `natMax n n` does not convert with `n`. Cause: `natMax` has no
  idempotence rule. The three neutral rules of part Z5b (flag 12) give
  `natMaxAssoc` by induction.
- A definition with arguments and a record result (`Resource`, `Item`)
  is not an entry, so the lowering does not take it (flag 13). It is a
  kit limit (`src/front/check.c`, `EVAL_ARGS`). `resGatesOf` and
  `resDegreeOf` in `examples/resource.lang` give `resAdd` and `natMax`
  at run time through `Nat` fields.
- `Tensor F s` currently checks the number of dimensions, but not their
  sizes. Each dimension adds a `List` level and its numeric size is
  ignored, so short rows and ragged tensors typecheck. The nested
  Fin-indexed representation in the design remains to be implemented.
- Flag 15 (RULED (a) 2026-10-16) is closed in part Z6d. The domain has
  a concrete `Inst`: `bbInst W rd`, where `rd : W -> BbWeights` reads
  the matrices from the weights (part Z6f). `examples/babybear.lang`
  has entries over `Layer` at closed shapes.
- `examples/category.lang` has no entry. Each definition in it takes an
  abstract `Inst`. The lowering gives an empty module. The entries over
  the concrete `bbInst` are in `examples/babybear.lang`.
- The `Inst` record is different from the N3 design at two points. A
  USER ruling is open for each. `Inst` carries the identity and
  sequence witness laws (`instIdSound`, `instSeqSat` and the related
  laws), because these laws do not check for an abstract instance
  (flag 6). `instInputWit` takes the weights, and `instInputWeights`
  gives them back (flag 14). The N3 design gives
  `inputWitness : Tensor s -> Witness`.
- The sequence witness law `instSeqSat` gives a truncated split (flag
  16, RULED (a) 2026-10-16, `SPEC.md` O9). A witness `w` of
  `circSeq a b c f g` is `instSeqWit u v` for a `u` and a `v` with the
  same weights: `u` for `f` and `v` for `g`. Part Z6b removes
  `instSeqEval` and adds `soundnessSplit`. `soundnessSeq` folds the
  `Trunc` by `soundnessSplit`. The concrete witness model of part Z6d
  uses set-valued folds, not a Type-valued fold over `Circuit`.
  `bbInstSeqWit u v` keeps the trace of `v`, not an append of the traces
  (flag 20, RULED keep 2026-10-17).
- Part Z6c: `isSetList` by encode-decode, and `isSetTensor` from
  `isSet F`. A concrete `Inst` gets `instTensorSet` from it in part
  Z6d. `zeroT`, `mapT`, `zipWithT` and `sumT` are `List` motive folds
  over `dims s`. `sumT` takes `add` and its zero as arguments, because
  the `Inst` add comes in part Z6d. A stage of a trace is `Stage A`, a
  `Sigma` of a shape and a tensor at that shape. `stageDec A z s`
  reads a stage at the shape `s`, one level at a time. `stageDecEnc`
  is the round trip at the shape of the stage, by `List` induction.
  The stage needs no new family.
- Part Z6d: the kit operations `natDiv` and `natMod` (the quotient and
  the remainder). A divisor of 0 traps. The trap gives the code
  `EVAL_OVERFLOW` and its overflow text, as each kit trap does (flag 19,
  RULED (a) 2026-10-17). The Wasm writer uses a divisor of 1 when the
  divisor is 0 and records the trap in the flag. The domain adds
  `stageLast`, the BabyBear field operations (`bbP`, `bbAdd`, `bbSub`,
  `bbMul`, `bbNeg`) and `bbInst W rd`, a concrete `Inst` at a weights type
  `W` and a reader `rd : W -> BbWeights` (part Z6f). Flag 15 is closed.
  A witness is the weights and a trace of stages. `bbRun` gives the
  function of a circuit, by a `Circuit` fold into a set of functions.
  A witness satisfies `f` at `x` when the read
  of its last stage is `bbRun a b f (rd (first w)) x`, where `w` is the
  witness. `bbInstSeqWit u v` keeps the weights of `u` and the trace of
  `v` (flag 20, RULED keep 2026-10-17). An append of the traces does
  not give `instSeqWitEval` when `v` has no stages. It also does not
  give `instSeqSat`, because satisfaction constrains only the last stage.
  `examples/babybear.lang` has the first entries over `Layer`.
  `listZipWith` unfolds the two list tails. `tensorConv` uses a list
  view with an equality for transport. These case splits return data,
  so zip and stage decoding also lower when their input lists and
  the source shape are known only at run time. `examples/tensor.lang`
  checks both operations against the generated Wasm.
- Flag 18 (OPEN, default (i)): the kit has no Empty former (a family
  needs at least one constructor), and `Unit` has no eta and no
  eliminator. Part Z6c uses stand-ins and does not change the kit:
  `Void` is `Path Nat 0 1`, `isPropVoid` comes from `natIsSet`, and
  `voidElim` is a `transp` over a `Nat` motive fold into `Type 0`. The
  code of nil and nil in `listCode` is `Path Nat 0 0`. `SPEC.md` O10.
- In a signature that binds an `Inst` as `I`, the name `I` hides the
  interval type `I`, so the signature cannot write `(j : I)`. An
  interval lambda in a type needs `(i : I)`. Give the `Inst` another
  name (for example `K`) when the signature needs the interval type.
- The succ iota of the `Nat` motive fold applies only to `natAdd x 1`.
  `natAdd x L` with a number `L` greater than 1 does not unfold. Cause:
  the iota matches only the form `natAdd x 1`.
- The kit limits stay. `docs/host/CAPABILITY.md` gives them.

## Internal boundaries

`src/front/lexer.c` and `src/front/parser.c` read the path syntax. `<i> t`,
`p @ r`, `~`, `/\` and `\/` become applications of builtin heads that no
identifier can spell, so the parser adds no new term kind. A system
`[ f |-> u , ... ]` becomes nested cells over the heads `|->` and `[]`,
and `(r = 0)` and `(r = 1)` become `~ r` and `r`.
`test/parse/expect.txt` gives the parse refusals.

`src/front/check.c` holds the builtin rows for C1, C2 and C3, the pretype
mode, the endpoint checks, the path forms of `refl`, `symm`, `cong`
and `funExt`, the cover and overlap checks of a system, and the line and
constancy checks of `transp`, the side and base checks of
`hcomp`, and the constructor checks of a higher inductive type (the
shape `Path X a b` or `Path (Path X a b) p q`, the point-term ends,
uniform recursion and the field telescope of a path constructor).
`test/check/expect.txt` gives the refusal code of each reject case
(`TYPE_ENDPOINT`, `TYPE_PRETYPE`, `TYPE_MISMATCH`, `TYPE_SYSTEM`,
`TYPE_LINE`).

`src/front/eval.c` holds the interval normal form, the face normal form,
the reduction of systems and `fromPartial`, the reduction of
`p @ r` (the value keeps the ends of the type) and path eta in the
conversion check, the reductions of `transp` and `hcomp`, the end
rules of a path constructor at an interval and the formal `hcomp` at a
higher inductive type. Neutral interval terms participate in the same algebra
as interval variables. The printer emits source syntax for interval and
path normal forms, and for a path constructor at an interval.
`test/fronttool.c` checks these round trips, neutral
interval laws and type-directed checking of the argument to `funExt`.
It also checks system normal forms, elimination with neutral branches,
and conversion of the face indices of `Partial` types.

`src/front/lower.c` refuses interval, path and system values at run time,
and with them a path constructor at an interval and the formal `hcomp`
at a higher inductive type. A point constructor of a higher inductive
type lowers as a data constructor.
A `transp` that stays unreduced lowers to its payload. An
`hcomp` that stays unreduced is refused. A
program cannot use `I`, `PathP` or `Path` values in an entry. The gate
checks `test/ir/path.lang` and `test/ir/hit-path.lang`.

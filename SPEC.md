# zkml-lang specification (draft)

Status: draft before milestone M0. `zkml-lang` is a working name.

Fill each section. The quoted line under each heading says what to write
there. Delete the quoted line when the section is complete.

## 1. Purpose

> Write here: what one program describes, what the compiler checks, and what
> it writes. Name the host and the target. Say that the core types and core
> operations are only those of `design/DESIGN.md`.

zkml-lang is a language for <the domain>. Its type formers are F1 to F15 of
`formers/FORMERS.md`. Its core data types and core operations are only the
types and operations of `design/DESIGN.md` (the design).

## 2. Programs

> Write here: the form of a program and the refusal list. Copy the host
> forms from `formers/tcc-wasm.md`. Add each fixed first definition that the
> domain needs (for example a size parameter).

A program is a sequence of definitions:

```
def NAME : TYPE := TERM
```

The compiler refuses these host forms in a program: <the list from
`formers/tcc-wasm.md`>. It also refuses a definition that uses a core name or
a prelude name. Thus a program cannot add a data type, an unproved fact or
general recursion. Recursion comes only from `fold` and `unfold` (F6, F7).
The compiler writes the target; a program cannot.

## 3. Type formers

> Write here: only the deviations from the formers. Do not copy the forms.
> Give each former whose status in `formers/tcc-wasm.md` is not DONE, and the
> open item (section 9) that records it.

The type formers are F1 to F15 of `formers/FORMERS.md`. This language uses
the tcc-wasm column of the realization matrix. `formers/tcc-wasm.md` gives
the host form of each former.

| ID | Status on tcc-wasm | Effect on this language | Open item |
|---|---|---|---|
| | | | |

## 4. Structures

> Write here: the carriers of Monad (F5), fold and unfold (F6, F7) and
> filter (F8) that the design uses. Name each carrier that the host does not
> give.

- **Monad**: `pure`, `map`, `bind` on <carriers>.
- **Algebra**: `fold` on <carriers>. `unfold` on <carriers>, or the reason
  that there is no `unfold`.
- **Filterable**: `filter` on <carriers>.

## 5. Core types

> Write here (DOMAIN SLOT): only the types of `design/DESIGN.md`. Give each
> type its meaning in the design and its definition in host forms. Mark each
> definition that a probe fact forces with "probe-forced".

| Type | Meaning (design section) | Definition |
|---|---|---|
| | | |

## 6. Core operations

> Write here (DOMAIN SLOT): only the operations of `design/DESIGN.md`. Give
> each operation its type and its meaning in the design.

| Operation | Type | Meaning (design section) |
|---|---|---|
| | | |

## 7. Target and instance encoding

> Write here: what the compiler writes. For a JSON target, give the instance
> rule and the encoding table. For a contract target, give the storage, the
> entries and the guards. Say how each proof erases.

An instance is a top-level definition whose type is a data type (F14).

| Type | Encoding |
|---|---|
| | |

### Witness format

A words entry (part Z7a, flag 56 (a)) is an entry with a `List Nat`
result. Its Wasm export returns the address of a words block in the
memory export `$memory` (flag 61 (a)). The block is [n, w0 .. w(n-1)]:
the word count n, then the n words. A word is an unsigned 64-bit
integer in 8 bytes, low byte first. Word k of the block at address A is
at A + 8k. `langc eval` prints the n words with one space between
them, or `nil` when n is 0. `test/run-wasm.mjs` gives the same output
in batch mode with a `langc abi` file (see `probe/CAPABILITY.md`
section 5). Its single-entry mode prints the block address.

The guest defs `bbWitnessWords` (BabyBear) and `frWitnessWords` (BN254)
in `domain/domain.lang` give the witness words of the weights and a
trace (flag 58 (a)). The checker typechecks these guest definitions.
The layout below assumes rectangular weight matrices, tensors whose
list lengths match their dimensions, well-formed stage words, and
canonical field elements. These value constraints are caller
preconditions. The serializers concatenate the supplied rows and
stage words, and copy field limbs without validating these constraints.

| Part | Words | Value |
|---|---|---|
| Version | 1 | 1. |
| Field id | 1 | 1 for BabyBear, 2 for BN254. |
| Element size e | 1 | The words for each element: 1 for BabyBear, 8 for BN254. |
| Matrix count m | 1 | The count of the weight matrices. |
| Matrices | m blocks | For each matrix (`matWords`, `frMatWords`): 2 (the rank), the row count, the column count, then the entries in row-major order, e words for each entry. |
| Stage count t | 1 | The count of the stages of the trace. |
| Stages | t blocks | For each stage (`stageWords`, `frStageWords`): the rank, the dims, the aux word 0, then the entries in row-major order, e words for each entry. |

- The column count of a matrix is the length of its first row, or 0
  when the matrix has no row.
- The rank of a stage is the count of its dims. The aux word is a count
  of aux words. It is 0 in slice Z7, and no aux word follows it.
- A canonical BabyBear element is one word below 2013265921. A
  canonical BN254 element (`Fr`) has 8 limbs fr0 .. fr7 below 2^32
  (flag 38), in the plain form (flag 39), representing a value below
  the BN254 scalar modulus r. `frWords` writes the supplied limbs low
  limb first, one word for each limb.
- The trace has one stage for each op and each residual of the circuit,
  in run order (`bbTrace`, `frTrace`, flag 62 (a)). An identity circuit
  has an empty trace and stage count 0. For a trace with stages, the
  last stage records the result of `bbRun` or `frRun`. The definitions
  `bbTraceLast` and `frTraceLast` in `examples/witness.lang` check this
  for their concrete residual circuits.

## 8. Host and target

> Write here: the host, its pinned commit (`PIN`), the facts from
> `probe/CAPABILITY.md` that set the design, and the check that the compiler
> runs on its output (for example the axiom report).

## 9. Open items

> Write here: one item for each open question. Number the items O1, O2 and
> so on. Mark each decision as "RULED YYYY-MM-DD (USER): <the ruling>". Mark
> each fact that a probe found as "Probe-forced".

- O1. The denotation of a layer (PLAN N7, ruling O4). The weights are part
  of the witness. Thus a layer denotes a function `Weights -> Tensor ->
  Tensor`, not `Tensor -> Tensor`. The design gives one functor `denote` from
  the layer category to the types. Default: `denote I ws` is a functor for
  each weight assignment `ws`. It sends a shape `s` to `Tensor (instF I) s`
  and a layer `L` to `layerDen L ws`. The identity law and the composition
  law hold by refl. Status: built with the default. Slice Z5b gives
  `Layer`, `idL` and `layerSeq` with this denotation. Slice Z5c gives
  `TypeFunctor` and `denote`: `tfOb` is `Tensor (instF I)`, `tfHom L` is
  `layerDen L ws`, and `tfId` and `tfSeq` hold by refl. Part Z6f: `bbRun
  a b f ws x` takes the weights `ws : BbWeights` before the input `x`. Thus
  `bbRun a b f ws` has the form of `layerDen L ws`, and `opMatMul` reads
  its matrix from `ws`. Flags 26 to 30 (part Z6f session 2) are open, and
  the build uses the defaults: the table of `opLookup` is a matrix of the
  weights, softmax reads its entries as `Nat` (no signed encoding),
  layerNorm reads the inverse square root from a table, and `mapLastT` at
  rank 0 reads the entry as a row of length 1. The circuit form of softmax
  does not yet check that the max is an entry of the row (a soundness gap
  to fix later).
  Flags 31 to 35 (part Z6g session 1) are open, and the build uses the
  defaults: `opAttention` reads W_Q, W_K, W_V and the exp table at
  separate indices of the weights, and the softmax k is a field (flag 31);
  the scale of the scores is part of the exp table (flag 32); each score
  gets (p - 1) / 2 before the softmax, so a score in the field reads as a
  signed score (flag 33); the output keeps the 2^k scale (flag 34); and the
  cost is 3 h d d + 2 h h d + 2 h h + h gates and 3 h h lookups (flag 35).
  Attention uses the softmax, so it has the same soundness gap of the max
  (flag 27). The lowering shares the values in each block (flags 51 to
  55), so babybear has the attention entry `bbAttentionSumUV` on its
  inputs and the 5 constant entries (flag 36).
  Flags 37 to 42 (part Z6h) are RULED: the bn254 entries are constant
  (flag 37), and `frMatVecSumUV` is an entry on its inputs (flags 51 to
  55); `Fr` is a record of 8 `Nat` limbs below 2^32 (flag 38);
  `frMul` is Montgomery CIOS in the plain form, with a second `frMont`
  by 2^512 mod r (flag 39); the canonical predicate is `frBad`,
  `frCanonB` and `frCanon` as domain defs, with rows and a `Flag` entry
  in `examples/bn254.lang` (flag 40); `frInst` is the `Inst` over `Fr`
  in the form of `bbInst`, and `frRun` has the structural cases of
  `bbRun` with `zeroT Fr frZero` at an operator (flags 41 and 42).
  `frInstSat` has no `frCanon` clause: the `Inst` family quantifies
  `instInputSat` over every x, and `instInputEval` forces the read of
  the input witness to be x, so a clause on x is not provable there. A
  clause on the output (`frCanon x -> frCanon` of the read) needs a
  theorem that `frRun` keeps `frCanon`, which is not built. That clause
  is flag 43.
  Flags 44 to 50 (part Z6i) are RULED: `frOpRun` is the `Op` run over
  `Fr` in `frRun`. The lookup operator is real: `frLookup` reads the
  table, and `frEqB` compares the limbs, which is exact on canonical
  values (flag 45). `opQuantize`, `opSoftmax`, `opLayerNorm` and
  `opAttention` need a `Nat` division or a `Nat` order, which `Fr` does
  not have, and give `zeroT Fr frZero` (flags 44, 46, 47 and 48). The
  matmul case of `frOpRun` is `frMatVec` over the weights at `mmK`
  (flag 50, DONE in slice ZL). In part Z6i, this case put the symbolic
  `frMul` chain under the stuck `Op` fold, and the check of
  `frInstSeqWitSat` used all of the 1 GiB arena (flag 49, measured in
  part Z6i session 1c). Slice ZL makes the checker lazier: `symm`,
  `trans` and `cong` quote a closure as its body applied to the quoted
  environment (`quote_lazy`), and the conversion accepts two closures
  with the same body and convertible environments (`same_closure`).
  `examples/bn254.lang` tests `frDot` and `frMatVec` directly, and the
  rows `frRunMatMulSum` and `frOpMatMulSum` test the matmul case
  through `frRun` and `frOpRun`.
  Flag 64 is RULED (a): `make check` builds `build/langc-256`, a `langc`
  with a 256 MiB arena (`ARENA_LIMIT_BYTES`), and checks the 28
  examples and the 4 `Fr` rows with it. The shipped `langc` keeps the
  1 GiB arena. This bound is on the arena bytes, not on the resident
  memory of the process.
  Flags 51 to 55 (part Z6j) are RULED: the lowering keeps a memo of the
  lowered values in each block. The key is the `Value` pointer (flag
  51): `apply_value` and the cache of the defs give one pointer to one
  value. The scope is the block and the blocks that it is in (flag 52),
  so one arm of a `flagIf` or a switch never reads a local of another
  arm. The memo gives the `Operand` as the lowering returned it (flag
  53), with no new statement. Only `VAL_OP` and `VAL_STUCK` values go in
  the memo (flag 54): the other kinds lower to a constant or a level, or
  the lowering refuses them. The tables grow in the arena until the
  arena is freed (flag 55); the peak memory of a check does not change.
- O2. The order of composition in `Category` (USER flag 7, N9). The
  design sets `LayerCat ._⋆_ = _∘_`, but `⋆` in the Cubical library is
  diagrammatic: `f ⋆ g` is `f`, then `g`. Thus `catSeq` takes the order
  of `∘`: `catSeq a b c g f` is `g` after `f`, as in `layerSeq`. With
  this order, `LayerCat` is well typed. The check row
  `test/check/cat-seq-order.lang` shows that the diagrammatic order is
  refused. Status: built in slice Z5c. RULED 2026-10-16 (USER): `catSeq`
  keeps the order of `∘`. No code change.
- O9. The sequence witness laws of `Inst` (USER flag 16, N8). In part
  Z5b, `instSeqSat` gave the same witness `w` to the two stages of
  `circSeq a b c f g`, and `instSeqEval` gave the value of the sequence
  at `w` as the value of `g` at `w`. With these laws, each layer of an
  instance must be an idempotent endo-layer (the F16 chain). RULED
  2026-10-16 (USER): (a). `instSeqSat` gives a truncated split,
  `Trunc (Sigma (u v : instWitness) ...)` with four parts:
  `Path w (instSeqWit a b c f g u v)`, a Path from the weights of `u` to
  the weights of `v`, `instSat a b f x u` and
  `instSat b c g (instEval a b f u) v`. The domain has no `instSeqEval`.
  `instSeqWitEval` stays. `instSeqWitSat` also requires the weights
  path, so only witnesses with equal weights establish satisfaction
  of their composite. `seqWitness` obtains this path from the two
  component witnesses at the common weights `ws`.
  Probe-forced: the weights path. `soundnessSeq`
  gives the denotation at `instWeightsOf w`, and without the weights
  path `soundnessSeq` does not check. `soundnessSplit` gives the
  soundness at one split. `soundnessSeq` folds the `Trunc` into the goal
  by `soundnessSplit`, because the goal is a path in a set. The check
  row `test/check/inst-seq-eval.lang` shows that a use of `instSeqEval`
  is refused (`NAME_UNKNOWN`). The check row
  `test/check/inst-seq-same-wit.lang` shows that the same witness for
  the two stages is refused (`TYPE_MISMATCH`). The check row
  `test/check/inst-seq-mixed-weights.lang` refuses witness composition
  without the weights path (`TYPE_MISMATCH`). Status: built in part
  Z6b.
- O10. No empty type and no `Unit` eta (flag 18). The kit has no Empty
  former, because a family needs at least one constructor. `Unit` has
  no eta and no eliminator, so `isProp Unit` does not check. Part Z6c
  uses stand-ins: `Void` is `Path Nat 0 1`, `voidElim` is a `transp`
  over a `Nat` motive fold, and the code of nil and nil is
  `Path Nat 0 0`. Default (i): keep the stand-ins, with no kit change.
  Status: OPEN.
- O11. The Wasm witness runtime (slice Z7, flags 56 to 63). RULED
  2026-10-20 (USER): flags 56 to 60 (a). An entry with a `List Nat`
  result is a words entry, the module exports its memory, and
  `test/run-wasm.mjs` reads the words block (flag 56). The field
  arithmetic is guest code in `domain/domain.lang` (flag 57). The
  commands `langc eval` and the Wasm writer run it through the `Nat`
  operations of the kit, and `src/` has no field code. The trace has
  one stage for each op and each residual, and the examples check its
  last stage (flag 58). The BN254 trace runs the operators that
  `frRun` runs now. Slice ZL completed the lazy checker and the real
  matmul case of `frOpRun` (flag 59). A witness entry takes `Nat` or
  `Flag` parameters, and its weights are constants (flag 60). RULED
  2026-10-21 (USER): flags 61 to 63 (a). The memory export has the name
  `$memory`. A guest name cannot have a `$`, so no entry export has
  this name (flag 61). The trace fold is in continuation-passing style
  into the stage words `List (List Nat)`, so the `idl`, `idr` and
  `assoc` cases hold by beta and eta (flag 62). The grid gives the
  BabyBear p - 1 and p values only to an entry with no loop. The eval
  row `bbWitResidual 2013265920 1` gives the p - 1 case in `langc eval`
  only (flag 63). Status: built in slice Z7. Section 7 (`Witness
  format`) gives the witness words.

## 10. Milestones

> Write here: the content of each milestone, and a dated status line under
> the current milestone.

| Milestone | Content |
|---|---|
| M0 | Host probe (`probe/CAPABILITY.md`); the core types construct and check; refusal list; examples and tests |
| M1 | Core operations |
| M2 | Queries and reads, or the contract |
| M3 | Hardening: speed, limits, a checked certificate for the output |

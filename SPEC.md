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
  `layerDen L ws`, and `tfId` and `tfSeq` hold by refl.
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

## 10. Milestones

> Write here: the content of each milestone, and a dated status line under
> the current milestone.

| Milestone | Content |
|---|---|
| M0 | Host probe (`probe/CAPABILITY.md`); the core types construct and check; refusal list; examples and tests |
| M1 | Core operations |
| M2 | Queries and reads, or the contract |
| M3 | Hardening: speed, limits, a checked certificate for the output |

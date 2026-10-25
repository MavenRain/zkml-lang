# zkml-lang specification

Status 2026-10-24: slices Z0 to Z10 and ZL are DONE. Slice Z11 writes the
final docs, runs the final gate and closes the plan.

## 1. Purpose

zkml-lang is a language for zero-knowledge circuits of machine-learning
inference. A program is a set of definitions over the domain prelude
(`domain/domain.lang`): shapes, tensors, layers, circuits and two field
instances. The compiler `build/langc` checks each definition in cubical
type theory, with the type formers F1 to F15 of `formers/FORMERS.md` and
the cubical formers C1 to C7 of `formers/tcc-wasm.md`. `langc build`
writes a Wasm module, the witness runtime, with one export for each entry.
The host printers `build/llzkprint`, `build/halo2print` and
`build/plonky3print` write the LLZK, Halo2 and Plonky3 forms from the
words of a guest entry. The host is TinyCC, with the tcc-wasm kit
(section 8). The core data types and core operations are only the types
and operations of `design/DESIGN.md` (the design).

## 2. Programs

A program is one file of at most 1 MiB, with a sequence of definitions:

```
def NAME : TYPE := TERM
```

A line comment starts with `--`. A definition can use only the core
names, the prelude names and the earlier definitions of the file. The
compiler refuses these host forms in a program (`docs/host/README.md`,
the rule table):

| Rule | Code | The compiler refuses |
|---|---|---|
| R1 | `REFUSE_DATA` | a `family` declaration |
| R2 | `REFUSE_AXIOM` | an `axiom` declaration |
| R3 | `REFUSE_REC` | `def rec`, or a definition that refers to itself or to a later definition |
| R4 | `REFUSE_NAME` | a core name, a prelude name, a reserved name or a repeated name as a new definition |

Thus a program cannot add a data type, an unproved fact or general
recursion. Recursion comes only from `fold` and `unfold` (F6, F7). The
mutants `test/check/data.lang`, `test/check/axiom.lang`,
`test/check/rec.lang` and `test/check/core-name.lang` give each code
(`test/check/expect.txt`).

`langc build` also refuses an entry that needs a value with no
first-order form at run time. These are HOST-LIMIT codes, not defects of
the program:

- `REFUSE_RUNTIME_CLOSURE`: a function value must exist at run time (F9).
- `REFUSE_RUNTIME_TREE`: a fold walks a tree that exists only at run
  time (F6).
- `REFUSE_RUNTIME_PATH`: an interval or path value must exist at run
  time (C1, C2, C3).

The compiler writes the target; a program cannot.

The fixed first definitions are the prelude `domain/domain.lang`. The
build embeds it in `build/langc` (`Makefile:18`). Each family,
constructor, field and definition name of the prelude is a core name
(`domain/domain.lang:2`). The prelude gives these names:

- `Shape` (`domain/domain.lang:33`): the dimensions of a tensor, a
  `List Nat`. The shape is the size parameter of the domain.
- `Tensor A s` (`domain/domain.lang:92`): a nested `List A`, with one
  level for each dimension of `s`.
- `Op a b`, `Circuit a b` and `Layer I a b` (`domain/domain.lang:41`,
  `domain/domain.lang:48`, `domain/domain.lang:119`): the ops, the
  circuits and the layers between two shapes.
- `Inst` (`domain/domain.lang:99`): an instance: the field, the witness,
  the weights and the circuit semantics.
- The two field instances: `bbInst` (BabyBear, p = 2013265921,
  `domain/domain.lang:537`) and `frInst` (the BN254 scalar field `Fr`,
  8 limbs below 2^32, `domain/domain.lang:766`).
- `LlzkHom`, `Halo2Hom` and `Plonky3Hom` (`domain/domain.lang:845`,
  `domain/domain.lang:983`, `domain/domain.lang:1065`): the guest models
  of the three circuit targets.

Sections 5 and 6 give the core types and the core operations.

## 3. Type formers

The type formers are F1 to F15 of `formers/FORMERS.md` and C1 to C7 of
`formers/tcc-wasm.md`. This language uses the tcc-wasm column of the
realization matrix. `formers/tcc-wasm.md` gives the host form and the
status of each former. Each former is DONE, except these two:

| ID | Status on tcc-wasm | Effect on this language | Open item |
|---|---|---|---|
| F15 | PARTIAL: only the domain file declares an indexed family | A program cannot declare a family (rule R1). The prelude declares the indexed families, for example `Lot`, `Op`, `Circuit` and `Layer`. A constructor takes its indices first. The index erases on Wasm. | None: rule R1 is a design rule. |
| C6 | PARTIAL: `natMax` has no idempotence rule | The checker does not reduce `natMax n n` to `n` when `n` is a variable. The three `natMax` neutral rules of slice Z5b apply. | None: flag 12 RULED (a) in slice Z5b. |

A former that is DONE can have a limit. `formers/tcc-wasm.md` gives each
limit: F5 is over `Option`, `Sum E` and `List`; F14 has `Type 0` and
`Type 1`, with no cumulativity; F6, F9, C1, C2 and C3 have the HOST-LIMIT
codes of section 2.

## 4. Structures

The design uses only the host carriers for the structures (F5 to F8):

- **Monad**: `pure`, `map` and `bind` on `Option`, `Sum E` and `List`
  (F5). The prelude uses `map` on `List` (for example
  `domain/domain.lang:408`) and on `Option`, and `bind` on `Option`
  (`domain/domain.lang:331`). No prelude definition uses `pure`.
- **Algebra**: `fold` on `Nat` (with a motive), on `List` (with a motive,
  C7) and on the prelude families, for example `Stack` and `Rose` (F6).
  `unfold` makes a `List` with a `Nat` step limit (F7). The prelude uses
  `unfold` once, in `listZipWith` (`domain/domain.lang:330`).
- **Filterable**: `filter` on `Option` and `List` (F8). The prelude uses
  `filter` on `List` (`domain/domain.lang:19`).

The host does not give a Monad, an Algebra or a filter on `Tensor A s`.
The prelude gives `mapT` (`domain/domain.lang:321`), a `fold` over the
dimension list of the shape. A `fold` over a tree that exists only at run
time (for example `Tree` or `Rose`) has no Wasm form
(`REFUSE_RUNTIME_TREE`, section 2).

## 5. Core types

The core types are the types of `design/DESIGN.md` and the defaults of
`docs/PLAN.md` section 4 (N1 to N7). The design is one fragment with no
section numbers, thus the meaning column cites the line of the design
name. Each definition is in `domain/domain.lang`. "Probe-forced" marks a
definition that a probe or an O item forces (section 9).

| Type | Meaning (design section) | Definition |
|---|---|---|
| `Shape` | The dimensions of a tensor, a `List ℕ` (`design/DESIGN.md:31`, "Shapes and tensors"). | `family Shape` (`domain/domain.lang:33`): `shape (dims : List Nat)`. |
| `Tensor` | The entries at a shape over the field `F` (`design/DESIGN.md:36`). | `def Tensor` (`domain/domain.lang:92`): a nested `List`, one level for each dimension, with the field as an argument. `isSetTensor` (`domain/domain.lang:307`) proves that it is a set, by an encode-decode proof over `listCode` (`domain/domain.lang:267`). Probe-forced (O10): the stand-ins `Void` (`domain/domain.lang:260`, `Path Nat 0 1`), `voidElim` (`domain/domain.lang:262`, a `transp` over a `Nat` motive fold) and the code of nil and nil (`Path Nat 0 0`). |
| `Circuit` | The circuit IR from `inS` to `outS`, with `id` (`design/DESIGN.md:40`, "Circuit IR"). | `family Circuit` (`domain/domain.lang:48`): a QIIT with `circId`, `circSeq` (the design `composeCircuits`), `circOp`, `circResidual`, the path constructors `circIdl`, `circIdr` and `circAssoc`, and the set truncation `circSquash` (PLAN N2). `circOp` takes an `Op` (`domain/domain.lang:41`), with one constructor for each operator of section 6. |
| `Resource` | The gates, the lookups and the degree of a circuit (`design/DESIGN.md:45`, "Resource monoid"). | `family Resource` (`domain/domain.lang:75`): `res (resGates resLookups resDegree : Nat)`. |
| `Monoid` | The monoid of `Cubical.Algebra.Monoid` (`design/DESIGN.md:23`), the type of `ResourceMonoid`. | `family Monoid` (`domain/domain.lang:86`): a carrier in `Type 0`, the unit, the operation and the 3 laws. |
| `Inst` | The module parameters: the field `F`, `Witness`, `satisfies` and `eval` (`design/DESIGN.md:25`). PLAN N3 adds the input witness, and N7 adds the weights. | `family Inst` (`domain/domain.lang:99`): `instF`, `instWitness`, `instWeights`, `instWeightsOf`, `instSat`, `instEval`, `instInputWit` (N3), the input, id and sequence laws (O9) and `instTensorSet`. Probe-forced (O9): the weights path in `instSeqSat` and `instSeqWitSat`. |
| `Layer` | A circuit with its denotation, soundness, completeness and resources (`design/DESIGN.md:77`, "Core Layer"). | `family Layer` (`domain/domain.lang:119`) over an `Inst`: `layerCircuit`, `layerDen` (a function of the weights, N7 and O1), `layerSound`, `layerComplete` (a `Trunc`) and `layerRes`. |
| `Category` | The category of `Cubical.Categories.Category` (`design/DESIGN.md:21`), the type of `LayerCat` and of the 3 target categories. | `family Category` (`domain/domain.lang:788`): the objects, the homs, id, seq, the 3 laws and the proof that each hom type is a set (N4). |
| `Functor` | The functor of `Cubical.Categories.Functor` (`design/DESIGN.md:22`), the type of `denote` and of the 3 export functors. | `family TypeFunctor` (`domain/domain.lang:798`), a functor into types, for `denote`. `family Functor` (`domain/domain.lang:808`) for the export functors. |
| `LLZKIRCat`, `Halo2CircuitCat`, `Plonky3CircuitCat` | The target categories of the export functors (`design/DESIGN.md:189`, `design/DESIGN.md:180`, `design/DESIGN.md:186`). | `def LLZKIRCat` (`domain/domain.lang:866`), `def Halo2CircuitCat` (`domain/domain.lang:1003`) and `def Plonky3CircuitCat` (`domain/domain.lang:1085`). Each hom is a module and its resources: `LlzkHom` (`domain/domain.lang:845`), `Halo2Hom` (`domain/domain.lang:983`) and `Plonky3Hom` (`domain/domain.lang:1065`). |

## 6. Core operations

The core operations are the operations of `design/DESIGN.md`, with the
defaults of `docs/PLAN.md` section 4. In the type column, `I` is an
`Inst`. In the design, an operator is a `Layer`. In the domain, an
operator is an `Op` step of a `Circuit` (`circOp`), and each instance
gives its value (`bbOpRun`, `frOpRun`).

The `quantize`, `softmax`, `layerNorm` and `attention` meanings below
describe BabyBear (`bbOpRun`, `domain/domain.lang:486`). For BN254,
`frOpRun` (`domain/domain.lang:693`) returns `zeroT Fr frZero` for these
four operators (flags 44, 46, 47 and 48, section 9).

| Operation | Type | Meaning (design section) |
|---|---|---|
| `_∙_` | `resAdd : Resource -> Resource -> Resource` (`domain/domain.lang:77`) | Adds the gates and the lookups, and takes the max of the degrees (`design/DESIGN.md:50`). |
| `zeroR` | `zeroR : Resource` (`domain/domain.lang:76`) | The unit, `res 0 0 0` (`design/DESIGN.md:53`). |
| `∙-assoc`, `∙-unitˡ`, `∙-unitʳ` | `resAssoc` (`domain/domain.lang:83`), `resUnitL` (`domain/domain.lang:79`) and `resUnitR` (`domain/domain.lang:81`), paths in `Resource` | The monoid laws (`design/DESIGN.md:57`). `resAssoc` uses `natAddAssoc` and `natMaxAssoc` (PLAN N1). |
| `ResourceMonoid` | `ResourceMonoid : Monoid` (`domain/domain.lang:90`) | The resource monoid (`design/DESIGN.md:66`). |
| `_∘_` | `layerSeq : (I : Inst) -> (a b c : Shape) -> Layer I b c -> Layer I a b -> Layer I a c` (`domain/domain.lang:156`) | Sequential composition: first `L1`, then `L2` (`design/DESIGN.md:86`). The circuit is `circSeq`, the denotation is `seqDen` (`domain/domain.lang:131`) at the same weights, and the resources add. |
| `idL` | `idL : (I : Inst) -> (s : Shape) -> Layer I s s` (`domain/domain.lang:125`) | The identity layer (`design/DESIGN.md:101`): `circId`, the identity denotation and `zeroR`. Completeness uses `instInputWit` (PLAN N3). |
| `∘-assoc`, `idL-unitˡ`, `idL-unitʳ` | `layerAssoc` (`domain/domain.lang:203`), `layerIdl` (`domain/domain.lang:197`) and `layerIdr` (`domain/domain.lang:200`), paths in `Layer I a b` | The category laws (`design/DESIGN.md:107`). Each law is a `layerExt` proof, not `refl`: the circuit path is a `Circuit` path constructor, and the resource path is a `Resource` law (PLAN N2). |
| `LayerCat` | `LayerCat : (I : Inst) -> Category` (`domain/domain.lang:796`) | The layer category (`design/DESIGN.md:120`): the objects are the shapes, and the homs are the layers. |
| `isSetHom` | `isSetHom : (I : Inst) -> (a b : Shape) -> isSet (Layer I a b)` (`domain/domain.lang:782`) | Each hom type of `LayerCat` is a set (`design/DESIGN.md:128`, PLAN N4), with no truncation of `Layer`. |
| `soundness-∘` | `soundnessSeq` (`domain/domain.lang:141`) | The soundness of a composition (`design/DESIGN.md:131`). It folds the `Trunc` split of `instSeqSat` into the goal by `soundnessSplit` (`domain/domain.lang:135`). It needs the weights path of `Inst` (probe-forced, O9). |
| `matmul` | `opMatMul n m k : Op (shape (cons n nil)) (shape (cons m nil))` (`domain/domain.lang:43`) | Multiplies by the matrix `k` of the weights (`design/DESIGN.md:139`, ruling O4, PLAN N7). One multiplication gate for each product. |
| `softmax` | `opSoftmax s t k : Op s s` (`domain/domain.lang:45`) | Softmax on each row of the last dim (`design/DESIGN.md:142`). The field function of PLAN N6: fixed point values, and `exp` through a table. |
| `attention` | `opAttention h d q k v t e : Op (shape (cons h (cons d nil))) (shape (cons h (cons d nil)))` (`domain/domain.lang:47`) | Attention with one head from `[h, d]` to `[h, d]` (`design/DESIGN.md:145`, O5): the Q, K and V products, the scores, the softmax and the output. |
| `layerNorm` | `opLayerNorm s g t : Op s s` (`domain/domain.lang:46`) | Layer norm on each row of the last dim (`design/DESIGN.md:148`): the mean, the variance, the inverse square root through a table and the affine step (PLAN N6). |
| `quantize` | `opQuantize s k : Op s s` (`domain/domain.lang:42`) | Divides each entry by 2^k (`design/DESIGN.md:151`). One linear gate and one range lookup for each entry (PLAN N5). |
| `lookup` | `opLookup s t : Op s s` (`domain/domain.lang:44`) | Applies the table `t` of the weights to each entry (`design/DESIGN.md:154`). The LLZK, Halo2 and Plonky3 exports refuse it (flag 69 (a)). |
| `residual` | `circResidual s f : Circuit s s` (`domain/domain.lang:52`) | The skip connection `x + f x` (`design/DESIGN.md:158`). It adds one linear gate for each entry to the cost of `f` (PLAN N5). |
| `residual-loop` | `circResidualLoop : (s : Shape) -> Circuit s s -> Nat -> Circuit s s` (`domain/domain.lang:237`) | The design asks for the coherence path of a single residual step (`design/DESIGN.md:168`). The domain helper iterates `n` residual steps of `f` after `circId`, by `Nat` induction; its result is a circuit, not that path. `costResidualLoop` (`domain/domain.lang:243`) proves that its cost is `resScale n` of the cost of one step. |
| `denote` | `denote : (I : Inst) -> instWeights I -> TypeFunctor (LayerCat I)` (`domain/domain.lang:804`) | The denotation functor at the weights `ws` (`design/DESIGN.md:173`, O1). The id and seq laws hold by `refl`. |
| `exportLLZK` | `exportLLZK : (I : Inst) -> Functor (LayerCat I) LLZKIRCat` (`domain/domain.lang:871`) | The LLZK export (`design/DESIGN.md:189`, O12). `funId` and `funSeq` hold by `refl`. |
| `exportHalo2` | `exportHalo2` (`domain/domain.lang:1008`), a functor into `Halo2CircuitCat` | The Halo2 export (`design/DESIGN.md:180`, O13). |
| `exportPlonky3` | `exportPlonky3` (`domain/domain.lang:1090`), a functor into `Plonky3CircuitCat` | The Plonky3 export (`design/DESIGN.md:186`, O14). |
| `resources-export` | `resourcesExport` (`domain/domain.lang:874`), `resourcesExportHalo2` (`domain/domain.lang:1011`) and `resourcesExportPlonky3` (`domain/domain.lang:1093`) | Each export keeps the resources of a layer (`design/DESIGN.md:193`): the hom holds the module and `layerRes`. |
| `resources-∘` | `resourcesSeq` (`domain/domain.lang:160`) | The resources of `layerSeq` are the sum of the resources of the two layers (`design/DESIGN.md:198`). |
| `cost`, `costLayer` | `cost : (a b : Shape) -> Circuit a b -> Resource` (`domain/domain.lang:234`); `costLayer` (`domain/domain.lang:247`) with `costLayerRes` (`domain/domain.lang:249`) | Not in the design: the default of PLAN N5. `cost` counts the gates, the lookups and the degree of a circuit by a `Circuit` fold. `costLayer` gives a layer the cost of its circuit as its resources. |

## 7. Target and instance encoding

The targets are the Wasm witness runtime, LLZK, Halo2 and Plonky3
(section 1). An emitted entry is a top-level definition whose type is a
data type (F14). The domain has two constructors for field instances.
Each takes a weights type `W : Type 0` and a reader `rd`: `W -> BbWeights`
for `bbInst`, or `W -> FrWeights` for `frInst`. The applications
`bbInst W rd` and `frInst W rd` have type `Inst`. For no matrices, examples
are `bbInst Nat (bbNoWeights Nat)` (`domain/domain.lang:539`) and
`frInst Nat (frNoWeights Nat)` (`domain/domain.lang:768`). LLZK and Halo2
are BN254 only (O12, O13). Plonky3 is BabyBear only (O14).

| Instance | Encoding |
|---|---|
| `bbInst W rd` (BabyBear, `domain/domain.lang:537`) | Witness words: `bbWitnessWords` (`domain/domain.lang:576`). Field id 1, element size 1: an element is one word below p = 2013265921. Subsection `Witness format`. |
| `bbInst W rd` | Plonky3 words: `p3Words` (`domain/domain.lang:1124`). Field id 1: an element is one word below p. Subsection `Plonky3 words`. |
| `frInst W rd` (BN254, `domain/domain.lang:766`) | Witness words: `frWitnessWords` (`domain/domain.lang:742`). Field id 2, element size 8: an element of `Fr` (`domain/domain.lang:584`) is 8 limbs below 2^32. Subsection `Witness format`. |
| `frInst W rd` | LLZK words: `llzkWords` (`domain/domain.lang:952`), the words of `exportLLZK`. Field id 2, element size 8. Subsection `LLZK words`. |
| `frInst W rd` | Halo2 words: `h2Words` (`examples/halo2.lang`). Field id 2, element size 8. Subsection `Halo2 words`. |

The export models map `quantize`, `softmax`, `layerNorm` and `attention`
to zero-output blocks: LLZK (`domain/domain.lang:839`), Halo2
(`domain/domain.lang:977`) and Plonky3 (`domain/domain.lang:1059`). This
agrees with the BN254 runtime for LLZK and Halo2. For Plonky3, a nonzero
BabyBear result for any of these operators fails the AIR's zero constraint
(`src/plonky3.c:611`).

A definition of a `PathP` type is not a Wasm entry. Proof contents within
an entry erase, while forcing a proof preserves its trap flag
(`formers/tcc-wasm.md:10`, `src/front/lower.c:1242`,
`test/ir/proof-trap.lang:1`). A `transp` that stays unreduced lowers to
its payload `u`; the line and `r` erase because the core has no Glue
and no univalence
(`src/front/lower.c:1312`).

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

### LLZK words

An LLZK entry (slice Z8, D10) is a words entry that `llzkWords` in
`domain/domain.lang` gives. Its words are the LLZK words of one module.
`build/llzkprint` (`src/llzk.c`) reads the words on stdin, as the one
line that `langc eval` prints. It writes the module as LLZK text on
stdout.

| Part | Words | Value |
|---|---|---|
| Version | 1 | 1. |
| Field id | 1 | 1 for BabyBear, 2 for BN254. `llzkWords` gives 2. |
| Element size e | 1 | 1 for BabyBear, 8 for BN254. |
| Input count n | 1 | The count of the inputs. |
| Weight count m | 1 | The count of the weights. |
| Output count q | 1 | The count of the outputs. |
| Instruction count | 1 | The count of the instructions. |
| Instructions | 1 block each | A tag, then its operands. |

| Tag | Instruction | Operands |
|---|---|---|
| 1 | const | k: e words, low limb first. |
| 2 | add | i j |
| 3 | sub | i j |
| 4 | mul | i j |
| 5 | out | o i: the output o gets the value i. |
| 6 | eq | i j |
| 7 | refuse | r: the refusal code. Code 1 is lookup (flag 69 (a)). |

- The value numbers 0 .. n - 1 are the inputs, and n .. n + m - 1 are
  the weights. Each const, add, sub, mul and out gives the next value
  number. The value of an out is the member `@o` of the output o.
- The module (flag 70) has one struct `@Main` with a public member `@o`
  for each output. `@compute` writes the members. `@constrain` reads
  them and has the `constrain.eq` constraints. The inputs are public
  arguments, and the weights are private arguments (O4).
- `llzkWords` puts one out and one eq for each output entry at the end.
  The lookup op gives `refuse 1` (flags 69 (a) and 73 (a)). The zero op
  gives one const 0 for all the output entries.

The printer checks all of the words before it prints. Each word is a
decimal number below 2^32. The version is 1. The field id is 1 with
element size 1, or 2 with element size 8. Each const is below the field
modulus: p = 2013265921 for BabyBear, the BN254 scalar modulus r for
BN254. The output count is not more than the instruction count. Each
tag is known, and each instruction has all of its operands. Each
operand is a value number that an earlier instruction gives. Only an eq
can read the value of an out. Each output number is below the output
count, and the words write each output one time. No word follows the
last instruction.

A refusal exits 1, writes the one line `llzkprint: MESSAGE` on stderr
and writes nothing on stdout. The messages are: `the block is short`,
`the version is not 1`, `the field id is not 1 or 2`, `the element size
does not agree with the field id`, `the tag is not known`, `a const is
not below the field modulus`, `a value number is not defined yet`, `an
out value is an operand of an op that is not eq`, `an output number is
not below the output count`, `an output is written twice`, `an output
is not written`, `lookup is refused (flag 69 (a))`, `the refuse code is
not known`, `the block has extra words`, `a word is not a decimal
number below 2^32` and `no memory`.

`make check` compares the printer output of 6 cases with
`test/llzk/*.mlir` and checks 9 refusals. It needs no LLVM tool.
`make zk-check` runs `llzk-opt` (LLZK 2.1.2) on the modules (see
`docs/VALIDATION.md`).

### Halo2 words

`build/halo2print NAME DIR` (`src/halo2.c`, slice Z9c) reads the words
of one Halo2 entry on stdin. `h2Words` (`examples/halo2.lang`) gives the
entry, and `langc eval` prints the words. The words are 4 blocks on one
line:

1. The circuit: the LLZK words of this section (`LLZK words`). The field
   id is 2 and the element size is 8, because Halo2 is BN254 only
   (flag 79).
2. The matrix map: the matmul count c, then the matrix index of each
   matmul in run order.
3. The inputs: n, then n elements.
4. The Z7 witness (`frWitnessWords`, `Witness format`): the version 1,
   the field id 2, the element size 8, the matrix count, the words of
   each matrix, the stage count, then the words of each stage.

An element is 8 limbs of 32 bits, low limb first. The printer checks
NAME (a to z and 0 to 9) and all of the words first. The circuit block
has the checks of `llzkprint`. Each matrix index is below the matrix
count. The mapped matrix entries are the weight count. The input count
is the circuit input count. The entry count of the last stage is the
output count. Each element is below r. Only the last stage maps to
circuit cells (O13).

A refusal exits 1, writes one message line on stderr and writes no
file. A write that fails is a refusal. A refusal keeps an existing
crate: the printer writes both files in the stage directory
`DIR/.halo2print` first, and keeps a backup of each old file until both
renames succeed. The messages include `the field id is not 2`, `a matrix
index is not below the matrix count`, `the mapped matrix entries are not
the weight count`, `the input count is not the circuit input count`,
`the last stage entry count is not the output count`, `an element is not
below the field modulus`, `a word follows the witness block`, `lookup is
refused (flag 69 (a))` and `the name is empty or has a character that is
not a to z or 0 to 9`.

An accepted entry exits 0, prints nothing and writes the crate
`zkml-halo2-NAME`: `DIR/Cargo.toml` (one dependency, `halo2_proofs`) and
`DIR/src/lib.rs`. The tests of `lib.rs` run MockProver (D7):
`accepts_z7_witness` must pass, `rejects_mutated_output` (output entry 0
plus 1) must fail, and for an entry with weights `rejects_mutated_weight`
(weight entry 0 plus 1) must fail.

### Plonky3 words

`build/plonky3print NAME DIR` (`src/plonky3.c`, slice Z10c) reads the
words of one Plonky3 entry on stdin. `p3Words` (`examples/plonky3.lang`)
gives the entry, and `langc eval` prints the words. The words are 3
blocks on one line:

1. The stage program: the stage count t, then one record for each stage
   in run order: `1 k s` (matmul: the matrix index k, the source s), `2`
   (zero), `3 s u` (residual: the residual input s, the inner result u)
   and `4` (lookup). A source is 0 for the inputs, or j for stage j. The
   first stage is stage 1.
2. The inputs: n, then n elements.
3. The Z7 witness (`bbWitnessWords`, `Witness format`): the version 1,
   the field id 1, the element size 1, the matrix count, the words of
   each matrix, the stage count, then the words of each stage.

An element is one word below the BabyBear modulus p = 15 * 2^27 + 1.
Plonky3 is BabyBear only (flag 90), so the field id is 1. The printer
checks NAME (a to z and 0 to 9) and all of the words first. Each tag is
known. Each source is below its stage number. Each matrix index is below
the matrix count. The witness stage count is t. The shape of each matmul
and each residual agrees with the entry counts of its stages. Each
element is below p.

A refusal exits 1, writes one message line on stderr and writes no
file. A write that fails is a refusal. A refusal keeps an existing
crate, as in `halo2print` (the stage directory is `DIR/.plonky3print`).
The messages include `the field id is not 1`, `the witness stage count
is not the stage count`, `a stage tag is not known`, `lookup is refused
(flag 69 (a))`, `a source is not below its stage number`, `a matrix
index is not below the matrix count`, `an element is not below the field
modulus`, `a word follows the witness block` and `the name is empty or
has a character that is not a to z or 0 to 9`.

An accepted entry exits 0, prints nothing and writes the crate
`zkml-plonky3-NAME`: `DIR/Cargo.toml` (the p3 crates at Plonky3 rev
5fcf8d7c, flag 87 (a)) and `DIR/src/lib.rs`: the AIR of the D5 layout
(one trace row holds the inputs, the weights and the entries of each
stage), the trace (`HEIGHT` copies of the row, with `HEIGHT` 1) and the
D7 tests. Each stage is bound, so the
tests check each stage, not only the last. The tests prove and verify
with the uni-stark test config (BabyBear, Poseidon2, flag 95 (a)):
`accepts_z7_trace` must pass, `rejects_mutated_output` (an entry of the
last stage plus 1) must fail, for an entry with weights
`rejects_mutated_weight` (weight entry 0 plus 1) must fail, and for an
entry with an inner stage (residual2 and seq2) `rejects_mutated_stage`
(an entry of stage 1 plus 1) must fail. In the dev profile, `prove` runs
the debug constraint check first, and a failed constraint panics there.

## 8. Host and target

The host is TinyCC 0.9.28rc, with the tcc-wasm kit. `PIN` records the
`tcc` version (`tcc version 0.9.28rc 2026-09-04 mob@0fb54300`), node
v23.10.0 and wabt 1.0.39 (`wasm-validate`, `wasm-objdump`). The target is
a Wasm 1.0 module, the witness runtime. The circuit forms are LLZK, Halo2
and Plonky3 (section 7).

These facts of `probe/CAPABILITY.md` set the design:

- A program is one file over the prelude, with no import. A definition
  uses only earlier definitions (sections 1, 2 and 7).
- Each loop is a `fold`, or an `unfold` with a `Nat` step limit. No other
  recursion checks (section 3).
- Each number is a 64-bit `Nat` word. `natAdd` and `natMul` trap on an
  overflow, `natSub` stops at 0, and `natDiv` and `natMod` trap on 0. The
  prelude makes each field element from `Nat` words (section 4).
- A proof erases, and a value at run time is first-order. An entry with
  a `Nat` or `Flag` result gives that value. An entry with a `List Nat`
  result is a words entry: `node test/run-wasm.mjs` prints its address,
  and `OUT - ABI` decodes the words (section 5).
- The cost is small at this scale: at N = 1000, each step (check, build,
  run) takes less than 0.1 s and less than 55 MiB (section 6).
- `refl` proves the eta laws of Sigma and Prod (section 8). `Eq` and
  `Path` work at each index type (section 9).
- A checked file has no axiom. `natIsSet` is the one trusted primitive,
  and the lowering refuses it at run time (section 10).

The checks on the output:

- `make check` (`Makefile:66`) runs the gate of the compiler and the
  printers. `docs/host/CAPABILITY.md` gives its counts.
- `make zk-check` (`Makefile:81`): `llzk-opt` of LLZK 2.1.2 must accept
  each LLZK module.
- `make halo2-check` (`Makefile:116`): `cargo test` of each Halo2 crate
  (halo2_proofs v0.3.0, offline). The MockProver must accept the witness.
- `make plonky3-check` (`Makefile:146`): `cargo test` of each Plonky3
  crate (Plonky3 rev 5fcf8d7c, the uni-stark test config, offline).

The three tool checks run outside the sandbox (flag 81). There is no
axiom report: the checker refuses `axiom` (rule R2), thus a checked file
has no axiom (`probe/CAPABILITY.md` section 10).

## 9. Open items

Each item ends with its status at the close: RULED (the USER ruled it),
or Default, built (the build uses the default, and the USER can still
rule it). "Probe-forced" marks a fact that a probe found. The numbers O3
to O8 are not used: `docs/PLAN.md` section 6 has its own items O1 to O8,
and the other docs cite O9 to O14 by number, thus the numbers stay.

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
  The flags of this item are RULED. Close: Default, built.
- O2. The order of composition in `Category` (USER flag 7, N9). The
  design sets `LayerCat ._⋆_ = _∘_`, but `⋆` in the Cubical library is
  diagrammatic: `f ⋆ g` is `f`, then `g`. Thus `catSeq` takes the order
  of `∘`: `catSeq a b c g f` is `g` after `f`, as in `layerSeq`. With
  this order, `LayerCat` is well typed. The check row
  `test/check/cat-seq-order.lang` shows that the diagrammatic order is
  refused. Status: built in slice Z5c. RULED 2026-10-16 (USER): `catSeq`
  keeps the order of `∘`. No code change. Close: RULED.
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
  Z6b. Close: RULED.
- O10. No empty type and no `Unit` eta (flag 18). The kit has no Empty
  former, because a family needs at least one constructor. `Unit` has
  no eta and no eliminator, so `isProp Unit` does not check. Part Z6c
  uses stand-ins: `Void` is `Path Nat 0 1`, `voidElim` is a `transp`
  over a `Nat` motive fold, and the code of nil and nil is
  `Path Nat 0 0`. Default (i): keep the stand-ins, with no kit change.
  RULED 2026-10-17 (USER): (i) (flag 18). Status: built in part Z6c.
  Close: RULED.
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
  format`) gives the witness words. Close: RULED.
- O12. The LLZK target (slice Z8, flags 65 to 75). RULED 2026-10-22 and
  2026-10-23 (USER). The guest model `LlzkMod` and the functor
  `exportLLZK` give the words, and the host printer `build/llzkprint`
  writes the text (flag 68 (c)). The lookup op is refused (flags 69 (a)
  and 73 (a)). `constrain.in` is the route for a later slice.
  `llzkWords` gives BN254 words only. The printer also takes BabyBear
  words. Status: built in slice Z8. Section 7 (`LLZK words`) gives the
  format. Close: RULED.
- O13. The Halo2 target (slice Z9, flags 76 to 86). RULED 2026-10-23
  and 2026-10-24 (USER). The guest entries `h2Words`
  (`examples/halo2.lang`) give the words, and the host printer
  `build/halo2print` writes a Rust crate for each entry (flag 78 (a)).
  Halo2 is BN254 only (flag 79). Only the last trace stage (the `frRun`
  result) maps to circuit cells, the out cells. The other stages have no
  cell, so the MockProver tests do not check them (G9, D7 (a)). The
  lookup op is refused: the circuit block gives `lookup is refused
  (flag 69 (a))`, as in `llzkprint`. `make halo2-check` runs the crate
  tests outside the sandbox (flag 81). Status: built in slice Z9.
  Section 7 (`Halo2 words`) gives the format. Close: RULED.
- O14. The Plonky3 target (slice Z10, flags 87 to 95). RULED (USER).
  The guest entries `p3Words` (`examples/plonky3.lang`) give the words,
  and the host printer `build/plonky3print` writes a Rust crate for each
  entry (flag 91 (a)). Plonky3 is BabyBear only (flag 90). Each trace
  stage is bound in the AIR, so the tests check each stage. The lookup op
  is refused: the stage program gives `lookup is refused (flag 69 (a))`.
  The crates use Plonky3 rev 5fcf8d7c (flag 87 (a)) and the uni-stark
  test config (flag 95 (a)). `make plonky3-check` runs the crate tests
  outside the sandbox (flag 81). Status: built in slice Z10. Section 7
  (`Plonky3 words`) gives the format. Close: RULED.

## 10. Milestones

| Milestone | Slices | Content |
|---|---|---|
| M0 | Z0 to Z4 | The seed; the core types construct and check; the refusal list; the cubical formers C1 to C5 (paths, the Kan operations, higher inductive types, quotient inductive-inductive types); examples and tests |
| M1 | Z5 and Z6 | The core operations of `design/DESIGN.md`: `Nat` and `List` induction (C6, C7), `Resource`, `Tensor`, `Inst`, `Layer`, `Category`, and the BabyBear and BN254 instances with their op runs |
| M2 | Z7 to Z10 | The targets: the Wasm witness runtime (Z7), LLZK (Z8), Halo2 (Z9) and Plonky3 (Z10), each with its check (section 8) |
| M3 | ZL and Z11 | Hardening: the lazy checker and the matmul case of `frOpRun` (ZL); the host probe, the final docs and the final gate (Z11) |

Status 2026-10-24: M0, M1 and M2 are DONE. M3 is the current milestone:
slice ZL is DONE, and slice Z11 closes the plan.

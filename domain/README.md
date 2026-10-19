# Domain slot

Replace the sample domain with the design of your language.
`domain/domain.lang` holds the domain. At build time, `tcc -run
gen/embed.c` copies it into `build/domain.c`, so the domain is part of
`build/langc`. `langc` loads the domain first, then the program.

## What the file holds

- `family` declarations. Only this file can declare a family. In a
  program, `family` is refused with `REFUSE_DATA` (rule R1).
- `def` operations, written in the language.
- Line comments that start with `--`.

Each family, constructor, field and definition name in this file becomes
a core name. A program cannot use one of them as a new definition name
(rule R4, `REFUSE_NAME`).

## Family syntax

```
family NAME (param : T)* := ctor (field : T)* | ctor (field : T)* ...
```

- A family with only nullary constructors is an enum.
- A family with one constructor is a record. It gets one projection for
  each field name.
- A field can have the type of its own family. Such a field is recursive.
- A parameter is an index (former F15). A constructor takes the indices
  first: `makeLot 3 250 : Lot 3`.
- A `fold` over a family takes one argument for each constructor, in
  declaration order. A constant constructor takes a value. A constructor
  with fields takes a function. A recursive field arrives folded.

In the IR, a constructor value is a heap block. Word 0 is the constructor
index, in declaration order. Each field has a word and a trap flag.
A `fold` over a family with one recursive field lowers to a loop.
A family with two or more recursive fields cannot be folded at run time
(`REFUSE_RUNTIME_TREE`, see `docs/CAPABILITY.md`).

## The sample

| Family | Kind | Use |
|---|---|---|
| `Color` | enum: `red`, `green`, `blue` | `isRed`, `colorCode` in `examples/algebra.lang` |
| `Item` | record: `itemColor`, `itemPrice`, `itemCount` | `lineTotal`, `total`, `redTotal` |
| `Stack` | one recursive field, `rest` | `depth`, and the spine loop in `test/ir/spine.lang` |
| `Tree` | two recursive fields | the tree mutant `test/ir/tree.lang` |
| `Lot (size : Nat)` | indexed record | `examples/inventory.lang`, `examples/laws.lang` |

The definitions are `lineTotal`, `isRed`, `total`, `redTotal` and `depth`.

`family Tree` exists only for the tree mutant. The mutant checks that the
lowering refuses a fold over a run-time tree. Rule R1 lets only this file
declare a family, so the family must be here.

## The ZkML core (slice Z5)

The file also holds the core of the language. The examples and the
tests use these names, so keep them when you replace the sample.

| Name | Kind | Use |
|---|---|---|
| `natAddAssoc`, `natMaxAssoc` | `Nat` lemmas, by induction | `circCount` in `examples/nat.lang`, `resAssoc` |
| `Resource` | record: `resGates`, `resLookups`, `resDegree` | the cost of a layer |
| `zeroR`, `resAdd`, `resUnitL`, `resUnitR`, `resAssoc` | the `Resource` monoid and its laws | `examples/resource.lang` |
| `Monoid`, `ResourceMonoid` | record of a monoid in `Type 0`, and the `Resource` instance | `examples/resource.lang` |
| `Tensor` | `Type 0 -> Shape -> Type 0`, a `List` level for each dimension | the `Inst` fields |
| `isSet`, `isProp`, `isPropPi`, `isPropPathInSet`, `isPropTrunc`, `isPropPathP` | the h-level library | `layerSoundProp`, `layerCompleteProp`, `examples/hit.lang`, `examples/qiit.lang`, `examples/rose.lang` |
| `Inst` | record of a backend: `instF`, `instWitness`, `instWeights`, `instSat`, `instEval`, `instInputWit` and their laws | the parameter of `Layer` |
| `Layer (I : Inst) (inS outS : Shape)` | record: `layerCircuit`, `layerDen`, `layerSound`, `layerComplete`, `layerRes` | `examples/layer.lang` |
| `layerWit` | the completeness witness of a layer (a Sigma) | the `layerComplete` field |
| `idL`, `seqDen`, `soundnessSplit`, `soundnessSeq`, `seqWitness`, `completenessSeq`, `layerSeq`, `resourcesSeq` | the identity layer and the sequential composition | `examples/layer.lang` |
| `layerSoundT`, `layerCompleteT`, `layerSoundProp`, `layerCompleteProp`, `layerExt` | the field types, their h-level, and a Path in `Layer` from Paths of the circuit, the denotation and the resources | `layerIdl`, `layerIdr`, `layerAssoc` |
| `layerIdl`, `layerIdr`, `layerAssoc` | the category laws of `Layer` | `examples/layer.lang` |
| `isSetPi`, `isSetResource`, `isPropIsSet`, `isPropSquareP` | more of the h-level library: a Pi of sets is a set, `Resource` is a set, a proposition is a set, a square of propositions has a filler | `isSetHom` |
| `isPropProd`, `Void`, `isPropVoid`, `voidElim` | a product of propositions is a proposition; `Void` is `Path Nat 0 1` (flag 18), a proposition with a map to each type | `listCode`, `isPropListCode` |
| `listCode`, `listCodeRefl`, `listEncode`, `listDecode`, `listDecodeRefl`, `listDecodeEncode`, `isPropListCode` | the encode-decode code of `List A`: a path of two lists is a path of the heads and a code of the tails | `isSetList` |
| `isSetList` | `List A` is a set when `A` is a set | `isSetTensor` |
| `isSetTensor` | `Tensor F s` is a set when `F` is a set | a concrete `Inst` (part Z6d) |
| `TensorL`, `listRep` | the `Tensor` at a list of dimensions; a list of `n` copies of a value | the tensor operations |
| `listUncons`, `listView` | the head and tail as an option; the empty or nonempty case with an equality for transport | runtime zip and stage decoding |
| `zeroT`, `mapT`, `listZipWith`, `zipWithT`, `sumT` | the tensor operations: one value at each entry, a map, a zip, and a sum with an `add` argument | a concrete `Inst` (part Z6d) |
| `Stage`, `stageEnc`, `tensorConv`, `stageDec`, `tensorConvSame`, `stageDecEnc` | a stage of a trace (a shape and a tensor), the read of a stage at a shape, and the round trip | `bbInst` (part Z6d) |
| `stageLast` | the last stage of a trace, or none for the empty trace | `bbInstEval` |
| `bbP`, `bbAdd`, `bbSub`, `bbMul`, `bbNeg` | the BabyBear field operations on a Nat below p = 2^31 - 2^27 + 1 | `examples/babybear.lang` |
| `bbRun`, `bbInstSat`, `bbInstEval`, `bbInstInputWit`, `bbInstSeqWit`, the other `bbInst` fields, `bbInst` | the BabyBear `Inst` at a weights type `W` and a reader `rd : W -> BbWeights` (part Z6f, flag 23): a witness is the weights and a trace, and satisfaction is a path from `bbRun` at `rd` of the weights to the read of the last stage. `bbRun a b f ws x` takes the weights `ws` before the input `x` | `examples/babybear.lang` (flag 15 closed) |
| `Op (a b : Shape)` | the operators: `opQuantize (qzS : Shape) (qzK : Nat) : Op qzS qzS` divides each entry by 2^k (part Z6e); `opMatMul (mmN mmM mmK : Nat) : Op [mmN] [mmM]` multiplies the input by the matrix mmK of the weights (part Z6f, flag 24); `opLookup (lkS : Shape) (lkT : Nat) : Op lkS lkS` reads each entry in the table lkT of the weights, and `opSoftmax (smS : Shape) (smT smK : Nat)` and `opLayerNorm (lnS : Shape) (lnG lnT : Nat)` (both `Op s s`) operate on each row of the last dim (part Z6f, flags 26 to 28); `opAttention (atH atD atQ atK atV atT atE : Nat)` maps [h, d] to [h, d] with one head (part Z6g, O5) | `circOp` |
| `circOp`, `circResidual` | the `Circuit` constructors for one operator and for a residual step x + f x (part Z6e) | `bbRun`, `cost` |
| `pow2` | 2 to the power k, by `Nat` induction; exponents at least 64 overflow `Nat` | arithmetic helper |
| `bbOpRun` | the function of an operator at the weights, by an `Op` fold; quantization uses at most 64 halvings, so any shift at least 64 gives zero; `opMatMul` reads its matrix with `listAt` and gives nil when the index is out of range; `opLookup` gives 0 for a key that is not in the table; `opSoftmax` and `opLayerNorm` use `mapLastT`; `opAttention` reads W_Q, W_K and W_V (the matrices atQ, atK and atV of the weights) and the exp table atT, and uses `bbAttention` with the softmax k atE (flags 31 and 32) | `bbRun` |
| `shapeSize` | the number of entries at a shape (the product of the dims) | `opRes`, `cost` |
| `opRes` | the resources of an operator: `opQuantize` uses one linear gate and one range lookup for each entry; `opMatMul n m k` uses n m gates of degree 2 and no lookups; `opLookup` uses one lookup for each entry; `opSoftmax` uses 2 gates and 3 lookups for each entry and 1 gate for each row; `opLayerNorm` uses 4 gates for each entry and 2 gates and 3 lookups for each row (flag 29); `opAttention` uses 3 h d d + 2 h h d + 2 h h + h gates and 3 h h lookups (flag 35) | `cost` |
| `cost` | the resources of a circuit, by a `Circuit` fold (N5); `circResidual` adds one linear gate for each entry | `costLayer`, `examples/babybear.lang` |
| `costLayer`, `costLayerRes` | a layer with the cost of its circuit as its resources, and the path from its `layerRes` to `cost` | `examples/babybear.lang` |
| `BbWeights` | the weights of `bbRun`: a list of matrices, and a matrix is a list of rows (part Z6f, flag 22) | `bbRun`, `bbInst` |
| `listAt` | the entry k of a list, or a default when the list is too short; a `Nat` fold drops k entries and a `List` fold takes the head | `bbOpRun` |
| `bbDot`, `bbMatVec` | the BabyBear dot product of two lists (it stops at the shorter list), and one dot product for each row of a matrix | `bbOpRun` |
| `bbNoWeights` | the reader that gives no matrices, for a weights type without `opMatMul` (flag 23) | `examples/babybear.lang` |
| `Fr`, `frMk`, `fr0` to `fr7` | the BN254 scalar field (part Z6h, flag 38): a record of 8 `Nat` limbs below 2^32, little-endian, and its projections. r = 21888242871839275222246405745257275088548364400416034343698204186575808495617 | `frToL`, `isSetFr` |
| `frToL`, `frOfL` | the limb list of an element and the element of a limb list; `listAt` reads 0 past the end | `frAdd`, `frSub`, `frMul`, `frBad` |
| `frMacRow` | ts + u * xs + c, limb by limb, in CPS: the result is the carry-out and the limbs | `frAddL`, `frMont` |
| `frSubRow` | xs - ys with a borrow chain: the result is the borrow-out (1 when xs < ys) and the limbs | `frReduce`, `frSubL` |
| `frReduce` | t - r when t >= r (the borrow of `frSubRow` is 0), else t | `frAddL`, `frMulL` |
| `frMont` | Montgomery CIOS (flag 39): a * b * 2^-256 mod r in 8 rounds, each `frMacRow`, then `frMontRed`, then `frMontShift`; `frMulL` applies it twice, the second time with 2^512 mod r (`frR2L`), for the plain form | `frMulL` |
| `frAdd`, `frSub`, `frMul`, `frNeg`, `frZero`, `frOne` | the field operators on `Fr`, for canonical inputs (flag 40); `frNeg a` is 0 - a | `frRun`, `examples/bn254.lang` |
| `frAtLeastR` | 1 when the limbs compare at or above r, otherwise 0; compares from the most significant end and accepts full-width `Nat` limbs without overflow | `frBad` |
| `frBad`, `frCanonB`, `frCanon` | the count of the limbs at 2^32 or above, plus `frAtLeastR`; the `Flag` that the count is 0; the type of a proof of it (flag 40). `frInstSat` has no `frCanon` clause: the `Inst` family quantifies `instInputSat` over every x | `examples/bn254.lang`, `test/check/fr-canon-r.lang` |
| `isSetFr` | `Fr` is a set: a square in `Fr` is a record of squares in `Nat` (`natIsSet`) | `frTensorSet` |
| `frTensorSet` | the `instTensorSet` of `frInst` | `frInst` |
| `FrWeights` | the weights of `frRun`: a list of matrices over `Fr` (the `BbWeights` form) | `frRun`, `frInst` |
| `frRun` | the structural cases of `bbRun` over `Fr` (flag 41): `circResidual` adds x with `frAdd`, and an operator gives `zeroT Fr frZero` (the BN254 operators come later) | `frInstSat`, `examples/bn254.lang` |
| `frInst`, `frInstSat`, `frInstEval`, `frInstInputWit`, `frInstSeqWit`, the other `frInst` fields | the BN254 `Inst` at a weights type `W` and a reader `rd : W -> FrWeights`, in the form of `bbInst` (flag 42) | `examples/bn254.lang` |
| `frNoWeights` | the reader that gives no matrices over `Fr` | `examples/bn254.lang` |
| `circResidualLoop` | n residual steps of a circuit after `circId`, by `Nat` induction | `costResidualLoop`, `examples/babybear.lang` |
| `resScale` | n copies of a resource under `resAdd`, from `zeroR` | `costResidualLoop` |
| `costResidualLoop` | the path from the cost of n residual steps to `resScale n` of the cost of one step, for each n, by `Nat` induction | `examples/babybear.lang` |
| `shapeRows` | the number of rows of the last dim: the product of the other dims (1 at rank 0) | `opRes` |
| `mapLastT` | a row function on each row of the last dim of a tensor; at rank 0 the entry is a row of length 1 (flag 30) | `bbOpRun` |
| `listLen`, `listSumNat`, `listMaxNat` | the length, the `Nat` sum and the `Nat` max of a list | `bbSoftmaxRow`, `bbLnMean` |
| `bbTableRow`, `bbLookup` | read the key and value of a row; the value of the first complete row `[key, value]` with the key, or a default when none has the key. Rows missing a key or value are ignored by lookup and membership (flag 26) | `bbOpRun` |
| `bbTableHit`, `bbTableMem`, `bbTableIn` | 1 when a row of the table has the key, else 0; the membership of a key, a path in `Nat` (flag 18 (i)); the keys of the table with a membership proof | `examples/babybear.lang` |
| `bbSoftmaxDouble`, `bbSoftmaxScale`, `bbSoftmaxDiv`, `bbSoftmaxExp`, `bbSoftmaxNorm`, `bbSoftmaxRow` | the softmax of a row: the table at the max minus x (`natSub`) for each entry, then each entry times 2^k divided by the sum (a sum of 0 reads as 1). `bbSoftmaxDiv k t e` and `bbSoftmaxNorm k e` take the exponent k. Quotient and remainder doubling avoids intermediate product overflow, including when k is at least 64 and the quotient still fits `Nat` (flag 27) | `bbOpRun` |
| `bbLnMean`, `bbLnCenter`, `bbLnVar`, `bbLnAffine`, `bbLnScaled`, `bbLayerNormRow` | the layerNorm of a row: the mean (`natDiv`), c = x minus the mean in the field, the variance, r = the table at the variance (the inverse square root), and gamma c r plus beta (flag 28) | `bbOpRun` |
| `bbAttnShift` | (p - 1) / 2: each score gets it before the softmax, so the `Nat` order of the shifted scores is the signed order of the scores (flag 33) | `bbAttnRow` |
| `bbAttnMix` | the sum of a_j V_j over the rows of V, from a zero row of length d | `bbAttnRow` |
| `bbAttnRow` | one output row: the softmax of the shifted scores K_j . q, then the mix; the output keeps the 2^k scale of the softmax (flag 34) | `bbAttention` |
| `bbAttention` | one head on the rows of x: Q, K and V by `bbMatVec` on each row, then `bbAttnRow` for each row of Q | `bbOpRun` |
| `layerExt2` | a square in `Layer` from squares of the circuit, the denotation and the resources | `isSetHom` |
| `isSetHom` | `Layer I a b` is a set (N4) | `LayerCat` |
| `Category` | record: `catOb`, `catHom`, `catId`, `catSeq`, `catIdl`, `catIdr`, `catAssoc`, `catHomSet` | `LayerCat`, `TypeFunctor` |
| `LayerCat (I : Inst)` | the category of layers: the objects are shapes, the morphisms are layers | `examples/category.lang` |
| `TypeFunctor (C : Category)` | record of a functor from `C` to the types: `tfOb`, `tfHom`, `tfId`, `tfSeq` | `denote` |
| `denote` | `(I : Inst) -> instWeights I -> TypeFunctor (LayerCat I)`: `Tensor (instF I)` on the shapes, `layerDen L ws` on the layers | `examples/category.lang` |

`layerSeq I a b c L2 L1` is `L2` after `L1`. Its resources are
`resAdd (layerRes L2) (layerRes L1)`.
`instSeqWitSat` requires a path between the weights of the component
witnesses. `seqWitness` supplies it from their common weights `ws`.
`catSeq` takes the same order: `catSeq C a b c g f` is `g` after `f`
(flag 7, `SPEC.md` O2).

The domain has no `isSetCircuit` definition. `isSetHom` uses
`circSquash` directly. `squashIsSet` in `examples/qiit.lang` has the
same statement.

## How to replace the domain

1. Write your families and definitions in `domain/domain.lang`. Keep the
   names apart from the core names in `README.md`.
2. Change the examples and the tests that use the sample names:
   `examples/algebra.lang`, `examples/inventory.lang`,
   `examples/laws.lang`, `test/eval/expect.txt`,
   `test/check/domain-name.lang`, `test/ir/spine.lang` and
   `test/ir/tree.lang`.
3. Keep a family with two recursive fields for `test/ir/tree.lang`, or
   remove that file and its block in `test/gate.sh`.
4. Run `make check` in the kit directory. The build makes `build/domain.c`
   again when `domain/domain.lang` changes.

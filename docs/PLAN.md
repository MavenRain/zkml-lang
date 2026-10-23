# zkml-lang plan

## 1. Rulings

- The language uses only the base types and operations of the Cubical Agda
  fragment in `design/DESIGN.md`. The laws of the fragment are a minimal
  base, not a full list. The language can state and prove more laws about
  the same operations.
- Type formers: products, coproducts, dependent types, monads (map, bind),
  algebras and coalgebras (fold, unfold), quotients, paths, loops, HITs and
  QIITs, all as in cubical type theory.
- Host: TinyCC.
- Targets (USER 2026-10-15, all four): LLZK IR, Halo2, Plonky3 and a Wasm
  witness runtime.
- Tests: unit and integration tests are in the guest language. A host
  runner that loads and runs guest test files is permitted. Do not add a
  node, Python or shell harness.

## 2. Design choices (not ruled)

- Name: zkml-lang, from the module name `ZkMLDSL`. The compiler binary is
  `build/langc`.
- Route: `bin/new-lang.sh zkml-lang tcc-wasm` in lang-template. The kit
  gives the C99 front end, the evaluator and the Wasm writer. The cubical
  formers (section 3) and the three circuit writers go into this
  repository. lang-template does not change (O1).
- Erasure: the checker checks all terms, paths and proofs included. An
  exported entry has a first-order type (Circuit, Tensor, Resource or a
  record of these). The compiler evaluates it to a closed first-order term.
  The kit refusal REFUSE_RUNTIME_CLOSURE stays. A new refusal,
  REFUSE_RUNTIME_PATH, stops an interval term or a path in an entry result.
- Field: `F`, `Witness`, `satisfies` and `eval` are module parameters of
  the design. Each instance gives them as definitions: the field, and the
  semantics of the Circuit IR on that field. Instances: BabyBear for
  Plonky3, the BN254 scalar field for Halo2, both for LLZK and Wasm (O2).
- Identity type: Path. The kit formers F12 and F13 (refl, symm, trans,
  transport, cong) become derived definitions over Path, so programs that
  use the kit names still check.
- Target categories: `Halo2CircuitCat`, `Plonky3CircuitCat` and
  `LLZKIRCat` are guest types, each a model of the target IR. Each export
  functor is a guest function, and F-id, F-seq and resources-export are
  guest Path proofs. A C writer prints each model as target text. The
  writers are in the trusted base. A differential gate tests each writer
  against the target tool.

## 3. Formers

The kit has F1 to F15 (`formers/FORMERS.md`). The design also needs:

- C1 Interval: `I`, `i0`, `i1`, `~`, `∧`, `∨`, face formulas, partial
  elements and systems.
- C2 PathP and Path: path abstraction, path application and the endpoint
  rules. A loop is a `Path x x`.
- C3 Kan operations: `transp` and `hcomp` (with `comp` and `fill`) for
  each former: Pi, Sigma, Path, universes, inductive types and records.
- C4 HITs: point constructors, path constructors, `hcomp` as a formal
  constructor, and eliminators with path cases. Examples: the circle
  (`base`, `loop`), the set quotient `A / R`, and the propositional
  truncation (the `∃` of the design).
- C5 QIITs: sorts that are defined together, with path constructors. The
  checker accepts a declaration of the finitary schema and gives its
  eliminator (O8).

Glue types and univalence are not in the list of the design (O3).

## 4. Notes on the fragment (defaults, not rulings)

The design stays verbatim. These notes record where a stated `refl` or a
type does not check as written, and the default for each.

- N1 Resource laws: Agda `+` and `max` recurse on the first argument. Thus
  `∙-unitˡ` is `refl`, but `∙-unitʳ` and `∙-assoc` are not. The guest
  proves them by induction on ℕ.
- N2 Category laws: `idL-unitˡ`, `idL-unitʳ` and `∘-assoc` are `refl` only
  when circuit composition is unital and associative by definition.
  Default: `Circuit` is a QIIT with the constructors `id` and `seq` and the
  path constructors `seq-idl`, `seq-idr`, `seq-assoc` and a set
  truncation. Then each export functor comes from the eliminator, F-seq is
  `refl` as the design states, and the eliminator obligations are the
  laws of the target category. `idL-unitʳ` also needs `∙-unitʳ` (N1).
- N3 Identity witness: `idL` completeness uses the input `x` as a
  witness, but `Witness` does not depend on the shape. Default: each
  instance gives `inputWitness : Tensor s → Witness` with
  `satisfies id x (inputWitness x)` and `eval id (inputWitness x) ≡ x`.
- N4 isSetHom: `Layer s t` is a set when `F` is a set. The denotation goes
  into a set, soundness is a family of paths in a set (a proposition),
  completeness is a truncation and resources is ℕ³. Each instance field has
  decidable equality, so the guest proves isSetHom without a truncation of
  Layer.
- N5 resources-export: `resources` is a free field of Layer, so
  resources-export does not hold for each Layer value. Default: a cost
  function `cost : Circuit a b → Resource` counts constraint gates,
  lookups and degree. This includes linear gates: quantization uses one
  gate and one range lookup per entry, and a residual adds one gate per
  entry to the cost of its inner circuit. `costLayer` sets resources to
  this cost, with `costLayerRes` as its proof. The guest proves
  `resources ≡ cost circuit` for sequential compositions of these layers
  and states resources-export for them.
- N6 Field operators: soundness makes `denotation x ≡ eval circuit w`, so
  the denotation of `softmax`, `layerNorm` and `quantize` is the field
  function that the circuit computes: fixed point values with scale 2^k,
  `exp` and the inverse square root through `lookup` tables, division
  through the field inverse with a range check. The error against real
  softmax is not in scope.
- N7 Weights: `matmul m n k` maps an m×k tensor to an m×n tensor, but no
  argument gives the k×n weights. Ruling O4: the weights are part of the
  witness. The denotation of the design has no witness argument, so the
  weights must reach it through the witness. Default: each instance gives
  `Weights` and `weightsOf : Witness → Weights`. A layer gets
  `denotation : Weights → Tensor inS → Tensor outS`, and soundness gets
  `denotation (weightsOf w) x ≡ eval circuit w`. Completeness quantifies
  over the weights too. Z5 records this change of the Layer type in
  `SPEC.md` section 9. The shapes of `attention` are not given (O5).

## 5. Slices and gates

- Z0: `bin/new-lang.sh zkml-lang tcc-wasm`, `design/DESIGN.md` (the design
  without change), this plan and the README introduction. Gate: kit
  `make check`.
- Z1: C1 and C2. F12 and F13 as derived definitions over Path. Guest
  tests: funExt and cong by definition, symm of symm by definition.
- Z2: C3. Guest tests: transport along refl, J, path composition and its
  laws.
- Z3: C4. Guest tests: circle recursion, quotient elimination into a set,
  truncation elimination into a proposition.
- Z4: C5. Guest tests: Circuit (N2) and one two-sort QIIT.
- Z5: Domain core: Shape, Tensor, Circuit, Resource with its monoid laws
  (N1), Layer, `∘`, `idL`, the category laws (N2), isSetHom (N4), LayerCat,
  `denote`, `resources-∘`, `soundness-∘`. Check that the kit Nat has an
  induction principle. If not, add one. Status: DONE 2026-10-16.
- Z6: The BabyBear and BN254 instances (N3), `cost` (N5) and the operators
  `matmul`, `softmax`, `attention`, `layerNorm`, `quantize`, `lookup`,
  `residual`, `residual-loop` (N6). O5 is RULED. Status: part Z6a (the
  `List` motive fold, flag 15) DONE 2026-10-16. Part Z6b (the split
  witness of `instSeqSat`, flag 16) DONE 2026-10-17. Part Z6c (the
  tensor library over the `List` fold) DONE 2026-10-17. Part Z6d
  (`natDiv`, `natMod` and the BabyBear `Inst`, flag 15 closed) DONE
  2026-10-17. Part Z6e (the `Op` family with `opQuantize`, the
  constructors `circOp` and `circResidual`, and `cost`) DONE
  2026-10-17. Part Z6f session 1 (`opMatMul`, the weights in `bbRun`,
  `circResidualLoop` and `costResidualLoop`) DONE 2026-10-18. Part Z6f
  session 2 (`opLookup`, `opSoftmax` and `opLayerNorm`) DONE 2026-10-18.
  Part Z6g session 1 (`opAttention`, one head) DONE 2026-10-18. Part
  Z6h session 1b (the BN254 scalar field `Fr`, `frCanon` and `frInst`)
  DONE 2026-10-19. Part Z6i session 1b (the BN254 operators: `frOpRun`
  in `frRun`, the lookup operator real, the matmul case `zeroT Fr
  frZero` under flag 50, and real since slice ZL) DONE 2026-10-19. Part
  Z6j session 1b (the lowering shares the values in each block, flags
  51 to 55) DONE 2026-10-20.
- Z7: Wasm witness runtime. An entry compiles to Wasm and makes the
  witness. The field arithmetic (31-bit, and 254-bit Montgomery) is guest
  code in `domain/domain.lang` (flag 57 (a)). `langc eval` and the Wasm
  writer run it through the `Nat` operations of the kit. The C evaluator
  and `src/wasm.c` have no field code. Gate: `langc eval` against Wasm,
  differential. Status: part Z7a (words entries, flag 56 (a)) DONE
  2026-10-21. Part Z7b (the BabyBear trace and its witness words, flag
  62 (a)) DONE 2026-10-21. Part Z7c (the BN254 trace and its witness
  words) DONE 2026-10-22. Part Z7d (the witness format in `SPEC.md` and
  the final gate) DONE 2026-10-22.
- ZL: Lazier checker (flags 50 and 64). `symm`, `trans` and `cong`
  quote a closure as its body applied to the quoted environment
  (`quote_lazy`). The conversion accepts two closures with the same body
  and convertible environments (`same_closure`). The matmul case of
  `frOpRun` is then `frMatVec` over the weights at `mmK` (flag 50).
  Flag 64 (a): `make check` builds `build/langc-256` (a 256 MiB arena)
  and checks the examples and the 4 `Fr` rows with it; the shipped
  `langc` keeps 1 GiB. Gate: `make check`. Status: part ZLa
  (`quote_lazy` and `same_closure`) DONE 2026-10-22. Part ZLb (the
  matmul case and flag 64) DONE 2026-10-22. Part ZLc (the docs) DONE
  2026-10-22. Slice ZL DONE 2026-10-22.
- Z8: LLZK. `LLZKIRCat`, `exportLLZK` with F-id, F-seq, resources-export.
  The writer prints LLZK MLIR text. Gate: `make zk-check`: `llzk-opt`
  accepts each module and rejects 2 mutated modules (O6). Status: part
  Z8a (the LLZK tools and the sample modules) DONE 2026-10-23. Part Z8b
  (the guest model and the export functor) DONE 2026-10-23. Part Z8c
  (the LLZK words and the printer) DONE 2026-10-23. Part Z8d
  (`make zk-check` and the docs) DONE 2026-10-23. Slice Z8 DONE
  2026-10-23.
- Z9: Halo2. `Halo2CircuitCat`, `exportHalo2`. The writer prints a Rust
  crate for each entry. Gate: MockProver accepts the Z7 witness and
  rejects a mutated witness.
- Z10: Plonky3. `Plonky3CircuitCat`, `exportPlonky3`. The writer prints an
  AIR and its trace. Gate: the Plonky3 debug constraint check accepts the
  Z7 trace and rejects a mutated trace.
- Z11: `SPEC.md`, `probe/CAPABILITY.md`, `docs/STATUS.md`,
  `docs/VALIDATION.md` and `README.md`. Final gate.
- Gate: `make check`. The target gates need Rust and LLVM tools, so they
  are `make zk-check`, outside the sandbox when needed. Each long run stays
  under 4 GB. Rust builds use `-j 2`.

## 6. Open items

- O1: Move C1 to C5 into lang-template as a kit change. Default: no, until
  Z4 is done and the USER asks.
- O2: Fields. Default: BabyBear and BN254. Goldilocks and Mersenne31 later.
- O3: Glue types and univalence. Default: out, because the design does not
  list them.
- O4: matmul weights (N7). RULED 2026-10-15 (USER): (b), the weights are
  part of the witness. The Layer type change is the N7 default.
- O5: attention shapes (N7). RULED 2026-10-16 (USER): the default.
  `attention` maps `shape (h ∷ d ∷ [])` to the same shape, with one head
  and the weights W_Q, W_K and W_V (d by d) in the witness (O4).
- O6: LLZK tools. `llzk-opt` needs an LLVM and MLIR build. Install
  APPROVED 2026-10-15 (USER). The build stays under 4 GB, or uses a release
  binary. Status: part Z8a built llzk-lib v2.1.2 (LLVM 20.1.8) with nix.
  `LLZK_OPT` in the `Makefile` gives the path of its `llzk-opt`.
- O7: Circuit as a QIIT (N2). The other option: `seq` pattern matches on
  `id`, so one unit law is `refl` and the others are proofs.
- O8: QIIT schema. Default: finitary QIITs (sorts indexed by earlier sorts,
  path constructors between point terms), as in Kaposi, Kovács and
  Altenkirch.

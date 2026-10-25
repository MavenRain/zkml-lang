# TinyCC Wasm realization

Paths below are relative to this host kit, or to the root of a generated
language. The template's `formers/FORMERS.md` gives the host-neutral laws.
`src/front/check.c` lines are rows of the builtin table. `src/front/lower.c`
lines are the lowering to the IR.

DONE means: `langc check` accepts the former, `langc eval` computes it,
and `langc build` lowers it to Wasm when its value is first-order at run
time. Types and universes erase on Wasm. Proof contents erase, while their trap flags remain.

| ID | Former | Status | Where | Evidence |
|---|---|---|---|---|
| F1 | Product | DONE | `src/front/check.c:110`, `src/front/lower.c:661`, `src/front/lower.c:1317` | `examples/formers.lang:3`, `examples/laws.lang:12` |
| F2 | Coproduct | DONE | `src/front/check.c:111`, `src/front/check.c:123`, `src/front/lower.c:1325` | `examples/formers.lang:8`, `examples/laws.lang:13` |
| F3 | Option | DONE | `src/front/check.c:115`, `src/front/check.c:124`, `src/front/lower.c:1327` | `examples/formers.lang:11`, `examples/formers.lang:12` |
| F4 | List | DONE | `src/front/check.c:116`, `src/front/lower.c:932` | `examples/formers.lang:13`, `examples/inventory.lang:4`, `test/ir/spine.lang:8` |
| F5 | Monad pure/map/bind | DONE over Option, Sum E and List | `src/front/check.c:125`, `src/front/lower.c:1172`, `src/front/lower.c:1187`, `src/front/lower.c:951`, `src/front/lower.c:966` | `examples/monad.lang:2`, `examples/monad.lang:6`, `examples/monad.lang:8`, `examples/laws.lang:8`, `test/ir/spine.lang:10` |
| F6 | Algebra fold | DONE over Nat (with the motive form, Z5a), List and domain families. HOST-LIMIT: a run-time tree | `src/front/check.c:129`, `src/front/check.c:1096`, `src/front/check.c:1180`, `src/front/eval.c:529`, `src/front/lower.c:1224`, `src/front/lower.c:932`, `src/front/lower.c:1123`, `src/front/lower.c:1148` | `examples/algebra.lang:4`, `examples/algebra.lang:3`, `examples/algebra.lang:6`, `test/ir/spine.lang:18`, `test/ir/tree.lang:3`, `domain/domain.lang:68`, `examples/nat.lang:20`, `examples/nat.lang:22`, `test/check/natadd-assoc-refl.lang:3`, `test/eval/expect.txt:58` |
| F7 | Algebra unfold | DONE into List with a Nat step limit | `src/front/check.c:130`, `src/front/lower.c:1038` | `examples/algebra.lang:7`, `examples/algebra.lang:8`, `test/ir/spine.lang:5` |
| F8 | Filter | DONE over Option and List | `src/front/check.c:128`, `src/front/lower.c:1198`, `src/front/lower.c:988` | `examples/monad.lang:9`, `test/ir/spine.lang:11` |
| F9 | Non-dependent Pi | DONE. HOST-LIMIT: a function value at run time | `src/front/check.c:489`, `src/front/lower.c:1383`, `src/front/lower.c:587` | `examples/functions.lang:2`, `examples/functions.lang:4`, `examples/laws.lang:5`, `test/ir/closure.lang:3` |
| F10 | Dependent Pi | DONE over `Type 0` and over Nat | `src/front/check.c:489`, `src/front/lower.c:1380` | `examples/functions.lang:8`, `examples/equality.lang:7` |
| F11 | Sigma | DONE | `src/front/check.c:117`, `src/front/check.c:121`, `src/front/lower.c:1318`, `src/front/lower.c:1321` | `examples/sigma.lang:3`, `examples/sigma.lang:5`, `examples/sigma.lang:6` |
| F12 | Eq refl/symm/trans | DONE | `src/front/check.c:118`, `src/front/check.c:131`, `src/front/check.c:132`, `src/front/lower.c:1287` | `examples/equality.lang:4`, `examples/equality.lang:5`, `examples/equality.lang:6`, `test/check/bad-refl.lang:1` |
| F13 | Eq transport/cong | DONE | `src/front/check.c:133`, `src/front/check.c:134`, `src/front/lower.c:1347`, `src/front/lower.c:1349` | `examples/equality.lang:8`, `examples/equality.lang:9` |
| F14 | Universes | DONE for Type 0 and Type 1, without cumulativity | `src/front/lower.c:1379` | `examples/universes.lang:4`, `test/check/universe.lang:1`, `test/check/universe-range.lang:1` |
| F15 | Generic indexed family | PARTIAL: only the domain file declares an indexed family | `domain/domain.lang:10` | `examples/inventory.lang:9`, `examples/laws.lang:16` |
| C1 | Interval | DONE | `src/front/check.c:147`, `src/front/check.c:150`, `src/front/check.c:151`, `src/front/check.c:152`, `src/front/eval.c:1141`, `src/front/lower.c:593`, `src/front/check.c:160`, `src/front/check.c:161`, `src/front/check.c:163`, `src/front/check.c:1723`, `src/front/eval.c:1205`, `src/front/eval.c:1294` | `examples/paths.lang:6`, `examples/paths.lang:7`, `examples/paths.lang:8`, `test/check/pretype.lang:2`, `examples/systems.lang:5`, `examples/systems.lang:6`, `examples/systems.lang:8`, `examples/systems.lang:10`, `test/check/system-overlap.lang:1`, `test/check/system-face.lang:1`, `test/check/from-partial.lang:2` |
| C2 | PathP and Path | DONE | `src/front/check.c:153`, `src/front/check.c:154`, `src/front/check.c:155`, `src/front/check.c:156`, `src/front/check.c:157`, `src/front/eval.c:1153`, `src/front/eval.c:2415` | `examples/paths.lang:4`, `examples/paths.lang:9`, `examples/paths.lang:12`, `examples/paths.lang:13`, `test/check/path-endpoint.lang:2`, `test/check/papp-nat.lang:2`, `test/ir/path.lang:4` |
| C3 | Kan operations | DONE: `transp`, `hcomp`, `comp` and `fill`; `transport` and `trans` over a path; the C3 notes give the limits | `src/front/check.c:165`, `src/front/check.c:1382`, `src/front/check.c:1803`, `src/front/check.c:1827`, `src/front/eval.c:1802`, `src/front/lower.c:1312`, `src/front/check.c:166`, `src/front/check.c:1928`, `src/front/eval.c:2071`, `src/front/lower.c:1307`, `src/front/check.c:167`, `src/front/check.c:168`, `src/front/check.c:1981`, `src/front/check.c:1268`, `src/front/eval.c:1498`, `src/front/eval.c:1955`, `src/front/eval.c:1733` | `examples/transp.lang:5`, `examples/transp.lang:9`, `examples/transp.lang:11`, `examples/transp.lang:15`, `examples/transp.lang:20`, `test/check/transp-const.lang:2`, `test/check/transp-line.lang:2`, `test/check/transp-start.lang:2`, `test/check/transp-name.lang:2`, `examples/hcomp.lang:7`, `examples/hcomp.lang:8`, `examples/hcomp.lang:9`, `examples/hcomp.lang:10`, `examples/hcomp.lang:11`, `examples/hcomp.lang:13`, `test/check/hcomp-face.lang:2`, `test/check/hcomp-base.lang:2`, `test/check/hcomp-side.lang:2`, `test/check/hcomp-name.lang:2`, `examples/kan.lang:8`, `examples/kan.lang:9`, `examples/kan.lang:10`, `examples/kan.lang:11`, `examples/kan.lang:12`, `examples/kan.lang:14`, `examples/kan.lang:17`, `examples/kan.lang:18`, `examples/kan.lang:21`, `test/check/comp-base.lang:2`, `test/check/comp-name.lang:2`, `test/check/fill-name.lang:2`, `test/check/trans-ends.lang:2`, `test/check/trans-line.lang:2` |
| C4 | Higher inductive types | DONE: declarations, path constructor values, the formal `hcomp`, the fold eliminator | `src/front/check.c:2894`, `src/front/check.c:2827`, `src/front/check.c:2712`, `src/front/check.c:2672`, `src/front/check.c:2175`, `src/front/core.h:175`, `src/front/eval.c:2104`, `src/front/eval.c:1971`, `src/front/eval.c:1996`, `src/front/eval.c:3031`, `src/front/lower.c:726`, `src/front/lower.c:1309`, `src/front/check.c:895`, `src/front/check.c:993`, `src/front/eval.c:2263`, `src/front/eval.c:3102`, `src/front/lower.c:1061` | `domain/domain.lang:23`, `domain/domain.lang:24`, `domain/domain.lang:26`, `examples/hit.lang:9`, `examples/hit.lang:10`, `examples/hit.lang:13`, `domain/domain.lang:168`, `examples/hit.lang:14`, `examples/hit.lang:15`, `examples/hit.lang:16`, `examples/hit.lang:19`, `test/check/hit-program.lang:2`, `test/ir/hit-path.lang:5`, `examples/hit.lang:20`, `examples/hit.lang:21`, `examples/hit.lang:22`, `examples/hit.lang:24`, `examples/hit.lang:33`, `test/check/hit-fold-path.lang:2`, `test/check/hit-fold-prop.lang:3`, `test/check/hit-fold-arity.lang:2` |
| C5 | Quotient inductive-inductive types | DONE: indexed sorts, the motive fold and the case shape D6 (Z4a); mutual blocks with one motive for each sort (Z4b); `Nat`-valued folds with `natIsSet` (Z5a) | `src/front/check.c:845`, `src/front/check.c:865`, `src/front/check.c:895`, `src/front/check.c:923`, `src/front/check.c:979`, `src/front/check.c:993`, `src/front/check.c:1170`, `src/front/check.c:2143`, `src/front/check.c:2185`, `src/front/check.c:2749`, `src/front/check.c:2964`, `src/front/check.c:3043`, `src/front/core.h:176`, `src/front/core.h:217`, `src/front/eval.c:594`, `src/front/eval.c:622`, `src/front/eval.c:663`, `src/front/eval.c:918`, `src/front/eval.c:2132`, `src/front/eval.c:2210`, `src/front/eval.c:2219`, `src/front/eval.c:2263`, `src/front/eval.c:3102`, `src/front/lower.c:786`, `src/front/lower.c:1085`, `src/front/lower.c:1105`, `src/front/lower.c:1123`, `src/front/lower.c:1135`, `src/front/parser.c:565` | `domain/domain.lang:48`, `domain/domain.lang:49`, `domain/domain.lang:50`, `domain/domain.lang:53`, `domain/domain.lang:59`, `domain/domain.lang:62`, `examples/nat.lang:33`, `examples/nat.lang:34`, `examples/qiit.lang:12`, `examples/qiit.lang:14`, `examples/qiit.lang:15`, `examples/qiit.lang:19`, `examples/qiit.lang:21`, `examples/qiit.lang:28`, `examples/rose.lang:6`, `examples/rose.lang:9`, `examples/rose.lang:13`, `examples/rose.lang:21`, `test/check/rose-count.lang:5`, `test/eval/expect.txt:34`, `test/eval/expect.txt:40`, `test/fronttool.c:218`, `test/fronttool.c:293`, `test/fronttool.c:297`, `test/ir/rose-tree.lang:5` |
| C6 | Nat conversion and record eta | DONE: the `natAdd` neutral rules R1 to R3, `natMax`, `natIsSet` and record eta (Z5a); the three `natMax` neutral rules (Z5b, flag 12). PARTIAL: `natMax` has no idempotence rule | `src/front/check.c:142`, `src/front/check.c:143`, `src/front/check.c:1450`, `src/front/eval.c:810`, `src/front/eval.c:874`, `src/front/eval.c:879`, `src/front/eval.c:883`, `src/front/eval.c:2484`, `src/front/eval.c:2494`, `src/front/lower.c:756`, `src/front/lower.c:1310`, `src/front/lower.c:1365` | `examples/nat.lang:14`, `examples/nat.lang:15`, `examples/nat.lang:16`, `examples/nat.lang:17`, `examples/nat.lang:18`, `examples/nat.lang:19`, `domain/domain.lang:70`, `examples/nat.lang:25`, `examples/nat.lang:26`, `examples/nat.lang:30`, `examples/nat.lang:31`, `examples/nat.lang:32`, `test/check/eta-hit.lang:4`, `test/check/eta-two-ctor.lang:3`, `test/check/natmax-neutral.lang:3`, `test/check/natmax-assoc-refl.lang:3`, `test/eval/expect.txt:45` |
| C7 | List induction | DONE: the `fold` eliminator with a motive `M : List A -> Type` (Z6a, flag 15): base `M nil`, step `(h : A) -> (t : List A) -> M t -> M (cons h t)`; check, eval and the lowering | `src/front/check.c:1124`, `src/front/check.c:1140`, `src/front/eval.c:553`, `src/front/eval.c:561`, `src/front/lower.c:932` | `examples/listind.lang:5`, `examples/listind.lang:11` |

F6: a fold over a family with one recursive field (for example `Stack`)
lowers to a loop over the spine. A constructor with two or more recursive
fields (for example `Tree`) needs a tree at run time. The lowering refuses
it with `REFUSE_RUNTIME_TREE` when the tree exists only at run time.
A fold over a closed tree works, because normalization folds it at
compile time.

F9: a lambda stays only as the direct function argument of an eliminator.
When a function value must exist at run time, the lowering refuses it
with `REFUSE_RUNTIME_CLOSURE`. Both refusals are HOST-LIMIT reasons, not
defects. `docs/CAPABILITY.md` gives the reasons.

F15: a domain family can take index parameters, for example
`family Lot (size : Nat)`. A constructor takes the indices first
(`makeLot 3 250 : Lot 3`). The checker keeps `Lot 3` and `Lot 4` apart.
A program cannot declare a family (rule R1). No test has a field type
that refers to an index. The index erases on Wasm.

C1, C2: `Path A a b` is `PathP` over a constant line, so the core has no
`Path` operation. `refl`, `symm` and `cong` take a path as well as an `Eq`
proof, and `funExt` gives a path. `transport` takes a path too (C3).
`trans` takes a path too (C3). Interval and path
values have no runtime content: a definition of a `PathP` type is a proof
and erases. When such a value must exist at run time, the lowering
refuses the program with `REFUSE_RUNTIME_PATH` (`test/ir/path.lang`).
This is a HOST-LIMIT reason, not a defect. A definition of a `Partial`
type erases too. `fromPartial` of a total system reduces to its branch
before the lowering (`examples/systems.lang`). Elimination of a neutral
partial element stays unreduced and is subject to the same runtime
refusal.

C3: `transp A r u` checks the line, its constancy where `r` holds
(`TYPE_LINE`) and `u : A i0`. The evaluator moves `u` along Pi, Sigma,
Prod and the kit data types. `transport P p u` over a path is
`transp (fun i => P (p @ i)) i0 u`. A `transp` that stays unreduced at
run time lowers to `u` (`src/front/lower.c:1312`). The core has no Glue
and no univalence, so the data does not change. Revisit this lowering
when Glue or `ua` lands. No entry can hold such a `transp` now: a
constant data line reduces, and a definition with a `Path` parameter is
not an entry.

C3: `hcomp side u0` takes a side of the form `fun j => [ ... ]`, a system
in `j` (`TYPE_SYSTEM` for another side). The faces are in the outer
context, and their join is `phi`. On each face, `side i0` must agree with
`u0` (`TYPE_SYSTEM`). The evaluator gives `side i1` where `phi` holds, and
moves the composition inside Pi, Prod and PathP types. An `hcomp` that
stays unreduced at run time is refused with `REFUSE_RUNTIME_PATH`
(`src/front/lower.c:1307`), for example `fun x => hcomp (fun j => []) x`
at `Nat`.

C3: `comp A side u0` and `fill A side u0 r` check the line, the system in
`j` at `A j` and the base condition (`TYPE_SYSTEM`). Over a constant line
the checker gives the `hcomp` forms (`src/front/check.c:1981`), so the
laws hold at a type variable. Over a varying line it gives the CCHM forms.
`hcomp` at a Sigma type (`src/front/eval.c:1955`) and `transp`
along a line of `PathP` types (`src/front/eval.c:1733`) use the CCHM
`comp` (`src/front/eval.c:1498`). `trans p q` over a path is an
`hcomp` (`src/front/check.c:1268`). A path over a varying line
fails with `TYPE_MISMATCH`, and `trans p refl` fails with `TYPE_INFER`:
use `<i> b` with a declared type. `examples/kan.lang` checks `rUnit`,
`lUnit` and `assoc`.

The refusal mutants are in `test/check/`: R1 `data.lang`, R2 `axiom.lang`,
R3 `rec.lang`, R4 `core-name.lang`. `test/check/expect.txt` gives the code
for each mutant.

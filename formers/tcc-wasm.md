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
| F1 | Product | DONE | `src/front/check.c:107`, `src/front/lower.c:577`, `src/front/lower.c:1189` | `examples/formers.lang:3`, `examples/laws.lang:12` |
| F2 | Coproduct | DONE | `src/front/check.c:108`, `src/front/check.c:120`, `src/front/lower.c:1197` | `examples/formers.lang:8`, `examples/laws.lang:13` |
| F3 | Option | DONE | `src/front/check.c:112`, `src/front/check.c:121`, `src/front/lower.c:1199` | `examples/formers.lang:11`, `examples/formers.lang:12` |
| F4 | List | DONE | `src/front/check.c:113`, `src/front/lower.c:824` | `examples/formers.lang:13`, `examples/inventory.lang:4`, `test/ir/spine.lang:8` |
| F5 | Monad pure/map/bind | DONE over Option, Sum E and List | `src/front/check.c:122`, `src/front/lower.c:1053`, `src/front/lower.c:1068`, `src/front/lower.c:839`, `src/front/lower.c:854` | `examples/monad.lang:2`, `examples/monad.lang:6`, `examples/monad.lang:8`, `examples/laws.lang:8`, `test/ir/spine.lang:10` |
| F6 | Algebra fold | DONE over Nat, List and domain families. HOST-LIMIT: a run-time tree | `src/front/check.c:126`, `src/front/lower.c:1103`, `src/front/lower.c:824`, `src/front/lower.c:1009`, `src/front/lower.c:1029` | `examples/algebra.lang:4`, `examples/algebra.lang:3`, `examples/algebra.lang:6`, `test/ir/spine.lang:18`, `test/ir/tree.lang:3` |
| F7 | Algebra unfold | DONE into List with a Nat step limit | `src/front/check.c:127`, `src/front/lower.c:926` | `examples/algebra.lang:7`, `examples/algebra.lang:8`, `test/ir/spine.lang:5` |
| F8 | Filter | DONE over Option and List | `src/front/check.c:125`, `src/front/lower.c:1079`, `src/front/lower.c:876` | `examples/monad.lang:9`, `test/ir/spine.lang:11` |
| F9 | Non-dependent Pi | DONE. HOST-LIMIT: a function value at run time | `src/front/check.c:477`, `src/front/lower.c:1249`, `src/front/lower.c:503` | `examples/functions.lang:2`, `examples/functions.lang:4`, `examples/laws.lang:5`, `test/ir/closure.lang:3` |
| F10 | Dependent Pi | DONE over `Type 0` and over Nat | `src/front/check.c:477`, `src/front/lower.c:1246` | `examples/functions.lang:8`, `examples/equality.lang:7` |
| F11 | Sigma | DONE | `src/front/check.c:114`, `src/front/check.c:118`, `src/front/lower.c:1190`, `src/front/lower.c:1193` | `examples/sigma.lang:3`, `examples/sigma.lang:5`, `examples/sigma.lang:6` |
| F12 | Eq refl/symm/trans | DONE | `src/front/check.c:115`, `src/front/check.c:128`, `src/front/check.c:129`, `src/front/lower.c:1160` | `examples/equality.lang:4`, `examples/equality.lang:5`, `examples/equality.lang:6`, `test/check/bad-refl.lang:1` |
| F13 | Eq transport/cong | DONE | `src/front/check.c:130`, `src/front/check.c:131`, `src/front/lower.c:1219`, `src/front/lower.c:1221` | `examples/equality.lang:8`, `examples/equality.lang:9` |
| F14 | Universes | DONE for Type 0 and Type 1, without cumulativity | `src/front/lower.c:1245` | `examples/universes.lang:4`, `test/check/universe.lang:1`, `test/check/universe-range.lang:1` |
| F15 | Generic indexed family | PARTIAL: only the domain file declares an indexed family | `domain/domain.lang:10` | `examples/inventory.lang:9`, `examples/laws.lang:16` |
| C1 | Interval | DONE | `src/front/check.c:140`, `src/front/check.c:143`, `src/front/check.c:144`, `src/front/check.c:145`, `src/front/eval.c:1008`, `src/front/lower.c:509`, `src/front/check.c:153`, `src/front/check.c:154`, `src/front/check.c:156`, `src/front/check.c:1433`, `src/front/eval.c:1072`, `src/front/eval.c:1161` | `examples/paths.lang:6`, `examples/paths.lang:7`, `examples/paths.lang:8`, `test/check/pretype.lang:2`, `examples/systems.lang:5`, `examples/systems.lang:6`, `examples/systems.lang:8`, `examples/systems.lang:10`, `test/check/system-overlap.lang:1`, `test/check/system-face.lang:1`, `test/check/from-partial.lang:2` |
| C2 | PathP and Path | DONE | `src/front/check.c:146`, `src/front/check.c:147`, `src/front/check.c:148`, `src/front/check.c:149`, `src/front/check.c:150`, `src/front/eval.c:1020`, `src/front/eval.c:2239` | `examples/paths.lang:4`, `examples/paths.lang:9`, `examples/paths.lang:12`, `examples/paths.lang:13`, `test/check/path-endpoint.lang:2`, `test/check/papp-nat.lang:2`, `test/ir/path.lang:4` |
| C3 | Kan operations | DONE: `transp`, `hcomp`, `comp` and `fill`; `transport` and `trans` over a path; the C3 notes give the limits | `src/front/check.c:158`, `src/front/check.c:1108`, `src/front/check.c:1513`, `src/front/check.c:1537`, `src/front/eval.c:1661`, `src/front/lower.c:1184`, `src/front/check.c:159`, `src/front/check.c:1638`, `src/front/eval.c:1926`, `src/front/lower.c:1180`, `src/front/check.c:160`, `src/front/check.c:161`, `src/front/check.c:1691`, `src/front/check.c:1006`, `src/front/eval.c:1365`, `src/front/eval.c:1814`, `src/front/eval.c:1596` | `examples/transp.lang:5`, `examples/transp.lang:9`, `examples/transp.lang:11`, `examples/transp.lang:15`, `examples/transp.lang:20`, `test/check/transp-const.lang:2`, `test/check/transp-line.lang:2`, `test/check/transp-start.lang:2`, `test/check/transp-name.lang:2`, `examples/hcomp.lang:7`, `examples/hcomp.lang:8`, `examples/hcomp.lang:9`, `examples/hcomp.lang:10`, `examples/hcomp.lang:11`, `examples/hcomp.lang:13`, `test/check/hcomp-face.lang:2`, `test/check/hcomp-base.lang:2`, `test/check/hcomp-side.lang:2`, `test/check/hcomp-name.lang:2`, `examples/kan.lang:8`, `examples/kan.lang:9`, `examples/kan.lang:10`, `examples/kan.lang:11`, `examples/kan.lang:12`, `examples/kan.lang:14`, `examples/kan.lang:17`, `examples/kan.lang:18`, `examples/kan.lang:21`, `test/check/comp-base.lang:2`, `test/check/comp-name.lang:2`, `test/check/fill-name.lang:2`, `test/check/trans-ends.lang:2`, `test/check/trans-line.lang:2` |
| C4 | Higher inductive types | DONE: declarations, path constructor values, the formal `hcomp`, the fold eliminator | `src/front/check.c:2392`, `src/front/check.c:2355`, `src/front/check.c:2274`, `src/front/check.c:2266`, `src/front/check.c:1857`, `src/front/core.h:170`, `src/front/eval.c:1959`, `src/front/eval.c:1830`, `src/front/eval.c:1855`, `src/front/eval.c:2712`, `src/front/lower.c:638`, `src/front/lower.c:1182`, `src/front/check.c:845`, `src/front/check.c:890`, `src/front/eval.c:2095`, `src/front/eval.c:2780`, `src/front/lower.c:949` | `domain/domain.lang:23`, `domain/domain.lang:24`, `domain/domain.lang:26`, `examples/hit.lang:9`, `examples/hit.lang:10`, `examples/hit.lang:13`, `examples/hit.lang:14`, `examples/hit.lang:15`, `examples/hit.lang:16`, `examples/hit.lang:17`, `examples/hit.lang:20`, `test/check/hit-program.lang:2`, `test/ir/hit-path.lang:5`, `examples/hit.lang:23`, `examples/hit.lang:24`, `examples/hit.lang:25`, `examples/hit.lang:27`, `examples/hit.lang:36`, `test/check/hit-fold-path.lang:2`, `test/check/hit-fold-prop.lang:3`, `test/check/hit-fold-arity.lang:2` |

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
run time lowers to `u` (`src/front/lower.c:1184`). The core has no Glue
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
(`src/front/lower.c:1180`), for example `fun x => hcomp (fun j => []) x`
at `Nat`.

C3: `comp A side u0` and `fill A side u0 r` check the line, the system in
`j` at `A j` and the base condition (`TYPE_SYSTEM`). Over a constant line
the checker gives the `hcomp` forms (`src/front/check.c:1691`), so the
laws hold at a type variable. Over a varying line it gives the CCHM forms.
`hcomp` at a Sigma type (`src/front/eval.c:1814`) and `transp`
along a line of `PathP` types (`src/front/eval.c:1596`) use the CCHM
`comp` (`src/front/eval.c:1365`). `trans p q` over a path is an
`hcomp` (`src/front/check.c:1006`). A path over a varying line
fails with `TYPE_MISMATCH`, and `trans p refl` fails with `TYPE_INFER`:
use `<i> b` with a declared type. `examples/kan.lang` checks `rUnit`,
`lUnit` and `assoc`.

The refusal mutants are in `test/check/`: R1 `data.lang`, R2 `axiom.lang`,
R3 `rec.lang`, R4 `core-name.lang`. `test/check/expect.txt` gives the code
for each mutant.

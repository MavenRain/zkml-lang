# Host capability probe: tcc-wasm

Date: 2026-10-24. Host tree: this repository at commit 6b4333f, read
only. TinyCC: 0.9.28rc 2026-09-04 mob@0fb54300 (AArch64 Darwin), as in
`PIN`. Driver: `build/langc` (built 2026-10-24). Other work loaded the
machine during the probe. Thus each wall time is an upper bound.

To run the probe again, write a chain file of N definitions over `Nat`
for N = 100 and for N = 1000:

```text
def chain0 : Nat := 1
def chain1 : Nat := natAdd chain0 1
```

The chain continues to `chain(N-1)`. Then run these commands for each
file from the repository root, outside a macOS sandbox (in the sandbox,
`/usr/bin/time -l` exits 1):

```sh
python3 -I probe/guard.py --rss-mb 4000 --timeout 600 -- /usr/bin/time -l build/langc check chain-N.lang
python3 -I probe/guard.py --rss-mb 4000 --timeout 600 -- /usr/bin/time -l build/langc build chain-N.lang -o chain-N.wasm
build/langc abi chain-N.lang > chain-N.abi
python3 -I probe/guard.py --rss-mb 4000 --timeout 600 -- /usr/bin/time -l node test/run-wasm.mjs chain-N.wasm - chain-N.abi < chain-N.abi
```

Each step runs below `probe/guard.py`, which stops a command above
4000 MB resident memory or above 600 s. The probe writes only its own
files: the chain files, the modules, the ABI files and the log. The
largest process in the probe used 54.9 MiB (`langc build` at N = 1000).

## Questions

Each host must answer these questions. Each answer is one numbered section
below. An answer sets the status of a former in `formers/tcc-wasm.md`.

| Question | Section | Formers |
|---|---|---|
| Which commands check a file, print a normal form, build and run? | 1 | all |
| Does the host take several files, or must the driver join them? | 2 | all |
| Which recursion does the host accept, and how does it refuse the rest? | 3 | F6, F7 |
| Which `Nat` primitives exist? Does `Nat` have an eliminator? | 4 | F6, F7 |
| How does a result leave the host? | 5 | target |
| What do check and build cost (time and resident memory) at N = 100 and N = 1000? | 6 | all |
| Can a family take a parameter? Can a constructor of such a family appear in a term? | 7 | F4, F12, F15 |
| Can a program project a Sigma? Does the kernel accept Sigma eta? | 8 | F11 |
| How does a program use equality: transport, cong, an index type? | 9 | F12, F13 |
| How does the host report an axiom? | 10 | R2 |

## 1. Commands

| Job | Command | Result |
| --- | --- | --- |
| Check | `build/langc check FILE` | `ok` and exit 0. A refusal prints `langc: CODE: DEF: message` on stderr and exits 1. A usage or IO error exits 2. |
| Print normal form | `build/langc eval FILE NAME [ARGS...]` | The normal form of `NAME`. Each argument is a decimal number. A trap prints `trap`, and the exit status is 0. |
| Build | `build/langc build FILE -o OUT` | A Wasm 1.0 module with one export for each entry. `build/langc abi FILE` prints the entries. |
| Run | `node test/run-wasm.mjs OUT NAME [ARGS...]` | A `Nat` or `Flag` result as `langc eval` prints it, or `trap`. For a `List Nat` entry, this command prints its address. Use `OUT - ABI` to decode the words (section 5); the script reads one `NAME [ARGS...]` line from stdin for each call. |

- Each command takes one program file. A source file is at most 1 MiB
  (`src/front/front.h:7`). `build/langc ir FILE` prints the first-order
  IR. Sources: `docs/host/README.md:114`, `docs/host/README.md:130` and
  `test/run-wasm.mjs:3`.
- Not probed in this run: `make zk-check`, `make halo2-check` and
  `make plonky3-check` (section 5). Their last numbers are in
  `docs/host/CAPABILITY.md`, section "Measured numbers".

## 2. Several files

No. Each command takes one program file, and the grammar has no import
form: a declaration is `def`, `axiom` or `family`
(`src/front/parser.c:2`). The domain prelude `domain/domain.lang` is part
of the executable: `tcc -run gen/embed.c` copies it into `build/domain.c`
at build time (`Makefile:18`). Each program can use its definitions.

To join several files, the driver must write one file with the
definitions in dependency order. A definition can use only earlier ones
(rule R3, `test/check/later.lang`). A program cannot declare a family
(section 7).

## 3. Recursion and totality

The host has no general recursion. A loop is a `fold` (over `Nat` and
`List` with a motive, and over the domain families) or an `unfold` into
`List` with a `Nat` step limit (formers F6, F7 and C7 in
`formers/tcc-wasm.md`). Each `fold` is structural and each `unfold` has a
step limit, so each program is total.

The checker refuses each other recursive definition with `REFUSE_REC`
and exit 1 (`test/check/expect.txt:3`). These are the exact lines for
`test/check/rec.lang`, `test/check/self.lang` and `test/check/later.lang`:

```text
langc: REFUSE_REC: loop: def rec is refused (rule R3); use fold or unfold
langc: REFUSE_REC: loop: loop is this definition or a later one; a definition can use only earlier ones (rule R3)
langc: REFUSE_REC: early: late is this definition or a later one; a definition can use only earlier ones (rule R3)
```

## 4. Nat

The `Nat` primitives are `natAdd`, `natSub`, `natMul`, `natDiv`,
`natMod`, `natEq`, `natLe` and `natMax`, and the proof `natIsSet`
(section 10). `natEq` and `natLe` give a `Flag`
(`src/front/check.c:135` to `src/front/check.c:143`).

- `Nat` is an unsigned 64-bit integer. The largest value is
  18446744073709551615. The lexer refuses a larger literal
  (`test/parse/nat-range.lang`).
- `natSub` stops at 0. `natAdd` and `natMul` trap on an overflow.
  `natDiv` and `natMod` trap on a divisor of 0. A trap is lazy
  (`docs/host/README.md`, section "Lazy traps").
- `Nat` has an eliminator: `fold` with a motive over `Nat` (slice Z5a).
  A definition can recurse on a `Nat` (`examples/nat.lang:20`).
- A `Nat` result leaves the host as an i64. `test/run-wasm.mjs` prints it
  as an unsigned integer, so each value up to 18446744073709551615 can
  leave the host.

## 5. How a result leaves the host

The Wasm runtime (slice Z7). The Wasm writer (`src/wasm.c`) writes a
Wasm 1.0 module. `langc abi` gives one line for each entry: `NAME` for
a `Nat` or a `Flag` result, and `NAME words` for a `List Nat` result.

| Item | Fact | Source |
|---|---|---|
| Entry export | Each entry is an export with its name. Each parameter and the result is an i64. A `Flag` argument above 1 traps. | `src/wasm.c` |
| `Nat` and `Flag` results | `test/run-wasm.mjs` prints the result as an unsigned 64-bit integer, as `langc eval` does, or `trap`. | `test/run-wasm.mjs` |
| Words entries | The result is the address of a words block [n, w0 .. w(n-1)] in the export `$memory`. A word is 8 bytes, low byte first. In batch mode with an ABI file, `test/run-wasm.mjs` prints the n words, or `nil` when n is 0. Its single-entry mode prints the address. | flag 56 (a), `test/run-wasm.mjs` |
| Memory export | Memory 0 has the name `$memory`. A guest name cannot have a `$`, so no entry export has this name. | flag 61 (a) |
| Heap | Function 0 is the allocator. It bumps the i64 global 0 over memory 0, and it grows the memory when a block does not fit. It traps at `WASM_MAX_PAGES`. Each entry sets the heap pointer to 0 first. | `src/wasm.c` |
| Locals | The lowering refuses an entry that needs more than 50000 locals (`REFUSE_LOCALS`, the V8 limit for one function). | `docs/STATUS.md` |
| Witness words | `bbWitnessWords` and `frWitnessWords` give the witness of a circuit as a words block. | `SPEC.md` section 7 |

To decode a words entry, build its module and pass its ABI file to the
batch runner. For example, from the repository root:

```sh
build/langc build examples/witness.lang -o /tmp/witness.wasm
build/langc abi examples/witness.lang > /tmp/witness.abi
printf 'bbWitResidual 4 6\n' | node test/run-wasm.mjs /tmp/witness.wasm - /tmp/witness.abi
```

The 3 circuit forms (slices Z8 to Z10). For each form, `langc eval`
prints the words of an entry, and a printer in `build/` writes the
target text from the words. The `make check` gate prints each form for
its cases: `llzk: 6 checked`, `halo2: 5 checked` and
`plonky3: 5 checked` (slice Z10d). Each tool check runs outside the
sandbox.

| Form | Printer | Output | Tool check |
|---|---|---|---|
| LLZK | `build/llzkprint` (`src/llzk.c`) | LLZK text | `make zk-check` (`Makefile:81`): `llzk-opt` (LLZK 2.1.2) on each module |
| Halo2 | `build/halo2print` (`src/halo2.c`) | A Rust crate: `Cargo.toml` and `src/lib.rs` | `make halo2-check` (`Makefile:116`): the halo2_proofs MockProver on 5 crates |
| Plonky3 | `build/plonky3print` (`src/plonky3.c`) | A Rust crate: `Cargo.toml` and `src/lib.rs` | `make plonky3-check` (`Makefile:146`): the uni-stark prover on 5 crates |

## 6. Cost

| Step | N = 100 | N = 1000 |
| --- | --- | --- |
| Check | 0.04 s, 46.5 MiB | 0.08 s, 50.3 MiB |
| Build | 0.06 s, 50.0 MiB | 0.07 s, 54.9 MiB |
| Run | 0.05 s, 41.1 MiB | 0.05 s, 44.4 MiB |

Each time is the wall time from `/usr/bin/time -l`. Each size is its
maximum resident set size. Each step exits 0. The modules have 2935 bytes
(N = 100) and 31686 bytes (N = 1000). The run step calls each of the N
entries one time, and the last result is N.

## 7. Parametric families

A program cannot declare a family. The checker refuses it
(`test/check/data.lang`):

```text
langc: REFUSE_DATA: Coin: a program cannot declare a family (rule R1); only domain/domain.lang can
```

In `domain/domain.lang`, a family can take a type parameter
(`Trunc (A : Type 0)`, `domain/domain.lang:24`; `Quot`,
`domain/domain.lang:26`) or an index (`Lot (size : Nat)`,
`domain/domain.lang:10`; `Op (a b : Shape)`, `domain/domain.lang:41`).
A constructor of such a family can appear in a term, with its parameters
first: `truncIn Nat 2` (`examples/hit.lang:30`) and `makeLot 3 250`
(`examples/inventory.lang:9`). The core families `List`, `Option`,
`Prod`, `Sum`, `Sigma` and `Eq` also take type parameters. Because a
program cannot declare a family, F15 is PARTIAL
(`formers/tcc-wasm.md:28`).

## 8. Sigma projection and eta

`witness s` and `payload s` check (`examples/sigma.lang:4`,
`examples/sigma.lang:5`). `(n : Nat) * B` is a second form of
`Sigma (n : Nat) B` (`examples/sigma.lang:6`). There is no pattern form:
the grammar has no Sigma pattern (`src/front/parser.c:2`).

The kernel identifies `pack (witness s) (payload s)` with `s`, and
`pair (first p) (second p)` with `p`. In this probe, `langc check`
prints `ok` for each of these definitions:

```text
def sEta : (s : Sigma (n : Nat) (Eq Nat n 1)) -> Path (Sigma (n : Nat) (Eq Nat n 1)) s (pack (witness s) (payload s)) := fun s => refl
def pEta : (p : Prod Nat Nat) -> Path (Prod Nat Nat) p (pair (first p) (second p)) := fun p => refl
```

Record eta also applies to a domain family with one constructor
(`examples/nat.lang:32`). It does not apply to a higher inductive type
(`test/check/eta-hit.lang`).

## 9. Equality

A program writes equality in 2 forms:

- `Eq A a b`, with `refl`, `symm e`, `trans e1 e2`, `transport P e u`
  and `cong f e` (`examples/equality.lang:4` to
  `examples/equality.lang:9`).
- The cubical forms `Path A a b`, `PathP`, `<i> t`, `p @ r` and `funExt`
  (former C2, `examples/paths.lang:4`).

One family serves each index type, because the first argument of `Eq` is
the type. In this probe, `langc check` prints `ok` for these definitions:

```text
def eqList : Eq (List Nat) (cons 1 nil) (cons 1 nil) := refl
def eqShape : Eq Shape (shape (cons 2 nil)) (shape (cons 2 nil)) := refl
def eqFlag : Eq Flag flagYes flagYes := refl
def moveList : (P : List Nat -> Type 0) -> P (cons 1 nil) -> P (cons 1 nil) := fun P u => transport P eqList u
```

A proof erases at the lowering, and its trap flags stay
(`docs/host/README.md`, section "Compilation").

## 10. Axiom detection

No command lists the axioms of a file, because a checked file has no
axiom. The checker refuses each `axiom` declaration
(`src/front/check.c:3112`). For a file with no axiom,
`build/langc check FILE` prints `ok` and exits 0. For a file with one
axiom (`test/check/axiom.lang`), it prints this line and exits 1:

```text
langc: REFUSE_AXIOM: magic: an axiom has no proof (rule R2)
```

`natIsSet` is the one trusted primitive (slice Z5a, flag 5 (a)): the
proof that `Nat` is a set. It is always stuck. The lowering refuses it at
run time (`src/front/lower.c:1310`).

## Consequences for M0

1. A program is one file over the domain prelude. A new family goes in
   `domain/domain.lang`, and the driver joins the files in dependency
   order (sections 2 and 7).
2. Each loop is a `fold` or an `unfold` with a step limit. No other
   recursion checks (section 3).
3. Each number is a 64-bit `Nat` word, and an overflow traps. The domain
   builds each field element from `Nat` words, for example `Fr`
   (`domain/domain.lang:584`) (sections 4 and 5).
4. A proof erases, and a value at run time is first-order. A program
   that needs a function, a tree or a path at run time gets a HOST-LIMIT
   code (sections 5 and 9, and `docs/host/CAPABILITY.md`).
5. The Wasm module is the witness runtime. Each circuit form comes from
   the `langc eval` words and a printer, and a tool check outside the
   sandbox accepts it (section 5).
6. The size of a program is not a constraint at this scale: at
   N = 1000, each step takes less than 0.1 s and less than 55 MiB
   (section 6).
7. A proof can use `refl` for a Sigma or a Prod repack (section 8), and
   one `Eq` for each index type (section 9).
8. A checked file has no axiom. `natIsSet` is the one trusted primitive
   (section 10).

# Host capability probe: tcc-wasm

Date: YYYY-MM-DD. Host tree: `<path>` at <commit>, read only. Driver:
`<host executable>` (built YYYY-MM-DD). <Say if other work loaded the
machine. Then each wall time is an upper bound.>

To run the probe again: <the command that runs all steps, and the command
that runs some steps>. Each step runs below `probe/guard.py`, which stops a
command above <N> MB resident memory or above its time limit. The probe
writes only below a new temporary directory. The largest process in the
probe used <N> MB.

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
| Check | `<command> FILE` | <exit codes and output> |
| Print normal form | `<command>` | <output> |
| Build | `<command>` | <output> |
| Run | `<command>` | <output> |

- <Limits of each command, for example one file only.>
- Not probed: <commands that the probe did not run>.

## 2. Several files

<Does the host accept several files or an import form? If not, which order
must the driver use to join them?>

## 3. Recursion and totality

<Which recursive definitions check. Which calls the host refuses, with the
exact error line.>

## 4. Nat

<The primitives. The result of a subtraction below zero. Whether a
definition can recurse on a `Nat`. The largest value that can leave the
host.>

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

## 6. Cost

| Step | N = 100 | N = 1000 |
| --- | --- | --- |
| Check | <s>, <MB> | <s>, <MB> |
| Build | <s>, <MB> | <s>, <MB> |
| Run | <s>, <MB> | <s>, <MB> |

## 7. Parametric families

<Whether a family can take a type parameter, and whether its constructors can
appear in a term. Name the probe file.>

## 8. Sigma projection and eta

<Whether `witness` and `payload` (or `.1` and `.2`) check, whether a Sigma
pattern parses, and whether the kernel identifies `pack (witness s)
(payload s)` with `s`.>

## 9. Equality

<How a program writes `Eq`, `transport` and `cong`. Whether one family can
serve each index type.>

## 10. Axiom detection

<The command that lists the axioms of a file. Its output for a file with no
axiom and for a file with one axiom.>

## Consequences for M0

1. <One rule for the build that a section above forces. Cite the section.>

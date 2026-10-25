# zkml-lang

zkml-lang is a language for zero-knowledge machine learning circuits. Its
core types and operations come from the Cubical Agda fragment in
`design/DESIGN.md`: shapes, tensors over a field, a circuit IR, a resource
monoid, and layers with soundness and completeness proofs that form a
category. The type formers are those of cubical type theory, with paths,
HITs and QIITs. The host is TinyCC. The targets are LLZK IR, Halo2, Plonky3
and a Wasm witness runtime. It comes from lang-template with the tcc-wasm
host.

Status: slices Z0 to Z10 and ZL are DONE. Slice Z11 (the final docs and
the final gate) closes the plan (`docs/PLAN.md` row Z11).

- `docs/PLAN.md`: the rulings, the design choices, the notes on the
  fragment and the slices Z0 to Z11 and ZL.
- `SPEC.md`: the specification: the core types and operations, the
  target encodings, the host facts and the open items.
- `design/DESIGN.md`: the domain design. It is the only source of the core
  types and the core operations.
- `formers/FORMERS.md`: the type formers. `formers/tcc-wasm.md` gives their
  status on the tcc-wasm host.
- `probe/CAPABILITY.md`: the host probe.
- `docs/STATUS.md` and `docs/VALIDATION.md`: the status and the gate results.
- `docs/host/README.md`: how to build and test the host kit, and how to
  replace the sample domain in `domain/`.

## Build and check

The build needs Make and TinyCC (`tcc`). `make check` also needs a C99
compiler (`cc`), Node.js and WABT (`wasm-validate` and `wasm-objdump`).
From this repository's root:

```sh
make build
make check
```

## Targets

| Target | Make target | Tools |
|---|---|---|
| Wasm witness runtime | `make check` | The host build and check tools listed above. `make check` also checks the LLZK, Halo2 and Plonky3 words and printers, with no target tool. |
| LLZK IR | `make zk-check` | `llzk-opt` from llzk-lib v2.1.2 (LLVM 20.1.8). Set `LLZK_OPT` to its path. |
| Halo2 | `make halo2-check` | Rust, offline: halo2_proofs v0.3.0 in the cargo cache and `test/halo2/Cargo.lock`. Run it outside the sandbox. |
| Plonky3 | `make plonky3-check` | Rust, offline: the Plonky3 crates of rev 5fcf8d7c in the cargo cache and `test/plonky3/Cargo.lock`. Run it outside the sandbox. |

If `testledger` is unavailable, pass `CARGO_TEST='cargo test'` to
`make halo2-check` and `make plonky3-check`.

## Examples

`examples/` has one file for each group of forms. `make check` checks
each file. The target files are `examples/witness.lang` (the witness
words), `examples/llzk.lang`, `examples/halo2.lang` and
`examples/plonky3.lang`. The field instances are in
`examples/babybear.lang` and `examples/bn254.lang`.

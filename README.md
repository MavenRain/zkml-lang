# zkml-lang

zkml-lang is a language for zero-knowledge machine learning circuits. Its
core types and operations come from the Cubical Agda fragment in
`design/DESIGN.md`: shapes, tensors over a field, a circuit IR, a resource
monoid, and layers with soundness and completeness proofs that form a
category. The type formers are those of cubical type theory, with paths,
HITs and QIITs. The host is TinyCC. The targets are LLZK IR, Halo2, Plonky3
and a Wasm witness runtime. It comes from lang-template with the tcc-wasm
host.

- `docs/PLAN.md`: the rulings, the design choices, the notes on the
  fragment and the slices Z0 to Z11.
- `SPEC.md`: the specification. Fill each section.
- `design/DESIGN.md`: the domain design. It is the only source of the core
  types and the core operations.
- `formers/FORMERS.md`: the type formers. `formers/tcc-wasm.md` gives their
  status on the tcc-wasm host.
- `probe/CAPABILITY.md`: the host probe.
- `docs/STATUS.md` and `docs/VALIDATION.md`: the status and the gate results.
- `docs/host/README.md`: how to build and test the host kit, and how to
  replace the sample domain in `domain/`.

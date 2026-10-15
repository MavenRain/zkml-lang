# zkml-lang domain design

> Paste the domain design below the line, verbatim. Do not edit it.

This file is the only source of the core types and the core operations of
zkml-lang. `SPEC.md` sections 5 and 6 cite its sections. Do not add a core
type or a core operation that this file does not give.

When the design changes, paste the new version. Then record the change as an
open item in `SPEC.md` section 9.

---

Type constructors and higher types include products, coproducts, dependent types, monads (map/bind), algebras and coalgebras (fold/unfold), quotients, paths, loops, HITs, and QIITs (all as in Cubical TT).  Base types, operations are restricted to what is in this Cubical Agda fragment:

```agda
{-# OPTIONS --cubical #-}

open import Cubical.Core.Everything
open import Cubical.Foundations.Prelude
open import Cubical.Categories.Category
open import Cubical.Categories.Functor
open import Cubical.Algebra.Monoid

module ZkMLDSL (F : Type)  -- field elements
                (Witness : Type)
                (satisfies : ∀ {inS outS} → Circuit F inS outS → Tensor F inS → Witness → Type)
                (eval : ∀ {inS outS} → Circuit F inS outS → Witness → Tensor F outS) where

  -- Shapes and tensors
  record Shape : Type where
    constructor shape
    field
      dims : List ℕ

  Tensor : Shape → Type
  Tensor s = -- nested Fin-indexed structure over F

  -- Circuit IR
  data Circuit (inS outS : Shape) : Type where
    id : Circuit inS inS
    -- arithmetic, lookups, constraints, …

  -- Resource monoid
  record Resource : Type where
    constructor res
    field
      gates lookups degree : ℕ

  _∙_ : Resource → Resource → Resource
  res g₁ l₁ d₁ ∙ res g₂ l₂ d₂ = res (g₁ + g₂) (l₁ + l₂) (max d₁ d₂)

  zeroR : Resource
  zeroR = res 0 0 0

  -- Resource monoid laws (as Paths; most definitional via ℕ)
  ∙-assoc : ∀ r s t → (r ∙ s) ∙ t ≡ r ∙ (s ∙ t)
  ∙-assoc _ _ _ = refl

  ∙-unitˡ : ∀ r → zeroR ∙ r ≡ r
  ∙-unitˡ _ = refl

  ∙-unitʳ : ∀ r → r ∙ zeroR ≡ r
  ∙-unitʳ _ = refl

  ResourceMonoid : Monoid _ _
  ResourceMonoid = record
    { Carrier = Resource
    ; ε = zeroR
    ; _·_ = _∙_
    ; assoc = ∙-assoc
    ; identityˡ = ∙-unitˡ
    ; identityʳ = ∙-unitʳ
    }

  -- Core Layer
  record Layer (inS outS : Shape) : Type where
    field
      circuit      : Circuit inS outS
      denotation   : Tensor inS → Tensor outS
      soundness    : ∀ x w → satisfies circuit x w → denotation x ≡ eval circuit w
      completeness : ∀ x → ∃[ w ] (satisfies circuit x w × denotation x ≡ eval circuit w)
      resources    : Resource

  -- Sequential composition
  _∘_ : ∀ {a b c} → Layer b c → Layer a b → Layer a c
  L₂ ∘ L₁ = record
    { circuit      = composeCircuits (L₂ .circuit) (L₁ .circuit)
    ; denotation   = L₂ .denotation ∘ L₁ .denotation
    ; soundness    = λ x w → -- composed soundness Path
                   -- (soundness L₂ (denotation L₁ x) …) ∙ cong (denotation L₂) (soundness L₁ x …)
    ; completeness = λ x → -- witness pairing
                   -- …
    ; resources    = L₂ .resources ∙ L₁ .resources
    }
    where
      composeCircuits : Circuit b c → Circuit a b → Circuit a c
      composeCircuits = -- wiring

  -- Identity
  idL : ∀ {s} → Layer s s
  idL = record
    { circuit = id ; denotation = idfun _ ; soundness = λ _ _ _ → refl
    ; completeness = λ x → x , (tt , refl) ; resources = zeroR }

  -- Category laws as Paths
  ∘-assoc : ∀ {a b c d} (L₃ : Layer c d) (L₂ : Layer b c) (L₁ : Layer a b)
          → (L₃ ∘ L₂) ∘ L₁ ≡ L₃ ∘ (L₂ ∘ L₁)
  ∘-assoc L₃ L₂ L₁ = -- Path built from circuit assoc, denotation assoc (function), resource ∙-assoc
                     -- and soundness/completeness coherence (PathP or higher Path if needed)
                     refl  -- or more elaborate Path when circuits are not definitionally associative

  idL-unitˡ : ∀ {a b} (L : Layer a b) → idL ∘ L ≡ L
  idL-unitˡ L = refl

  idL-unitʳ : ∀ {a b} (L : Layer a b) → L ∘ idL ≡ L
  idL-unitʳ L = refl

  -- Layer category (with explicit Path laws)
  LayerCat : Category _ _ _
  LayerCat .ob = Shape
  LayerCat .Hom[ s , t ] = Layer s t
  LayerCat .id = idL
  LayerCat ._⋆_ = _∘_
  LayerCat .⋆IdL = idL-unitˡ
  LayerCat .⋆IdR = idL-unitʳ
  LayerCat .⋆Assoc = ∘-assoc
  -- isSetHom : Path-truncated if desired

  -- Soundness of composition as a Path (coherence of proofs)
  soundness-∘ : ∀ {a b c} (L₂ : Layer b c) (L₁ : Layer a b) (x : Tensor a) (w : Witness)
              → satisfies (L₂ ∘ L₁ .circuit) x w
              → (L₂ ∘ L₁) .denotation x ≡ eval (L₂ ∘ L₁ .circuit) w
  soundness-∘ L₂ L₁ x w sat = -- Path relating the two soundness proofs
                             -- via transport or cong along the composition Path
                             (L₂ ∘ L₁) .soundness x w sat

  -- Specific NN operators
  matmul : (m n k : ℕ) → Layer (shape (m ∷ k ∷ [])) (shape (m ∷ n ∷ []))
  matmul m n k = -- …

  softmax : (d : ℕ) → Layer (shape (d ∷ [])) (shape (d ∷ []))
  softmax d = -- …

  attention : (h d : ℕ) → Layer (shape …) (shape …)
  attention h d = -- composition of matmuls + softmax; uses ∘-assoc Path

  layerNorm : (d : ℕ) → Layer (shape (d ∷ [])) (shape (d ∷ []))
  layerNorm d = -- …

  quantize : (bits : ℕ) → Layer s s
  quantize bits = -- …

  lookup : (table : Fin n → F) → Layer (shape []) (shape [])
  lookup table = -- …

  -- Residual / skip connection (as a loop-like Path or fixed-point style)
  residual : ∀ {s} → Layer s s → Layer s s
  residual L = record
    { circuit = -- L.circuit + identity wiring
    ; denotation = λ x → L .denotation x + x   -- or residual path
    ; soundness = -- Path from L.soundness and id
    ; completeness = -- …
    ; resources = L .resources ∙ zeroR
    }

  -- Higher path / loop for residual coherence (optional HIT-style or PathP)
  residual-loop : ∀ {s} (L : Layer s s) (x : Tensor s)
                → residual L .denotation x ≡ L .denotation x + x
  residual-loop L x = refl

  -- Denotational functor (preserves composition up to Path)
  denote : Functor LayerCat (FUNCTOR Shapes (Tensor F))
  denote .F-ob = Tensor
  denote .F-hom L = L .denotation
  denote .F-id = λ _ → refl
  denote .F-seq = λ L₂ L₁ → refl   -- denotation (L₂ ∘ L₁) ≡ denotation L₂ ∘ denotation L₁

  -- Export functors (machine-checked preservation)
  exportHalo2 : Functor LayerCat Halo2CircuitCat
  exportHalo2 .F-hom L = translate (L .circuit)
  exportHalo2 .F-id = refl
  exportHalo2 .F-seq = λ L₂ L₁ → -- Path that translation preserves composition
                                -- and resources / soundness are preserved (via transport along Path)

  exportPlonky3 : Functor LayerCat Plonky3CircuitCat
  exportPlonky3 .F-seq = λ _ _ → refl  -- or explicit Path

  exportLLZK : Functor LayerCat LLZKIRCat
  exportLLZK .F-seq = λ _ _ → refl

  -- Resource preservation under export (homomorphism Path)
  resources-export : ∀ {a b} (L : Layer a b)
                   → resources (exportHalo2 .F-hom L) ≡ L .resources
  resources-export L = refl

  -- Functoriality of resources (composition preserved)
  resources-∘ : ∀ {a b c} (L₂ : Layer b c) (L₁ : Layer a b)
              → (L₂ ∘ L₁) .resources ≡ L₂ .resources ∙ L₁ .resources
  resources-∘ _ _ = refl
```

Laws from the fragment should be considered a minimal base, not exhaustive list.

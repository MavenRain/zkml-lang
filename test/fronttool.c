/* Domain declaration regressions use the same parser and checker as langc. */
#include <stdio.h>
#include <string.h>

#include "front/check.h"
#include "front/lexer.h"
#include "front/parser.h"

static int domain(const char *text, const char *want) {
  Arena arena;
  Diag diag;
  TokenList tokens = {0};
  DeclList decls = {0};
  Machine machine;
  arena_init(&arena, (size_t)1 << 24);
  diag_init(&diag);
  int ok = lex_source(&arena, "test-domain", text, strlen(text), &tokens, &diag)
    && parse_source(&arena, "test-domain", ORIGIN_DOMAIN, &tokens, &decls, &diag)
    && check_program(&arena, &decls, &machine, &diag);
  int passed = want == NULL ? ok : !ok && diag.code != NULL && strcmp(diag.code, want) == 0;
  if (!passed) {
    fprintf(stderr, "FAIL domain: wanted %s, got %s: %s\n", want == NULL ? "ok" : want,
            ok ? "ok" : diag.code == NULL ? "no diagnostic" : diag.code, text);
  }
  arena_release(&arena);
  return passed;
}

/* Printed normal forms must check again and denote the original value. */
static int roundtrip_at(const char *type, const char *body, int nat_runtime) {
  Arena arena;
  Diag diag;
  TokenList tokens = {0};
  DeclList decls = {0};
  Machine machine;
  char source[16384];
  char normal[8192];
  int ok;
  arena_init(&arena, (size_t)1 << 24);
  diag_init(&diag);
  snprintf(source, sizeof source, "def original : %s := %s\n", type, body);
  ok = lex_source(&arena, "roundtrip", source, strlen(source), &tokens, &diag)
    && parse_source(&arena, "roundtrip", ORIGIN_PROGRAM, &tokens, &decls, &diag)
    && check_program(&arena, &decls, &machine, &diag)
    && value_print(&machine, NULL, 0, def_value(&machine, 0), normal, sizeof normal);
  if (ok) {
    snprintf(source, sizeof source, "def original : %s := %s\ndef copy : %s := %s\n", type, body, type, normal);
    tokens = (TokenList){0};
    decls = (DeclList){0};
    ok = lex_source(&arena, "roundtrip", source, strlen(source), &tokens, &diag)
      && parse_source(&arena, "roundtrip", ORIGIN_PROGRAM, &tokens, &decls, &diag)
      && check_program(&arena, &decls, &machine, &diag)
      && conv_values(&machine, 0, def_value(&machine, 0), def_value(&machine, 1));
  }
  /* Conversion can ignore runtime checks. Also apply both functions to
     concrete inputs when checking preservation of successor overflow. */
  if (nat_runtime) {
    uint32_t i;
    for (i = 0; ok && i < 2u; i++) {
      const Value *fn = def_value(&machine, i);
      const Value *neutral = apply_value(&machine, fn, val_var(&machine, 0u));
      const Value *small = apply_value(&machine, fn, val_nat(&machine, 3u));
      const Value *large = apply_value(&machine, fn, val_nat(&machine, UINT64_MAX));
      ok = small != NULL && small->kind == VAL_NAT && small->nat == 0u && large != NULL && large->kind == VAL_TRAP
        && conv_values(&machine, 1u, neutral, val_nat(&machine, 0u));
    }
  }
  if (!ok) {
    fprintf(stderr, "FAIL roundtrip: %s := %s\n", type, body);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

static int roundtrip(const char *type, const char *body) {
  return roundtrip_at(type, body, 0);
}

/* As roundtrip, with a domain text (family declarations and the defs they
   need) in front of both sources, so the normal form can use constructors
   that the kit domain is not linked in for. The last def is the one printed
   and compared. */
static int roundtrip_in(const char *domain, const char *type, const char *body) {
  Arena arena;
  Diag diag;
  TokenList tokens = {0};
  DeclList decls = {0};
  Machine machine;
  char source[16384];
  char normal[8192];
  int ok;
  arena_init(&arena, (size_t)1 << 24);
  diag_init(&diag);
  snprintf(source, sizeof source, "%s\ndef original : %s := %s\n", domain, type, body);
  ok = lex_source(&arena, "roundtrip", source, strlen(source), &tokens, &diag)
    && parse_source(&arena, "roundtrip", ORIGIN_DOMAIN, &tokens, &decls, &diag)
    && check_program(&arena, &decls, &machine, &diag)
    && value_print(&machine, NULL, 0, def_value(&machine, machine.def_count - 1u), normal, sizeof normal);
  if (ok) {
    snprintf(source, sizeof source, "%s\ndef original : %s := %s\ndef copy : %s := %s\n", domain, type, body, type, normal);
    tokens = (TokenList){0};
    decls = (DeclList){0};
    ok = lex_source(&arena, "roundtrip", source, strlen(source), &tokens, &diag)
      && parse_source(&arena, "roundtrip", ORIGIN_DOMAIN, &tokens, &decls, &diag)
      && check_program(&arena, &decls, &machine, &diag)
      && conv_values(&machine, 0, def_value(&machine, machine.def_count - 2u), def_value(&machine, machine.def_count - 1u));
  }
  if (!ok) {
    fprintf(stderr, "FAIL roundtrip_in: %s := %s\n", type, body);
    diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* Z4a: the kit domain's indexed family, for the C5 round trips. */
static const char circuit_domain[] =
  "family Shape := shape (dims : List Nat)\n"
  "family Op (a b : Shape) :=\n"
  "    opQuantize (qzS : Shape) (qzK : Nat) : Op qzS qzS\n"
  "  | opMatMul (mmN mmM mmK : Nat) : Op (shape (cons mmN nil)) (shape (cons mmM nil))\n"
  "  | opLookup (lkS : Shape) (lkT : Nat) : Op lkS lkS\n"
  "  | opSoftmax (smS : Shape) (smT smK : Nat) : Op smS smS\n"
  "  | opLayerNorm (lnS : Shape) (lnG lnT : Nat) : Op lnS lnS\n"
  "  | opAttention (atH atD atQ atK atV atT atE : Nat) : Op (shape (cons atH (cons atD nil))) (shape (cons atH (cons atD nil)))\n"
  "family Circuit (a b : Shape) :=\n"
  "    circId (idA : Shape) : Circuit idA idA\n"
  "  | circSeq (seqA seqB seqC : Shape) (seqF : Circuit seqA seqB) (seqG : Circuit seqB seqC) : Circuit seqA seqC\n"
  "  | circOp (opA opB : Shape) (opO : Op opA opB) : Circuit opA opB\n"
  "  | circResidual (rsS : Shape) (rsF : Circuit rsS rsS) : Circuit rsS rsS\n"
  "  | circIdl (idlA idlB : Shape) (idlF : Circuit idlA idlB)\n"
  "      : Path (Circuit idlA idlB) (circSeq idlA idlA idlB (circId idlA) idlF) idlF\n"
  "  | circIdr (idrA idrB : Shape) (idrF : Circuit idrA idrB)\n"
  "      : Path (Circuit idrA idrB) (circSeq idrA idrB idrB idrF (circId idrB)) idrF\n"
  "  | circAssoc (asA asB asC asD : Shape) (asF : Circuit asA asB) (asG : Circuit asB asC) (asH : Circuit asC asD)\n"
  "      : Path (Circuit asA asD) (circSeq asA asC asD (circSeq asA asB asC asF asG) asH) (circSeq asA asB asD asF (circSeq asB asC asD asG asH))\n"
  "  | circSquash (sqA sqB : Shape) (sqF sqG : Circuit sqA sqB) (sqP sqQ : Path (Circuit sqA sqB) sqF sqG)\n"
  "      : Path (Path (Circuit sqA sqB) sqF sqG) sqP sqQ";

int main(void) {
  int ok = 1;
  ok &= roundtrip_at("Nat -> Nat", "fun n => fold (fun k => Nat) (fun k acc => 0) 0 (natAdd n 1)", 1);
  ok &= domain("def answer : Nat := fold (fun k => Nat -> Nat) (fun k f x => natAdd (f x) k) (fun x => x) 3 4", NULL);
  ok &= domain("family Tone := low | high\n"
               "def answer : Nat := fold (fun k => Tone -> Nat) (fun k f x => natAdd (f x) k) (fun x => 0) 3 low", NULL);
  ok &= roundtrip("Nat -> Nat", "fold (fun k => Nat) (fun k acc => natAdd acc 1) 0");
  ok &= roundtrip("(Nat -> Nat -> Nat) -> Nat -> Nat -> Nat", "fold (fun k => Nat)");
  ok &= roundtrip("Nat -> Type 0", "fun n => fold (fun k => Type 0) (fun k acc => acc) Nat n");
  ok &= domain("def good : (A : Type 0) -> A -> Nat -> A := fun A a n => fold (fun k => A) (fun k acc => acc) a n", NULL);
  ok &= domain("def good : Nat := fold (fun f x => f x) (fun (x : Nat) => x) 3 4", NULL);
  ok &= domain("family F := base | loop (x : F) : Path F x x\n"
               "def foldF : F -> Nat := fun x => fold 0 (fun y => refl) x\n"
               "def law : (i : I) -> Path Nat (foldF (loop base @ i)) 0 := fun i => refl", NULL);
  ok &= domain("def bad : I := hcomp (fun j => [ i1 |-> j ]) i0", "TYPE_PRETYPE");
  ok &= domain("def bad : I := hcomp (fun j => []) i0", "TYPE_PRETYPE");
  ok &= domain("def bad : I -> Nat := hcomp (fun j => []) (fun i => 3)", "TYPE_PRETYPE");
  ok &= domain("def bad : Nat -> I := hcomp (fun j => []) (fun x => i0)", "TYPE_PRETYPE");
  ok &= domain("def bad : Partial i1 Nat := hcomp (fun j => []) [ i1 |-> 3 ]", "TYPE_PRETYPE");
  ok &= domain("def s : Partial i1 Nat := [ i1 |-> 3 ]\ndef bad : Nat := fromPartial (hcomp (fun j => []) s)", "TYPE_PRETYPE");
  ok &= domain("def good : (i : I) -> Nat := fun i => hcomp (fun i => [ i |-> 3 ]) 3", NULL);
  ok &= domain("def bad : (i : I) -> Nat := fun i => hcomp (fun i => [ i |-> 3 ]) 4", "TYPE_SYSTEM");
  ok &= domain("def good : (i : I) -> Path Nat 3 4 -> Nat := fun i p => hcomp (fun i => [ i |-> p @ i ]) 3", NULL);
  ok &= domain("def bad : (i : I) -> Path Nat 3 4 -> Nat := fun i p => hcomp (fun i => [ i |-> p @ i , i1 |-> 3 ]) 3", "TYPE_SYSTEM");
  ok &= roundtrip("(i : I) -> Nat", "fun i => hcomp (fun i => [ i |-> 3 ]) 3");
  ok &= domain("family F := makeF (field : Nat) (field : Flag)", "REFUSE_NAME");
  ok &= domain("family F := makeF (makeF : Nat)", "REFUSE_NAME");
  ok &= domain("family F := makeF (firstField : Nat) (secondField : Flag)", NULL);
  ok &= domain("family F := a (field : Nat) | b (field : Nat)", "REFUSE_NAME");
  /* Z3a: higher inductive types (C4). */
  ok &= domain("family Circle := circleBase | circleLoop : Path Circle circleBase circleBase\n"
               "family Trunc (A : Type 0) := truncIn (truncVal : A)\n"
               "  | truncSquash (truncX truncY : Trunc A) : Path (Trunc A) truncX truncY\n"
               "family Quot (A : Type 0) (R : A -> A -> Type 0) := quotIn (quotRep : A)\n"
               "  | quotGlue (quotA quotB : A) (quotR : R quotA quotB) : Path (Quot A R) (quotIn A R quotA) (quotIn A R quotB)\n"
               "  | quotSquash (quotX quotY : Quot A R) (quotP quotQ : Path (Quot A R) quotX quotY) : Path (Path (Quot A R) quotX quotY) quotP quotQ", NULL);
  ok &= domain("family L (A : Type 0) := lnil | lcons (h : A) (t : L A)", NULL);
  ok &= domain("family F := base | loop (x : F) (y : Eq Nat (fold 0 0 x) 0) : Path F x x", "DOMAIN_FAMILY");
  ok &= domain("family F := base | extra (y : Eq Nat (fold 0 0 0 base) 0) | loop : Path F base base", "DOMAIN_FAMILY");
  ok &= domain("family C := c | l : Nat", "DOMAIN_HIT");
  ok &= domain("family C := c | l : Path Nat 0 0", "DOMAIN_HIT");
  ok &= domain("family C := c | l : Path C (transp (fun i => C) i0 c) c", "DOMAIN_HIT");
  ok &= domain("family C := c | l (p : Path C c c) : Path (Path (Path C c c) p p) refl refl", "DOMAIN_HIT");
  ok &= domain("family C := c | d (p : Path C c c)", "DOMAIN_HIT");
  ok &= domain("family C := c | l : Path C c c | d", "DOMAIN_HIT");
  ok &= domain("family C := l (x : C) : Path C x x", "DOMAIN_HIT");
  ok &= domain("family C := c | l : Path C c 0", "TYPE_MISMATCH");
  ok &= domain("family L (A : Type 0) := lnil | lcons (h : A) (t : L Nat)", "DOMAIN_FAMILY");
  ok &= domain("family Trunc (A : Type 0) := truncIn (truncVal : A)\n"
               "  | truncSquash (truncX truncY : Trunc A) : Path (Trunc A) truncX truncY\n"
               "def bad : Trunc Nat -> Nat := fun t => truncVal t", "NAME_UNKNOWN");
  ok &= domain("family Circle := circleBase | circleLoop : Path Circle circleBase circleBase\n"
               "def loop : Path Circle circleBase circleBase := circleLoop", NULL);
  /* Z3b: a fold over the formal hcomp is an hcomp at B (D8); at Nat it is its base. */
  ok &= domain("family Circle := circleBase | circleLoop : Path Circle circleBase circleBase\n"
               "def sq : (i : I) -> Circle := fun i => hcomp (fun j => [ i |-> circleLoop @ j ]) circleBase\n"
               "def f : (i : I) -> Nat := fun i => fold 0 refl (sq i)\n"
               "def end1 : Path Nat (f i1) 0 := refl\n"
               "def any : (i : I) -> Path Nat (f i) 0 := fun i => refl", NULL);
  ok &= roundtrip_in("family Circle := circleBase | circleLoop : Path Circle circleBase circleBase",
                     "Circle -> Nat", "fun x => fold 0 refl x");
  ok &= roundtrip_in("family Trunc (A : Type 0) := truncIn (truncVal : A)\n"
                     "  | truncSquash (truncX truncY : Trunc A) : Path (Trunc A) truncX truncY",
                     "Trunc Nat -> Trunc Nat", "fun x => fold (fun a => truncIn Nat a) (truncSquash Nat) x");
  /* Indexed folds recover printed motive domains from the index telescope,
     including an index whose type depends on an earlier index. */
  ok &= roundtrip_in("family F (n : Nat) := makeF (value : Nat) : F value",
                     "(n : Nat) -> F n -> Nat",
                     "fun n x => fold (fun (i : Nat) => Nat) (fun k => k) x");
  ok &= roundtrip_in("family Box (A : Type 0) (x : A) := box (T : Type 0) (v : T) : Box T v",
                     "(A : Type 0) -> (x : A) -> Box A x -> A",
                     "fun A x b => fold (fun (T : Type 0) => fun (v : T) => T) (fun T v => v) b");
  ok &= roundtrip_in("family F (n : Nat) := makeF (value : Nat) : F value",
                     "(n : Nat) -> F n -> Type 0",
                     "fun n x => fold (fun (i : Nat) => Type 0) (fun k => Nat) x");
  /* Z4a: indexed point and path constructors, and both a stuck fold and
     a fold at a path constructor, print and read back. */
  ok &= roundtrip_in(circuit_domain, "Circuit (shape nil) (shape nil)",
                     "circSeq (shape nil) (shape nil) (shape nil) (circId (shape nil)) (circId (shape nil))");
  ok &= roundtrip_in(circuit_domain, "(i : I) -> Circuit (shape nil) (shape nil)",
                     "fun i => circIdl (shape nil) (shape nil) (circId (shape nil)) @ i");
  ok &= roundtrip_in(circuit_domain, "(a : Shape) -> (b : Shape) -> Circuit a b -> Circuit b a",
                     "fun a b x => fold (fun (a : Shape) => fun (b : Shape) => Circuit b a) (fun idA => circId idA)"
                     " (fun seqA seqB seqC seqF seqG => circSeq seqC seqB seqA seqG seqF)"
                     " (fun opA opB opO => circOp opB opA (fold (fun (a : Shape) => fun (b : Shape) => Op b a) (fun qzS qzK => opQuantize qzS qzK) (fun mmN mmM mmK => opMatMul mmM mmN mmK) (fun lkS lkT => opLookup lkS lkT) (fun smS smT smK => opSoftmax smS smT smK) (fun lnS lnG lnT => opLayerNorm lnS lnG lnT) (fun atH atD atQ atK atV atT atE => opAttention atH atD atQ atK atV atT atE) opO))"
                     " (fun rsS rsF => circResidual rsS rsF)"
                     " (fun idlA idlB idlF => circIdr idlB idlA idlF) (fun idrA idrB idrF => circIdl idrB idrA idrF)"
                     " (fun asA asB asC asD asF asG asH => symm (circAssoc asD asC asB asA asH asG asF))"
                     " (fun sqA sqB sqF sqG sqP sqQ => circSquash sqB sqA sqF sqG sqP sqQ) x");
  ok &= roundtrip_in(circuit_domain, "(i : I) -> Circuit (shape nil) (shape nil)",
                     "fun i => fold (fun (a : Shape) => fun (b : Shape) => Circuit b a) (fun idA => circId idA)"
                     " (fun seqA seqB seqC seqF seqG => circSeq seqC seqB seqA seqG seqF)"
                     " (fun opA opB opO => circOp opB opA (fold (fun (a : Shape) => fun (b : Shape) => Op b a) (fun qzS qzK => opQuantize qzS qzK) (fun mmN mmM mmK => opMatMul mmM mmN mmK) (fun lkS lkT => opLookup lkS lkT) (fun smS smT smK => opSoftmax smS smT smK) (fun lnS lnG lnT => opLayerNorm lnS lnG lnT) (fun atH atD atQ atK atV atT atE => opAttention atH atD atQ atK atV atT atE) opO))"
                     " (fun rsS rsF => circResidual rsS rsF)"
                     " (fun idlA idlB idlF => circIdr idlB idlA idlF) (fun idrA idrB idrF => circIdl idrB idrA idrF)"
                     " (fun asA asB asC asD asF asG asH => symm (circAssoc asD asC asB asA asH asG asF))"
                     " (fun sqA sqB sqF sqG sqP sqQ => circSquash sqB sqA sqF sqG sqP sqQ)"
                     " (circIdl (shape nil) (shape nil) (circId (shape nil)) @ i)");
  /* D5: hcomp at a HIT is a formal constructor (canonical, distinct from the
     base) when the faces do not hold, and the side at i1 when they do. */
  ok &= domain("family Circle := circleBase | circleLoop : Path Circle circleBase circleBase\n"
               "def sq : (i : I) -> Circle := fun i => hcomp (fun j => [ i |-> circleLoop @ j ]) circleBase\n"
               "def top : Path Circle (sq i1) circleBase := refl\n"
               "def same : (i : I) -> Path Circle (sq i) (hcomp (fun j => [ i |-> circleLoop @ j ]) circleBase) := fun i => refl\n"
               "def tr : Path Circle circleBase circleBase := trans circleLoop circleLoop\n"
               "def e0 : Path Circle (tr @ i0) circleBase := refl", NULL);
  ok &= domain("family Circle := circleBase | circleLoop : Path Circle circleBase circleBase\n"
               "def sq : (i : I) -> Circle := fun i => hcomp (fun j => [ i |-> circleLoop @ j ]) circleBase\n"
               "def bad : (i : I) -> Path Circle (sq i) circleBase := fun i => refl", "TYPE_ENDPOINT");
  /* Round trips through the printer with the family in scope: a path
     constructor at an interval (print_pctor), a square at two intervals, and
     a formal hcomp at a HIT (print_path_syntax). */
  ok &= roundtrip_in("family Circle := circleBase | circleLoop : Path Circle circleBase circleBase",
                     "(i : I) -> Circle", "fun i => circleLoop @ i");
  ok &= roundtrip_in("family Quot (A : Type 0) (R : A -> A -> Type 0) := quotIn (quotRep : A)\n"
                     "  | quotGlue (quotA quotB : A) (quotR : R quotA quotB) : Path (Quot A R) (quotIn A R quotA) (quotIn A R quotB)\n"
                     "  | quotSquash (quotX quotY : Quot A R) (quotP quotQ : Path (Quot A R) quotX quotY) : Path (Path (Quot A R) quotX quotY) quotP quotQ\n"
                     "def Q : Nat -> Nat -> Type 0 := fun x => fun y => Nat",
                     "(i : I) -> (j : I) -> Quot Nat Q",
                     "fun i j => (quotSquash Nat Q (quotIn Nat Q 1) (quotIn Nat Q 2) (quotGlue Nat Q 1 2 0) (quotGlue Nat Q 1 2 1) @ i) @ j");
  ok &= roundtrip_in("family Circle := circleBase | circleLoop : Path Circle circleBase circleBase",
                     "(i : I) -> Circle", "fun i => hcomp (fun j => [ i |-> circleLoop @ j ]) circleBase");
  /* D4: each end rule of a path constructor. refl quotes its end under the
     binder, so t i0 and u i0 s reach the PCTOR value at an end. */
  ok &= domain("family Quot (A : Type 0) (R : A -> A -> Type 0) := quotIn (quotRep : A)\n"
               "  | quotGlue (quotA quotB : A) (quotR : R quotA quotB) : Path (Quot A R) (quotIn A R quotA) (quotIn A R quotB)\n"
               "  | quotSquash (quotX quotY : Quot A R) (quotP quotQ : Path (Quot A R) quotX quotY) : Path (Path (Quot A R) quotX quotY) quotP quotQ\n"
               "def Q : Nat -> Nat -> Type 0 := fun x => fun y => Nat\n"
               "def g0 : Path (Quot Nat Q) (quotIn Nat Q 1) (quotIn Nat Q 2) := quotGlue Nat Q 1 2 0\n"
               "def g1 : Path (Quot Nat Q) (quotIn Nat Q 1) (quotIn Nat Q 2) := quotGlue Nat Q 1 2 1\n"
               "def sq : Path (Path (Quot Nat Q) (quotIn Nat Q 1) (quotIn Nat Q 2)) g0 g1 := quotSquash Nat Q (quotIn Nat Q 1) (quotIn Nat Q 2) g0 g1\n"
               "def t : (r : I) -> Path (Quot Nat Q) (g0 @ r) (g0 @ r) := fun r => refl\n"
               "def e0 : Path (Path (Quot Nat Q) (quotIn Nat Q 1) (quotIn Nat Q 1)) (t i0) refl := refl\n"
               "def e1 : Path (Path (Quot Nat Q) (quotIn Nat Q 2) (quotIn Nat Q 2)) (t i1) refl := refl\n"
               "def u : (r : I) -> (s : I) -> Path (Quot Nat Q) ((sq @ r) @ s) ((sq @ r) @ s) := fun r => fun s => refl\n"
               "def f0 : (s : I) -> Path (Path (Quot Nat Q) (g0 @ s) (g0 @ s)) (u i0 s) refl := fun s => refl\n"
               "def f1 : (s : I) -> Path (Path (Quot Nat Q) (g1 @ s) (g1 @ s)) (u i1 s) refl := fun s => refl\n"
               "def h0 : (r : I) -> Path (Path (Quot Nat Q) (quotIn Nat Q 1) (quotIn Nat Q 1)) (u r i0) refl := fun r => refl\n"
               "def h1 : (r : I) -> Path (Path (Quot Nat Q) (quotIn Nat Q 2) (quotIn Nat Q 2)) (u r i1) refl := fun r => refl", NULL);
  ok &= domain("def s : Sum Nat Nat := inl 7\ndef bad : Sum Flag Nat := bind s (fun x => inr x)", "TYPE_MISMATCH");
  ok &= domain("family Lot (n : Nat) := makeLot (amount : Nat)\n"
               "def s : Sum Nat Nat := inl 0\n"
               "def f : (n : Nat) -> Nat -> Lot n := fun n x => makeLot n x\n"
               "def bad : (n : Nat) -> Lot n := first (pair (either f f s) 0)", "TYPE_MISMATCH");
  /* Z4a: indexed sorts with path constructors (C5, D1 to D4). Field names
     are domain-wide (rule R4), so each constructor has its own. */
  ok &= domain("family Sh := sh (n : Nat)\n"
               "family C (a b : Sh) :=\n"
               "    cId (ia : Sh) : C ia ia\n"
               "  | cSeq (sa sb sc : Sh) (sf : C sa sb) (sg : C sb sc) : C sa sc\n"
               "  | cIdl (la lb : Sh) (lf : C la lb) : Path (C la lb) (cSeq la la lb (cId la) lf) lf\n"
               "  | cSquash (qa qb : Sh) (qf qg : C qa qb) (qp qq : Path (C qa qb) qf qg) : Path (Path (C qa qb) qf qg) qp qq", NULL);
  ok &= domain("family Sh := sh (n : Nat)\nfamily C (a b : Sh) := cId (ia : Sh) : C ia ia | cBad (ba : Sh)", "DOMAIN_INDEX");
  ok &= domain("family Sh := sh (n : Nat)\nfamily C (a b : Sh) := cId (ia : Sh) : C ia ia | cBad (ba : Sh) (bf : C ba ba) : C ba ((fun (z : C ba ba) => ba) bf)", "DOMAIN_INDEX");
  ok &= domain("family Sh := sh (n : Nat)\nfamily C (a b : Sh) := cId (ia : Sh) : C ia ia | cBad (ba : Sh) (bf : C ba ba) (bg : C ba ((fun (z : C ba ba) => ba) bf)) : C ba ba", "DOMAIN_INDEX");
  ok &= domain("family Sh := sh (n : Nat)\nfamily C (a b : Sh) := cId (ia : Sh) : C ia ia | cBad (ba : Sh) (bf : C ba ba) (be : Eq Sh ((fun (z : C ba ba) => ba) bf) ba) : C ba ba", "DOMAIN_INDEX");
  ok &= domain("family Sh := sh (n : Nat)\nfamily C (a b : Sh) := cId (ia : Sh) : C ia ia | cBad (ba : Sh) : Sh", "DOMAIN_HIT");
  ok &= domain("family Sh := sh (n : Nat)\nfamily C (a b : Sh) := cId (ia : Sh) : C ia ia | cBad (ba : Sh) (bf : C ba ba) : Path (C ba ba) ((fun (z : C ba ba) => z) bf) bf", "DOMAIN_HIT");
  ok &= domain("family Sh := sh (n : Nat)\nfamily C (a b : Sh) := cId (ia : Sh) : C ia ia\ndef bad : C (sh 0) (sh 0) -> Nat := fun x => fold (fun a => 0) x", "TYPE_MISMATCH");
  ok &= domain("family F (n : Nat) := makeF (value : Nat) : F value\n"
               "def bad : (n : Nat) -> F n -> Nat := fun n x => fold (fun (i : Flag) => Nat) (fun k => k) x", "TYPE_MISMATCH");
  ok &= domain("family F (n : Nat) := makeF (value : Nat) : F value\n"
               "def bad : (n : Nat) -> F n -> Nat := fun n x => fold (fun i => 0) (fun k => k) x", "TYPE_MISMATCH");
  ok &= domain("family F := base | loop (x : F) (p : Path F ((fun (z : F) => z) x) x) : Path F x x", "DOMAIN_HIT");
  ok &= domain("family F := base | sq (x : F) : Path (Path F x x) refl refl", "DOMAIN_HIT");
  ok &= domain("family Ta := ta\n  and Tb (tbI : Ta) := tb : Tb ta", "DOMAIN_QIIT");
  ok &= domain("family Ta := ta\n  and Tb (tbI : List Ta) := tb (tbL : List Ta) : Tb tbL", "REFUSE_REC");
  ok &= domain("family Ta := ta (taK : Tb) | tl : Path Ta (ta tb) (ta tb)\n  and Tb := tb", "NAME_UNKNOWN");
  ok &= domain("family Ta := ta\n  and Tb := tb (tbA : Ta) | tbl : Path Tb (tb ta) (tb ta)", NULL);
  ok &= domain("family Ta := ta\n  and Tb (A : Type 0) := tb (tbV : A)", "DOMAIN_QIIT");
  ok &= domain("family Ta := ta\n  and Tb := tb (tbP : Path Ta ta ta)", "DOMAIN_QIIT");
  ok &= domain("family Ta := ta0 | ta (taX : Ta) (taK : Tb ((fun (z : Ta) => 0) taX))\n"
               "  and Tb (n : Nat) := tb (tbN : Nat) : Tb tbN", "DOMAIN_INDEX");
  ok &= domain("family Ta := ta0 | ta (taN : Nat) (taK : Tb taN)\n  and Tb (n : Nat) := tb (tbN : Nat) : Tb tbN", NULL);
  ok &= domain("family Ta := ta\n  and Ta := tb", "REFUSE_NAME");
  ok &= domain("family Ta := ta (taB : Tb)\n  and Tb := tb (tbType : Type 0)\n"
               "def bad : Type 0 := Ta", "TYPE_MISMATCH");
  ok &= domain("family Ta := ta (taB : Tb)\n  and Tb := tb (tbType : Type 0)\n"
               "def good : Type 1 := Ta", NULL);
  ok &= domain("family Ta := ta (taB : Tb)\n  and Tb := tb (tbC : Tc)\n  and Tc := tc (tcType : Type 0)\n"
               "def bad : Type 0 := Ta", "TYPE_MISMATCH");
  ok &= domain("family Ta := ta (taB : Tb)\n  and Tb := tb (tbC : Tc)\n  and Tc := tc (tcType : Type 0)\n"
               "def good : Type 1 := Ta", NULL);
  ok &= domain("family Ta := ta (taB : Tb)\n  and Tb := tb0 | tb (tbA : Ta) (tbType : Type 0)\n"
               "def good : Type 1 := Ta", NULL);
  ok &= domain("family Ta := ta\n  and Tb := tb (tbType : Type 0)\n"
               "def good : Type 0 := Ta", NULL);
  ok &= domain("family Ta (ai : Type 0) := ta : Ta Tb\n  and Tb := tb (tbType : Type 0)", "TYPE_MISMATCH");
  ok &= domain("family Ta (ai : Type 1) := ta : Ta Tb\n  and Tb := tb (tbType : Type 0)", NULL);
  ok &= domain("family Ta (ai : Type 1) := ta : Ta Tb\n  and Tb := tb", "TYPE_MISMATCH");
  ok &= domain("family Ta := ta (taC : Tc)\n  and Tb := tb (tbType : Type 1)\n"
               "  and Tc := tc (tcValue : tbType (tb (Type 0)))\n"
               "def bad : Type 0 := Ta", "TYPE_MISMATCH");
  ok &= domain("family Ta := ta (taC : Tc)\n  and Tb := tb (tbType : Type 1)\n"
               "  and Tc := tc (tcValue : tbType (tb (Type 0)))\n"
               "def good : Type 1 := Ta", NULL);
  ok &= domain("family Ta (ai : Type 0) := ta : Ta Tc\n  and Tb := tb (tbType : Type 1)\n"
               "  and Tc := tc (tcValue : tbType (tb (Type 0)))", "TYPE_MISMATCH");
  ok &= domain("family Ta (ai : Type 1) := ta : Ta Tc\n  and Tb := tb (tbType : Type 1)\n"
               "  and Tc := tc (tcValue : tbType (tb (Type 0)))", NULL);
  ok &= domain("family Ta := ta\n  and Tb := tb\ndef bad : Type 1 := Nat", "TYPE_MISMATCH");
  ok &= domain("family Ta := ta (taK : Tb)\n  and Tb := tb\n"
               "def good : Ta -> Nat := fun x => fold Nat Nat (fun taK => taK) 0 x", NULL);
  ok &= domain("family Ta := ta (taK : Tb)\n  and Tb := tb\n"
               "def bad : Ta -> Nat := fun x => fold Nat (fun taK => taK) 0 x", "TYPE_MISMATCH");
  ok &= domain("def bad : Type 1 := Nat", "TYPE_MISMATCH");
  ok &= domain("def bad : Type 2 := Type 1", "TYPE_UNIVERSE");
  ok &= domain("family U := makeU (decode : Type 0)\ndef bad : Type 0 := U", "TYPE_MISMATCH");
  ok &= domain("family U := makeU (decode : Type 0)\nfamily W := makeW (wrapped : U)\ndef bad : Type 0 := W", "TYPE_MISMATCH");
  ok &= domain("family Box (A : Type 1) := box (unbox : A)\ndef bad : Type 0 := Box (Type 0)", "TYPE_MISMATCH");
  ok &= domain("family U := makeU (decode : Type 0)\ndef good : Type 1 := U", NULL);
  ok &= domain("def eta : (p : Prod Nat Nat) -> Eq (Prod Nat Nat) (pair (first p) (second p)) p := fun p => refl", NULL);
  ok &= domain("def eta : (p : Sigma (n : Nat) (Eq Nat n n)) -> Eq (Sigma (n : Nat) (Eq Nat n n)) (pack (witness p) (payload p)) p := fun p => refl", NULL);
  ok &= roundtrip("Path Nat 0 0", "<i> 0");
  ok &= roundtrip("I -> I -> I", "fun i j => i /\\ j");
  ok &= roundtrip("I -> I -> I", "fun i j => i \\/ j");
  ok &= roundtrip("I -> I", "fun i => ~ i");
  ok &= roundtrip("(p : Path Nat 0 1) -> I -> Nat", "fun p i => p @ i");
  ok &= roundtrip("(p : Path Nat 0 1) -> Path Nat 1 0", "fun p => symm p");
  ok &= roundtrip("Type 0", "Path Nat 0 0");
  ok &= roundtrip("(A : Type 0) -> (a : A) -> Path A a a", "fun A a => refl");
  ok &= domain("def law : (g : I -> I) -> (i : I) -> (p : Path Nat 0 1) -> "
               "Eq Nat (p @ (i0 /\\ g i)) 0 := fun g i p => refl", NULL);
  ok &= domain("def law : (g : I -> I) -> (i : I) -> (p : Path Nat 0 1) -> "
               "Eq Nat (p @ (~ ~ (g i))) (p @ (g i)) := fun g i p => refl", NULL);
  ok &= domain("def law : (g : I -> I) -> (h : I -> I) -> (i : I) -> (p : Path Nat 0 1) -> "
               "Eq Nat (p @ (g i /\\ h i)) (p @ (h i /\\ g i)) := fun g h i p => refl", NULL);
  ok &= domain("def law : (g : (I -> I) -> I) -> (p : Path Nat 0 1) -> "
               "Eq Nat (p @ (g (fun i => i) /\\ g (fun i => ~ ~ i))) "
               "(p @ (g (fun i => i))) := fun g p => refl", NULL);
  ok &= domain("def bad : (g : I -> I) -> (i : I) -> (p : Path Nat 0 1) -> "
               "Eq Nat (p @ (g i /\\ ~ (g i))) 0 := fun g i p => refl", "TYPE_REFL");
  ok &= domain("def bad : (g : I -> I) -> (h : I -> I) -> (i : I) -> (p : Path Nat 0 1) -> "
               "Eq Nat (p @ (g i /\\ h i)) (p @ (g i)) := fun g h i p => refl", "TYPE_REFL");
  ok &= domain("def bad : (g : I -> I) -> (i : I) -> (j : I) -> (p : Path Nat 0 1) -> "
               "Eq Nat (p @ (g i /\\ g j)) (p @ (g i)) := fun g i j p => refl", "TYPE_REFL");
  ok &= domain("def f : Nat -> Nat := fun x => x\n"
               "def law : Path (Nat -> Nat) f f := funExt (fun x => refl)", NULL);
  ok &= domain("def law : (B : Nat -> Type 0) -> (f : (x : Nat) -> B x) -> "
               "Path ((x : Nat) -> B x) f f := fun B f => funExt (fun x => refl)", NULL);
  ok &= domain("def f : Nat -> Nat := fun x => x\n"
               "def g : Nat -> Nat := fun x => natAdd x 1\n"
               "def bad : Path (Nat -> Nat) f g := funExt (fun x => refl)", "TYPE_ENDPOINT");
  ok &= roundtrip("Partial i1 Nat", "[ i1 |-> 7 ]");
  ok &= roundtrip("Nat -> Partial i1 Nat", "fun x => [ i1 |-> x ]");
  ok &= roundtrip("(i : I) -> Partial (i \\/ ~ i) Nat",
                  "fun i => [ i |-> 0 , ~ i |-> 1 ]");
  ok &= roundtrip("(p : Partial i1 Nat) -> Nat", "fun p => fromPartial p");
  ok &= roundtrip("(i : I) -> Partial (i /\\ ~ i) Nat", "fun i => []");
  ok &= roundtrip("(A : Type 0) -> (u : A) -> A", "fun A => fun u => transp (fun _ => A) i0 u");
  ok &= roundtrip("(A : Type 0) -> (B : Type 0) -> (q : Path (Type 0) A B) -> (f : A -> Nat) -> B -> Nat",
                  "fun A => fun B => fun q => fun f => transp (fun i => q @ i -> Nat) i0 f");
  ok &= roundtrip("(A : Type 0) -> (a : A) -> (b : A) -> Path A a b -> Eq A a b",
                  "fun A a b p => transp (fun i => Eq A a (p @ i)) i0 refl");
  ok &= roundtrip("(A : Type 0) -> (P : A -> Type 0) -> (a : A) -> (b : A) -> (p : Path A a b) -> P a -> P b",
                  "fun A P a b p u => transport P (transp (fun i => Eq A a (p @ i)) i0 refl) u");
  ok &= roundtrip("(A : Type 0) -> (a : A) -> (b : A) -> Path A a b -> Path A a b",
                  "fun A a b p => <i> hcomp (fun j => [ ~ i |-> a , i |-> b ]) (p @ i)");
  ok &= roundtrip("(f : Nat -> Nat) -> Path (Nat -> Nat) f f", "fun f => <i> hcomp (fun j => [ ~ i |-> f , i |-> f ]) f");
  ok &= roundtrip("(s : Sigma (x : Nat) Nat) -> Sigma (x : Nat) Nat", "fun s => hcomp (fun j => []) s");
  ok &= roundtrip("(q : Path Nat 3 3) -> Path Nat 3 3", "fun q => transp (fun i => Path Nat 3 3) i0 q");
  ok &= roundtrip("(p : Path Nat 3 4) -> (q : Path Nat 4 5) -> Path Nat 3 5", "fun p q => trans p q");
  ok &= roundtrip("(A : I -> Type 0) -> (u : A i0) -> (r : I) -> A r",
                  "fun A u r => fill A (fun j => []) u r");
  ok &= roundtrip("(P : I -> I -> Type 0) -> (a : (i : I) -> P i i0) -> (b : (i : I) -> P i i1) -> (p : PathP (P i0) (a i0) (b i0)) -> PathP (P i1) (a i1) (b i1)",
                  "fun P a b p => transp (fun i => PathP (P i) (a i) (b i)) i0 p");
  ok &= roundtrip("(A : Type 0) -> (B : A -> Type 0) -> (s : Sigma (x : A) (B x)) -> Sigma (x : A) (B x)",
                  "fun A B s => hcomp (fun j => []) s");
  ok &= domain("def good : (A : I -> Type 0) -> (u : A i0) -> (r : I) -> (s : I) -> Partial (~ r \\/ ~ s) (A (r /\\ s)) := fun A u r s => [ ~ r \\/ ~ s |-> u ]", NULL);
  ok &= domain("def bad : (A : I -> Type 0) -> (u : A i0) -> (r : I) -> (s : I) -> Partial (~ r \\/ s) (A (r /\\ s)) := fun A u r s => [ ~ r \\/ s |-> u ]", "TYPE_MISMATCH");
  ok &= domain("def good : (A : I -> Type 0) -> (u : A i0) -> (r : I) -> Partial (~ r) (A r) := fun A u r => [ ~ r |-> hcomp (fun j => [ i1 |-> u ]) u ]", NULL);
  ok &= domain("def good : (A : I -> Type 0) -> (u : A i0) -> (r : I) -> Partial (~ r) (A r) := fun A u r => [ ~ r |-> transp (fun j => A (r /\\ j)) i1 u ]", NULL);
  ok &= domain("def bad : (A : I -> Type 0) -> (u : A i0) -> (r : I) -> A r := fun A u r => (fun (s : Partial (~ r) (A r)) => u) [ ~ r |-> u ]", "TYPE_MISMATCH");
  ok &= domain("def bad : (A : I -> Type 0) -> (u : A i0) -> (r : I) -> Partial (~ r) (A r) := fun A u r => [ ~ r |-> missing ]", "NAME_UNKNOWN");
  ok &= domain("def bad : Partial i0 Nat := [ i0 |-> flagYes ]", "TYPE_MISMATCH");
  ok &= domain("def good : (p : Path Nat 3 4) -> (r : I) -> Partial (~ r) Nat := fun p r => [ ~ r |-> hcomp (fun j => [ i1 |-> p @ r , i1 |-> 3 ]) 3 ]", NULL);
  ok &= domain("def bad : (p : Path Nat 3 4) -> (r : I) -> Partial (~ r) Nat := fun p r => [ ~ r |-> hcomp (fun j => [ i1 |-> p @ r , i1 |-> 4 ]) 3 ]", "TYPE_SYSTEM");
  ok &= domain("def good : (p : Path Nat 3 4) -> (r : I) -> Partial (~ r) (Path Nat (p @ r) 3) := fun p r => [ ~ r |-> refl ]", NULL);
  ok &= domain("def wrap : Nat -> Partial i1 Nat := fun x => [ i1 |-> x ]\n"
               "def law : (x : Nat) -> Eq Nat (fromPartial (wrap x)) x := fun x => refl", NULL);
  ok &= domain("def wrap : Nat -> Partial i1 Nat := fun x => [ i1 |-> natAdd x 1 ]\n"
               "def law : (x : Nat) -> Eq Nat (fromPartial (wrap x)) (natAdd x 1) := fun x => refl", NULL);
  ok &= domain("def wrap : (Nat -> Nat) -> Partial i1 (Nat -> Nat) := fun f => [ i1 |-> f ]\n"
               "def law : (f : Nat -> Nat) -> Eq (Nat -> Nat) (fromPartial (wrap f)) f := fun f => refl", NULL);
  ok &= domain("def wrap : (i : I) -> Nat -> Partial i1 Nat := fun i x => [ i |-> x , i1 |-> x ]\n"
               "def law : (i : I) -> (x : Nat) -> Eq Nat (fromPartial (wrap i x)) x := fun i x => refl", NULL);
  ok &= domain("def empty : (i : I) -> Partial (i /\\ ~ i) Nat -> Partial i0 Nat := fun i p => p", NULL);
  ok &= domain("def empty : (g : I -> I) -> (i : I) -> Partial (g i /\\ ~ (g i)) Nat -> "
               "Partial i0 Nat := fun g i p => p", NULL);
  ok &= domain("def bad : (i : I) -> Partial (i \\/ ~ i) Nat -> Partial i1 Nat := fun i p => p", "TYPE_MISMATCH");
  ok &= domain("def same : (a : I) -> (b : I) -> (c : I) -> (d : I) -> (e : I) -> "
               "(f : I) -> (g : I) -> (h : I) -> (i : I) -> "
               "Partial (a /\\ b /\\ c /\\ d /\\ e /\\ f /\\ g /\\ h /\\ i) Nat -> "
               "Partial (a /\\ b /\\ c /\\ d /\\ e /\\ f /\\ g /\\ h /\\ i) Nat := fun a b c d e f g h i p => p", NULL);
  ok &= roundtrip_in("def ignore : (A : Type 0) -> A -> Nat -> Nat := fun A a x => x",
                     "Path (Nat -> Nat) (ignore Nat 0) (ignore (Nat -> Nat) (fun x => x))", "refl");
  if (ok) puts("domain, path and system regressions: passed");
  return ok ? 0 : 1;
}

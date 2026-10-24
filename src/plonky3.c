/* The Plonky3 printer (slice Z10c, flag 91 (a)). `build/plonky3print NAME DIR`
   reads the words of one Plonky3 entry on stdin: the `langc eval` output of an
   entry that `p3Words` (examples/plonky3.lang) gives.

   The words (D3, flag 90 (a)) are 3 blocks on one line:
   1. The stage program: the stage count t, then one record for each stage in
      run order: 1 k s (matmul: the matrix index k, the source s), 2 (zero),
      3 s u (residual: the residual input s, the inner result u), 4 (lookup).
      A source is 0 for the inputs, or j for stage j. The first stage is
      stage 1.
   2. The inputs: n, then n elements.
   3. The Z7 witness (`bbWitnessWords`): the version 1, the field id 1, the
      element size 1, the matrix count, the words of each matrix (the rank 2,
      the row count, the column count, the entries), the stage count, then
      the words of each stage (the rank, the dims, the aux word 0, the
      entries). The entries are in row-major order.
   An element is one word below the BabyBear modulus p. Plonky3 is BabyBear
   only (flag 90), so the field id of block 3 must be 1.

   The word reader is a copy of the src/halo2.c reader (flag 84 (a)), so
   src/llzk.c and src/halo2.c do not change.

   The printer checks NAME (a to z and 0 to 9) and all of the words first. A
   refusal exits 1 with one message line on stderr and writes no file. For an
   accepted entry, the printer writes DIR/Cargo.toml and DIR/src/lib.rs (D10):
   the AIR of the D5 layout and the D7 tests. It prints nothing and exits 0. */
#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

enum {
  TAG_MATMUL = 1,
  TAG_ZERO = 2,
  TAG_RESIDUAL = 3,
  TAG_LOOKUP = 4
};

/* The BabyBear modulus p = 15 * 2^27 + 1. */
static const uint32_t BABYBEAR_P = 2013265921u;

static const char *const SHORT = "the block is short";
static const char *const NOT_WORD = "a word is not a decimal number below 2^32";
static const char *const NO_MEMORY = "no memory";
static const char *const BAD_NAME = "the name is empty or has a character that is not a to z or 0 to 9";

typedef struct {
  uint32_t *word;
  size_t count;
  size_t cap;
} Words;

/* One witness matrix: the row count, the column count, the first word of its
   entries and its entry count. */
typedef struct {
  uint32_t rows;
  uint32_t cols;
  size_t at;
  uint64_t entries;
} Matrix;

/* One stage. From block 1: the tag, the matrix index k (matmul), the source s
   (matmul and residual) and the source u (residual). From block 3: the rank,
   the first word of the entries and the entry count. Stage 0 is the inputs:
   its entries are the n elements of block 2. */
typedef struct {
  uint32_t tag;
  uint32_t k;
  uint32_t s;
  uint32_t u;
  uint32_t rank;
  size_t at;
  uint64_t entries;
} Stage;

/* The checked entry. stage has t + 1 items (stage 0 is the inputs). weights is
   m, the sum of the matrix entry counts. outputs is q, the entry count of stage
   t (n when t is 0). width is w = n + m + the sum of the stage entry counts
   (D5). */
typedef struct {
  uint32_t stages;
  Stage *stage;
  uint32_t inputs;
  uint32_t matrices;
  Matrix *matrix;
  uint64_t weights;
  uint64_t outputs;
  uint64_t width;
} Entry;

/* push appends one word. It returns 0 when there is no memory. */
static int push(Words *w, uint32_t v)
{
  if (w->count == w->cap) {
    size_t cap = w->cap == 0 ? 64 : w->cap * 2;
    uint32_t *grown = realloc(w->word, cap * sizeof *grown);
    if (grown == NULL)
      return 0;
    w->word = grown;
    w->cap = cap;
  }
  w->word[w->count++] = v;
  return 1;
}

/* read_words reads the words on stdin. It returns NULL or a refusal. */
static const char *read_words(Words *w)
{
  uint64_t v = 0;
  int digits = 0;
  int c;
  while ((c = getchar()) != EOF) {
    if (c == ' ' || c == '\n') {
      if (digits && !push(w, (uint32_t)v))
        return NO_MEMORY;
      v = 0;
      digits = 0;
      continue;
    }
    if (c < '0' || c > '9')
      return NOT_WORD;
    v = v * 10 + (uint64_t)(c - '0');
    if (v > UINT32_MAX)
      return NOT_WORD;
    digits = 1;
  }
  if (ferror(stdin))
    return "the input is not read";
  if (digits && !push(w, (uint32_t)v))
    return NO_MEMORY;
  return NULL;
}

/* elements checks that count elements start at the word at and that each
   element is below p. at is not above the word count. It returns NULL or a
   refusal. */
static const char *elements(const Words *w, size_t at, uint64_t count)
{
  if (w->count - at < count)
    return SHORT;
  for (uint64_t e = 0; e < count; e++)
    if (w->word[at + e] >= BABYBEAR_P)
      return "an element is not below the field modulus";
  return NULL;
}

/* program reads block 1 into e->stage and sets *at to the first word after
   it. Each tag is known, a lookup record is refused, and each source is below
   its stage number. */
static const char *program(Entry *e, const Words *w, size_t *at)
{
  if (w->count == 0)
    return SHORT;
  e->stages = w->word[0];
  *at = 1;
  /* Each record has one word or more. */
  if (e->stages > w->count - *at)
    return SHORT;
  e->stage = calloc((size_t)e->stages + 1, sizeof *e->stage);
  if (e->stage == NULL)
    return NO_MEMORY;
  for (uint32_t i = 0; i < e->stages; i++) {
    Stage *st = &e->stage[i + 1];
    if (*at >= w->count)
      return SHORT;
    st->tag = w->word[*at];
    if (st->tag == TAG_LOOKUP)
      return "lookup is refused (flag 69 (a))";
    if (st->tag != TAG_MATMUL && st->tag != TAG_ZERO && st->tag != TAG_RESIDUAL)
      return "a stage tag is not known";
    const size_t operands = st->tag == TAG_ZERO ? 0 : 2;
    if (w->count - *at - 1 < operands)
      return SHORT;
    if (st->tag == TAG_MATMUL) {
      st->k = w->word[*at + 1];
      st->s = w->word[*at + 2];
    }
    if (st->tag == TAG_RESIDUAL) {
      st->s = w->word[*at + 1];
      st->u = w->word[*at + 2];
    }
    /* Stage i + 1. calloc makes the sources of a zero stage and the u of a
       matmul stage 0. */
    if (st->s > i || st->u > i)
      return "a source is not below its stage number";
    *at += 1 + operands;
  }
  return NULL;
}

/* matrices reads the matrices of block 3 from the word *at into e->matrix. */
static const char *matrices(Entry *e, const Words *w, size_t *at)
{
  for (uint32_t i = 0; i < e->matrices; i++) {
    if (w->count - *at < 3)
      return SHORT;
    const uint32_t *head = w->word + *at;
    if (head[0] != 2)
      return "a matrix rank is not 2";
    Matrix *m = &e->matrix[i];
    m->rows = head[1];
    m->cols = head[2];
    m->at = *at + 3;
    m->entries = (uint64_t)head[1] * head[2];
    const char *msg = elements(w, m->at, m->entries);
    if (msg != NULL)
      return msg;
    e->weights += m->entries;
    *at = m->at + m->entries;
  }
  return NULL;
}

/* stages reads the stages of block 3 from the word *at into e->stage. The
   witness stage count must be t. */
static const char *stages(Entry *e, const Words *w, size_t *at)
{
  if (*at >= w->count)
    return SHORT;
  if (w->word[*at] != e->stages)
    return "the witness stage count is not the stage count";
  *at += 1;
  for (uint32_t i = 0; i < e->stages; i++) {
    Stage *st = &e->stage[i + 1];
    if (*at >= w->count)
      return SHORT;
    const uint32_t rank = w->word[*at];
    *at += 1;
    if (w->count - *at < (uint64_t)rank + 1)
      return SHORT;
    /* A zero dimension makes the stage empty, even after a large prefix. */
    uint64_t entries = 1;
    for (uint32_t d = 0; d < rank; d++)
      if (w->word[*at + d] == 0)
        entries = 0;
    /* A nonempty product above the word count cannot fit, so it is short. */
    for (uint32_t d = 0; d < rank; d++) {
      const uint32_t dim = w->word[*at + d];
      if (dim != 0 && entries > w->count / dim)
        return SHORT;
      entries *= dim;
    }
    *at += rank;
    if (w->word[*at] != 0)
      return "a stage aux word is not 0";
    *at += 1;
    const char *msg = elements(w, *at, entries);
    if (msg != NULL)
      return msg;
    st->rank = rank;
    st->at = *at;
    st->entries = entries;
    *at += entries;
  }
  return NULL;
}

/* witness reads block 3 from the word *at: the header, the matrices and the
   stages. No word follows it. */
static const char *witness(Entry *e, const Words *w, size_t *at)
{
  if (w->count - *at < 4)
    return SHORT;
  const uint32_t *head = w->word + *at;
  if (head[1] != 1)
    return "the field id is not 1";
  if (head[0] != 1 || head[2] != 1)
    return "the witness header is not 1 1 1";
  e->matrices = head[3];
  *at += 4;
  if (e->matrices > (w->count - *at) / 3)
    return SHORT;
  e->matrix = calloc((size_t)e->matrices + 1, sizeof *e->matrix);
  if (e->matrix == NULL)
    return NO_MEMORY;
  const char *msg = matrices(e, w, at);
  if (msg == NULL)
    msg = stages(e, w, at);
  if (msg != NULL)
    return msg;
  if (*at != w->count)
    return "a word follows the witness block";
  return NULL;
}

/* shapes checks each stage against its matrix and its sources, then sets q
   and w (D3, D5). */
static const char *shapes(Entry *e)
{
  uint64_t entries = 0;
  for (uint32_t i = 0; i < e->stages; i++) {
    const Stage *st = &e->stage[i + 1];
    if (st->tag == TAG_MATMUL) {
      if (st->k >= e->matrices)
        return "a matrix index is not below the matrix count";
      const Matrix *m = &e->matrix[st->k];
      if (st->rank != 1)
        return "a matmul stage does not have rank 1";
      if (st->entries != m->rows)
        return "the matmul stage entry count is not the row count of its matrix";
      if (m->cols != e->stage[st->s].entries)
        return "the matrix column count is not the entry count of the matmul source";
    }
    if (st->tag == TAG_RESIDUAL &&
        (e->stage[st->s].entries != st->entries || e->stage[st->u].entries != st->entries))
      return "a residual source entry count is not the stage entry count";
    entries += st->entries;
  }
  e->outputs = e->stage[e->stages].entries;
  e->width = e->inputs + e->weights + entries;
  return NULL;
}

/* check_entry checks the 3 blocks into e. It returns NULL or a refusal. */
static const char *check_entry(Entry *e, const Words *w)
{
  size_t at = 0;
  const char *msg = program(e, w, &at);
  if (msg != NULL)
    return msg;
  /* Block 2: the inputs. Stage 0 has the n entries. */
  if (at >= w->count)
    return SHORT;
  e->inputs = w->word[at];
  e->stage[0].at = at + 1;
  e->stage[0].entries = e->inputs;
  msg = elements(w, e->stage[0].at, e->inputs);
  if (msg != NULL)
    return msg;
  at = e->stage[0].at + e->inputs;
  msg = witness(e, w, &at);
  if (msg == NULL)
    msg = shapes(e);
  return msg;
}

/* name_ok reports if name is one or more of the characters a to z and 0 to 9
   (a part of a Cargo package name). */
static int name_ok(const char *name)
{
  if (*name == '\0')
    return 0;
  for (const char *c = name; *c != '\0'; c++)
    if (!((*c >= 'a' && *c <= 'z') || (*c >= '0' && *c <= '9')))
      return 0;
  return 1;
}


/* CARGO_DEPS: the dependencies of Cargo.toml (the Z10a sample, flags 87 (a)
   and 95 (a)). */
static const char *const CARGO_DEPS[] = {
  "",
  "[dependencies]",
  "# Flag 87 (a): Plonky3 rev 5fcf8d7c (workspace version 0.4.2), from the cargo",
  "# git db (the lambdaworks lock pins the same rev).",
  "p3-air = { git = \"https://github.com/Plonky3/Plonky3\", rev = \"5fcf8d7c78666dd10304d45ae47b71c9e79bd141\" }",
  "p3-baby-bear = { git = \"https://github.com/Plonky3/Plonky3\", rev = \"5fcf8d7c78666dd10304d45ae47b71c9e79bd141\" }",
  "p3-field = { git = \"https://github.com/Plonky3/Plonky3\", rev = \"5fcf8d7c78666dd10304d45ae47b71c9e79bd141\" }",
  "p3-matrix = { git = \"https://github.com/Plonky3/Plonky3\", rev = \"5fcf8d7c78666dd10304d45ae47b71c9e79bd141\" }",
  "",
  "[dev-dependencies]",
  "# Flag 95 (a): the BabyBear config of uni-stark/tests/fib_air.rs at the rev",
  "# (Poseidon2 hash and compress, a Merkle tree MMCS, Radix2DitParallel,",
  "# DuplexChallenger, the test FRI params, rand seed 1).",
  "p3-challenger = { git = \"https://github.com/Plonky3/Plonky3\", rev = \"5fcf8d7c78666dd10304d45ae47b71c9e79bd141\" }",
  "p3-commit = { git = \"https://github.com/Plonky3/Plonky3\", rev = \"5fcf8d7c78666dd10304d45ae47b71c9e79bd141\" }",
  "p3-dft = { git = \"https://github.com/Plonky3/Plonky3\", rev = \"5fcf8d7c78666dd10304d45ae47b71c9e79bd141\" }",
  "p3-fri = { git = \"https://github.com/Plonky3/Plonky3\", rev = \"5fcf8d7c78666dd10304d45ae47b71c9e79bd141\" }",
  "p3-merkle-tree = { git = \"https://github.com/Plonky3/Plonky3\", rev = \"5fcf8d7c78666dd10304d45ae47b71c9e79bd141\" }",
  "p3-symmetric = { git = \"https://github.com/Plonky3/Plonky3\", rev = \"5fcf8d7c78666dd10304d45ae47b71c9e79bd141\" }",
  "p3-uni-stark = { git = \"https://github.com/Plonky3/Plonky3\", rev = \"5fcf8d7c78666dd10304d45ae47b71c9e79bd141\" }",
  "rand = { version = \"0.9\", default-features = false, features = [\"small_rng\"] }",
};

/* LIB_HEAD: the doc comment after its first line, and the use block. */
static const char *const LIB_HEAD[] = {
  "//!",
  "//! Printed by plonky3print from the words of the entry (slice Z10c).",
  "//!",
  "//! The layout (D5 (a)): one wide row. The columns are the inputs, the weight",
  "//! entries (the matrices in order, row-major), then the entries of each stage.",
  "//! The public values are the inputs, then the outputs. The constraints have no",
  "//! row selector, so each row of the trace must hold the same values.",
  "",
  "use p3_air::{Air, AirBuilder, AirBuilderWithPublicValues, BaseAir};",
  "use p3_baby_bear::BabyBear;",
  "use p3_field::PrimeCharacteristicRing;",
  "use p3_matrix::dense::RowMajorMatrix;",
  "use p3_matrix::Matrix;",
};

/* LIB_HEIGHT: the HEIGHT constant (H 1, a Z10a fact). */
static const char *const LIB_HEIGHT[] = {
  "",
  "/// The count of the rows: a power of two, the least that `prove` accepts at",
  "/// the test config.",
  "pub const HEIGHT: usize = 1;",
};

/* LIB_EVAL: the head of Air::eval, up to the cell pattern. */
static const char *const LIB_EVAL[] = {
  "    fn eval(&self, builder: &mut AB) {",
  "        let main = builder.main();",
  "        let cells = main",
  "            .row_slice(0)",
  "            .and_then(|row| <&[AB::Var; WIDTH]>::try_from(&*row).ok().cloned())",
  "            .zip(<[AB::PublicVar; PUBLIC_COUNT]>::try_from(builder.public_values()).ok());",
  "        // A row or a public value list of a different length fails here.",
  "        builder.assert_zero(AB::Expr::from_bool(cells.is_none()));",
  "        // AB::Var is Clone (not Copy) at the rev: each use before the last is a clone.",
  "        cells",
  "            .into_iter()",
};

/* LIB_CLOSE: the end of Air::eval, then the field and trace functions. */
static const char *const LIB_CLOSE[] = {
  "            });",
  "    }",
  "}",
  "",
  "/// The values as BabyBear elements.",
  "pub fn field<const N: usize>(values: [u32; N]) -> [BabyBear; N] {",
  "    values.map(BabyBear::new)",
  "}",
  "",
  "/// The trace: `first` is row 0, then HEIGHT - 1 copies of ROW.",
  "pub fn trace(first: &[BabyBear]) -> RowMajorMatrix<BabyBear> {",
  "    RowMajorMatrix::new(",
  "        first",
  "            .iter()",
  "            .copied()",
  "            .chain(core::iter::repeat_n(field(ROW), HEIGHT - 1).flatten())",
  "            .collect(),",
  "        WIDTH,",
  "    )",
  "}",
};

/* LIB_CONFIG: the test imports and the test config (flag 95 (a)), up to the
   prove call. */
static const char *const LIB_CONFIG[] = {
  "    use p3_baby_bear::{BabyBear, Poseidon2BabyBear};",
  "    use p3_challenger::DuplexChallenger;",
  "    use p3_commit::ExtensionMmcs;",
  "    use p3_dft::Radix2DitParallel;",
  "    use p3_field::extension::BinomialExtensionField;",
  "    use p3_field::{Field, PrimeCharacteristicRing};",
  "    use p3_fri::{create_test_fri_params, TwoAdicFriPcs};",
  "    use p3_merkle_tree::MerkleTreeMmcs;",
  "    use p3_symmetric::{PaddingFreeSponge, TruncatedPermutation};",
  "    use p3_uni_stark::{prove, verify, StarkConfig};",
  "    use rand::rngs::SmallRng;",
  "    use rand::SeedableRng;",
  "",
  "    // The BabyBear config of uni-stark/tests/fib_air.rs at the rev (test",
  "    // values, not a secure config).",
  "    type Val = BabyBear;",
  "    type Perm = Poseidon2BabyBear<16>;",
  "    type MyHash = PaddingFreeSponge<Perm, 16, 8, 8>;",
  "    type MyCompress = TruncatedPermutation<Perm, 2, 8, 16>;",
  "    type ValMmcs =",
  "        MerkleTreeMmcs<<Val as Field>::Packing, <Val as Field>::Packing, MyHash, MyCompress, 8>;",
  "    type Challenge = BinomialExtensionField<Val, 4>;",
  "    type ChallengeMmcs = ExtensionMmcs<Val, Challenge, ValMmcs>;",
  "    type Challenger = DuplexChallenger<Val, Perm, 16, 8>;",
  "    type Dft = Radix2DitParallel<Val>;",
  "    type Pcs = TwoAdicFriPcs<Val, Dft, ValMmcs, ChallengeMmcs>;",
  "    type MyConfig = StarkConfig<Pcs, Challenge, Challenger>;",
  "",
  "    fn config() -> MyConfig {",
  "        let mut rng = SmallRng::seed_from_u64(1);",
  "        let perm = Perm::new_from_rng_128(&mut rng);",
  "        let hash = MyHash::new(perm.clone());",
  "        let compress = MyCompress::new(perm.clone());",
  "        let val_mmcs = ValMmcs::new(hash, compress);",
  "        let challenge_mmcs = ChallengeMmcs::new(val_mmcs.clone());",
  "        let fri_params = create_test_fri_params(challenge_mmcs, 0);",
  "        let pcs = Pcs::new(Dft::default(), val_mmcs, fri_params);",
  "        MyConfig::new(pcs, Challenger::new(perm))",
  "    }",
  "",
  "    /// Prove the trace with `first` as row 0, then verify the proof. In the",
  "    /// dev profile, `prove` runs the debug check first: a failed constraint",
  "    /// panics there.",
  "    fn prove_and_verify(first: &[BabyBear]) -> Result<(), String> {",
  "        let config = config();",
  "        let public = field(PUBLIC);",
};

/* LIB_TESTS: bump and accepts_z7_trace (D7). */
static const char *const LIB_TESTS[] = {
  "    }",
  "",
  "    /// ROW with the entry in `column` plus one.",
  "    fn bump(column: usize) -> Vec<BabyBear> {",
  "        field(ROW)",
  "            .into_iter()",
  "            .enumerate()",
  "            .map(|(i, v)| if i == column { v + BabyBear::ONE } else { v })",
  "            .collect()",
  "    }",
  "",
  "    #[test]",
  "    fn accepts_z7_trace() -> Result<(), String> {",
  "        prove_and_verify(&field(ROW))",
  "    }",
};

/* A weight with zero coefficients can change without invalidating the trace. */
static const char *const LIB_WEIGHT_ACCEPT[] = {
  "",
  "    #[test]",
  "    fn accepts_mutated_weight() -> Result<(), String> {",
  "        prove_and_verify(&bump(FIRST_WEIGHT))",
  "    }",
};

/* The crate to write: the name, the checked entry, its words and the use
   counts of the cells (one item for each column; all 0 between the calls of
   put_lib). */
typedef struct {
  const char *name;
  const Entry *e;
  const Words *w;
  uint32_t *left;
} Crate;

/* LINES gives the lines of a fixed text and their count. */
#define LINES(text) (text), sizeof(text) / sizeof *(text)

/* put_lines prints count lines. */
static void put_lines(FILE *f, const char *const *line, size_t count)
{
  for (size_t i = 0; i < count; i++)
    fprintf(f, "%s\n", line[i]);
}

/* say prints to f. It prints nothing when f is NULL (the count pass of
   put_constraints). */
static void say(FILE *f, const char *format, ...)
{
  if (f == NULL)
    return;
  va_list ap;
  va_start(ap, format);
  vfprintf(f, format, ap);
  va_end(ap);
}

/* col gives the column of entry i of stage j (D5): i for stage 0 (the inputs),
   else n + m + the entry counts of the stages before j + i. */
static uint64_t col(const Entry *e, uint32_t j, uint64_t i)
{
  uint64_t c = j == 0 ? 0 : e->inputs + e->weights;
  for (uint32_t x = 1; x < j; x++)
    c += e->stage[x].entries;
  return c + i;
}

/* weight gives the column of W_k[i][j] (D5): n + the entry counts of the
   matrices before k + i * cols + j. */
static uint64_t weight(const Entry *e, uint32_t k, uint64_t i, uint64_t j)
{
  uint64_t c = e->inputs;
  for (uint32_t x = 0; x < k; x++)
    c += e->matrix[x].entries;
  return c + i * e->matrix[k].cols + j;
}

/* put_var prints the cell of column c. AB::Var is Clone, not Copy, at the
   rev: a use before the last use of the cell is a clone. The count pass (f is
   NULL) counts the uses of the cell in left. */
static void put_var(FILE *f, uint32_t *left, uint64_t c)
{
  if (f == NULL)
    left[c]++;
  else
    fprintf(f, "c%llu%s", (unsigned long long)c, --left[c] > 0 ? ".clone()" : "");
}

/* put_constraints prints one line for each constraint of D5, or counts the
   uses of each cell when f is NULL. */
static void put_constraints(FILE *f, const Entry *e, uint32_t *left)
{
  const char *const indent = "                ";
  if (e->width == 0) {
    say(f, "%s// An empty computation has one zero padding column.\n%sbuilder.assert_zero(", indent, indent);
    put_var(f, left, 0);
    say(f, ");\n");
    return;
  }
  say(f, "%s// Input i is public value i.\n", indent);
  for (uint32_t i = 0; i < e->inputs; i++) {
    say(f, "%sbuilder.assert_eq(", indent);
    put_var(f, left, i);
    say(f, ", p%u);\n", i);
  }
  for (uint32_t j = 1; j <= e->stages; j++) {
    const Stage *st = &e->stage[j];
    const uint32_t cols = st->tag == TAG_MATMUL ? e->matrix[st->k].cols : 0;
    if (st->tag == TAG_MATMUL)
      say(f, "%s// Stage %u: entry i is the sum over j of W_%u[i][j] times entry j of stage %u.\n",
          indent, j, st->k, st->s);
    if (st->tag == TAG_RESIDUAL)
      say(f, "%s// Stage %u: entry i is entry i of stage %u plus entry i of stage %u.\n",
          indent, j, st->s, st->u);
    if (st->tag == TAG_ZERO)
      say(f, "%s// Stage %u: each entry is 0.\n", indent, j);
    for (uint64_t i = 0; i < st->entries; i++) {
      const int zero = st->tag == TAG_ZERO || (st->tag == TAG_MATMUL && cols == 0);
      say(f, "%sbuilder.%s(", indent, zero ? "assert_zero" : "assert_eq");
      put_var(f, left, col(e, j, i));
      for (uint32_t x = 0; x < cols; x++) {
        say(f, x == 0 ? ", " : " + ");
        put_var(f, left, weight(e, st->k, i, x));
        say(f, " * ");
        put_var(f, left, col(e, st->s, x));
      }
      if (st->tag == TAG_RESIDUAL) {
        say(f, ", ");
        put_var(f, left, col(e, st->s, i));
        say(f, " + ");
        put_var(f, left, col(e, st->u, i));
      }
      say(f, ");\n");
    }
  }
  say(f, "%s// Output i (entry i of the last stage) is public value %u + i.\n", indent, e->inputs);
  for (uint64_t i = 0; i < e->outputs; i++) {
    say(f, "%sbuilder.assert_eq(", indent);
    put_var(f, left, col(e, e->stages, i));
    say(f, ", p%llu);\n", (unsigned long long)(e->inputs + i));
  }
}

/* put_words prints count words from the word at, with ", " before each word
   but the first of the list (*first is 1 at the start of the list). */
static void put_words(FILE *f, const Words *w, size_t at, uint64_t count, int *first)
{
  for (uint64_t i = 0; i < count; i++) {
    fprintf(f, "%s%u", *first ? "" : ", ", w->word[at + i]);
    *first = 0;
  }
}

/* put_air prints the name of the AIR struct: NAME with its first letter in
   upper case, then Air ("Case" first when NAME starts with a digit). */
static void put_air(FILE *f, const char *name)
{
  fprintf(f, "%s%c%sAir", *name >= '0' && *name <= '9' ? "Case" : "",
          *name >= 'a' && *name <= 'z' ? *name - 'a' + 'A' : *name, name + 1);
}

/* put_cargo prints Cargo.toml (the Z10a sample form, D10). The crate is a
   member of the workspace of make plonky3-check, so it has no [workspace]. */
static void put_cargo(FILE *f, const Crate *c)
{
  fprintf(f,
          "[package]\n"
          "name = \"zkml-plonky3-%s\"\n"
          "version = \"0.1.0\"\n"
          "edition = \"2021\"\n"
          "license = \"MIT OR Apache-2.0\"\n"
          "description = \"The Plonky3 AIR of the zkml-lang entry %sPlonky3 (printed by plonky3print)\"\n"
          "publish = false\n",
          c->name, c->name);
  put_lines(f, LINES(CARGO_DEPS));
}

/* The first constraint affected by a stage mutation determines its panic. */
static int stage_zero(const Entry *e, uint32_t j)
{
  const Stage *st = &e->stage[j];
  return st->tag == TAG_ZERO ||
         (st->tag == TAG_MATMUL && e->matrix[st->k].cols == 0);
}

static void put_rejection(FILE *f, const char *name, const char *column, int zero)
{
  fprintf(f, "\n    #[test]\n    #[should_panic(expected = \"%s on row 0\")]\n"
             "    fn rejects_mutated_%s() {\n"
             "        prove_and_verify(&bump(%s)).unwrap_or_default();\n    }\n",
          zero ? "constraints had nonzero value" : "values didn't match", name, column);
}

/* Does weight entry 0 have a nonzero coefficient in any matmul constraint?
   The first nonempty matrix contains it. Other weights and stages stay fixed. */
static int first_weight_used(const Crate *c)
{
  uint32_t k = 0;
  while (k < c->e->matrices && c->e->matrix[k].entries == 0)
    k++;
  for (uint32_t j = 1; j <= c->e->stages; j++) {
    const Stage *st = &c->e->stage[j];
    if (st->tag == TAG_MATMUL && st->k == k &&
        c->w->word[c->e->stage[st->s].at] != 0)
      return 1;
  }
  return 0;
}

/* put_lib prints lib.rs (D5, D7, D10): the constants of the entry, the AIR
   with one line for each constraint, the trace and the tests. Mutations only
   target existing entries, and an unused weight mutation must be accepted. */
static void put_lib(FILE *f, const Crate *c)
{
  const Entry *e = c->e;
  const uint32_t n = e->inputs;
  const uint64_t m = e->weights;
  const uint64_t width = e->width == 0 ? 1 : e->width;
  const int stage_test = e->stages >= 2 && e->stage[1].entries > 0;
  int first = 1;
  put_constraints(NULL, e, c->left);
  fprintf(f, "//! zkml-plonky3-%s: the Plonky3 AIR of the zkml-lang entry `%sPlonky3`.\n", c->name, c->name);
  put_lines(f, LINES(LIB_HEAD));
  if (e->width == 0)
    fputs("\n/// An empty computation uses one zero padding column so the trace has a row.\n", f);
  fprintf(f,
          "\n/// The count of the columns: %u inputs, %llu weight entries, %llu stage entries.\n"
          "pub const WIDTH: usize = %llu;\n"
          "\n/// The count of the public values: %u inputs, then %llu outputs.\n"
          "pub const PUBLIC_COUNT: usize = %llu;\n",
          n, (unsigned long long)m, (unsigned long long)(e->width - n - m),
          (unsigned long long)width, n, (unsigned long long)e->outputs,
          (unsigned long long)(n + e->outputs));
  put_lines(f, LINES(LIB_HEIGHT));
  if (m > 0)
    fprintf(f, "\n/// The column of weight entry 0.\npub const FIRST_WEIGHT: usize = %u;\n", n);
  if (stage_test)
    fprintf(f, "\n/// The column of entry 0 of stage 1.\npub const FIRST_STAGE: usize = %llu;\n",
            (unsigned long long)(n + m));
  if (e->outputs > 0)
    fprintf(f, "\n/// The column of entry 0 of the last stage (output 0).\npub const LAST_STAGE: usize = %llu;\n",
            (unsigned long long)col(e, e->stages, 0));
  fputs("\n/// The row: the inputs, the weights, then the entries of each stage.\n"
        "pub const ROW: [u32; WIDTH] = [", f);
  put_words(f, c->w, e->stage[0].at, n, &first);
  for (uint32_t k = 0; k < e->matrices; k++)
    put_words(f, c->w, e->matrix[k].at, e->matrix[k].entries, &first);
  for (uint32_t j = 1; j <= e->stages; j++)
    put_words(f, c->w, e->stage[j].at, e->stage[j].entries, &first);
  if (e->width == 0)
    fputs("0", f);
  fputs("];\n\n/// The public values: the inputs, then the outputs.\n"
        "pub const PUBLIC: [u32; PUBLIC_COUNT] = [", f);
  first = 1;
  put_words(f, c->w, e->stage[0].at, n, &first);
  put_words(f, c->w, e->stage[e->stages].at, e->outputs, &first);
  fprintf(f, "];\n\n/// The AIR of `%sPlonky3`.\n#[derive(Clone, Copy, Debug, Default)]\npub struct ", c->name);
  put_air(f, c->name);
  fputs(";\n\nimpl<F> BaseAir<F> for ", f);
  put_air(f, c->name);
  fputs(" {\n    fn width(&self) -> usize {\n        WIDTH\n    }\n}\n\n"
        "impl<AB: AirBuilderWithPublicValues> Air<AB> for ", f);
  put_air(f, c->name);
  fputs(" {\n", f);
  put_lines(f, LINES(LIB_EVAL));
  fputs("            .for_each(|([", f);
  for (uint64_t i = 0; i < width; i++)
    fprintf(f, "%sc%llu", i == 0 ? "" : ", ", (unsigned long long)i);
  fputs("], [", f);
  for (uint64_t i = 0; i < n + e->outputs; i++)
    fprintf(f, "%sp%llu", i == 0 ? "" : ", ", (unsigned long long)i);
  fputs("])| {\n", f);
  put_constraints(f, e, c->left);
  put_lines(f, LINES(LIB_CLOSE));
  fputs("\n#[cfg(test)]\nmod tests {\n    use super::{field, trace, ", f);
  put_air(f, c->name);
  fprintf(f, ", %s%s%sPUBLIC, ROW};\n", stage_test ? "FIRST_STAGE, " : "",
          m > 0 ? "FIRST_WEIGHT, " : "", e->outputs > 0 ? "LAST_STAGE, " : "");
  put_lines(f, LINES(LIB_CONFIG));
  fputs("        let proof = prove(&config, &", f);
  put_air(f, c->name);
  fputs(", trace(first), &public);\n        verify(&config, &", f);
  put_air(f, c->name);
  fputs(", &proof, &public).map_err(|e| format!(\"verify: {e:?}\"))\n", f);
  put_lines(f, LINES(LIB_TESTS));
  if (e->outputs > 0)
    put_rejection(f, "output", "LAST_STAGE", stage_zero(e, e->stages));
  if (m > 0) {
    if (first_weight_used(c))
      put_rejection(f, "weight", "FIRST_WEIGHT", 0);
    else
      put_lines(f, LINES(LIB_WEIGHT_ACCEPT));
  }
  if (stage_test)
    put_rejection(f, "stage", "FIRST_STAGE", stage_zero(e, 1));
  fputs("}\n", f);
}

/* put_file writes the file path with put. It returns 0 when a write fails. */
static int put_file(const char *path, void (*put)(FILE *, const Crate *), const Crate *c)
{
  FILE *f = fopen(path, "w");
  if (f == NULL)
    return 0;
  put(f, c);
  const int bad = ferror(f);
  return fclose(f) == 0 && !bad;
}

/* made reports if path is a directory and records whether it created it. */
static int made(const char *path, int *created)
{
  struct stat st;
  *created = mkdir(path, 0777) == 0;
  return (*created || errno == EEXIST) && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

/* write_crate writes DIR/Cargo.toml and DIR/src/lib.rs (the src/halo2.c
   form, D4 (a)). It returns 0 when a write fails. Render both files before
   replacing either destination. Keep backups until both replacements succeed,
   so a refusal preserves an existing crate. The private staging directory
   also prevents concurrent writers from sharing temporary files. */
static int write_crate(const char *dir, const Crate *c)
{
  enum { CARGO, LIB, SRC, STAGE, CARGO_NEW, LIB_NEW, CARGO_OLD, LIB_OLD, PATHS };
  const char *const suffix[PATHS] = {
    "/Cargo.toml", "/src/lib.rs", "/src", "/.plonky3print",
    "/.plonky3print/Cargo.toml", "/.plonky3print/lib.rs",
    "/.plonky3print/Cargo.toml.old", "/.plonky3print/lib.rs.old"
  };
  char *path[PATHS] = {NULL};
  int created[3] = {0};
  int existed[2] = {0};
  unsigned int mode[2] = {0};
  int saved[2] = {0};
  int installed[2] = {0};
  int ok = 0;
  const size_t len = strlen(dir);
  for (int i = 0; i < PATHS; i++) {
    path[i] = malloc(len + strlen(suffix[i]) + 1);
    if (path[i] == NULL)
      goto done;
    memcpy(path[i], dir, len);
    strcpy(path[i] + len, suffix[i]);
  }
  if (!made(dir, &created[0]) || !made(path[SRC], &created[1]))
    goto done;
  for (int i = CARGO; i <= LIB; i++) {
    struct stat st;
    const int status = stat(path[i], &st);
    if ((status == 0 && !S_ISREG(st.st_mode)) || (status != 0 && errno != ENOENT))
      goto done;
    existed[i] = status == 0;
    if (existed[i])
      mode[i] = st.st_mode & 0777;
  }
  if (mkdir(path[STAGE], 0700) != 0)
    goto done;
  created[2] = 1;
  if (!put_file(path[CARGO_NEW], put_cargo, c) || !put_file(path[LIB_NEW], put_lib, c))
    goto done;
  for (int i = 0; i < 2; i++)
    if (existed[i] && chmod(path[CARGO_NEW + i], mode[i]) != 0)
      goto done;
  for (int i = 0; i < 2; i++) {
    if (rename(path[i], path[CARGO_OLD + i]) == 0)
      saved[i] = 1;
    else if (errno != ENOENT)
      goto done;
  }
  for (int i = 0; i < 2; i++) {
    if (rename(path[CARGO_NEW + i], path[i]) != 0)
      goto done;
    installed[i] = 1;
  }
  ok = 1;
done:
  if (created[2]) {
    for (int i = 1; i >= 0; i--) {
      if (ok)
        remove(path[CARGO_OLD + i]);
      else {
        if (installed[i])
          remove(path[i]);
        if (saved[i])
          rename(path[CARGO_OLD + i], path[i]);
      }
      remove(path[CARGO_NEW + i]);
    }
    /* A failed restore leaves its backup in STAGE for recovery. */
    remove(path[STAGE]);
  }
  if (!ok) {
    if (created[1])
      remove(path[SRC]);
    if (created[0])
      remove(dir);
  }
  for (int i = 0; i < PATHS; i++)
    free(path[i]);
  return ok;
}

int main(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "usage: plonky3print NAME DIR\n");
    return 2;
  }
  Words w = {NULL, 0, 0};
  Entry e = {0};
  const char *msg = name_ok(argv[1]) ? read_words(&w) : BAD_NAME;
  if (msg == NULL)
    msg = check_entry(&e, &w);
  /* The use counts of the cells: one item for each column (D5). */
  uint32_t *left = msg == NULL ? calloc((size_t)e.width + 1, sizeof *left) : NULL;
  if (msg == NULL && left == NULL)
    msg = NO_MEMORY;
  const Crate c = {argv[1], &e, &w, left};
  const int written = msg == NULL && write_crate(argv[2], &c);
  free(left);
  free(e.stage);
  free(e.matrix);
  free(w.word);
  if (msg != NULL) {
    fprintf(stderr, "plonky3print: %s\n", msg);
    return 1;
  }
  if (!written) {
    fprintf(stderr, "plonky3print: the write to %s failed\n", argv[2]);
    return 1;
  }
  return 0;
}

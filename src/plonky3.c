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
   refusal exits 1 with one message line on stderr and writes no file. Part 1
   of slice Z10c has no writer: an accepted entry exits 0, prints nothing and
   writes no file. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
  free(e.stage);
  free(e.matrix);
  free(w.word);
  if (msg != NULL) {
    fprintf(stderr, "plonky3print: %s\n", msg);
    return 1;
  }
  return 0;
}

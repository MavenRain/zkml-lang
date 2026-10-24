/* The Halo2 printer (slice Z9c, flag 78 (a)). `build/halo2print NAME DIR`
   reads the words of one Halo2 entry on stdin: the `langc eval` output of an
   entry that `h2Words` (examples/halo2.lang) gives.

   The words (D3) are 4 blocks on one line:
   1. The circuit: the LLZK words of src/llzk.c: the version 1, the field id,
      the element size, the input count n, the weight count m, the output
      count q, the instruction count, then the instructions. Each instruction
      is a tag and its operands: 1 const k, 2 add i j, 3 sub i j, 4 mul i j,
      5 out o i, 6 eq i j, 7 refuse r.
   2. The matrix map: the matmul count c, then the matrix index of each
      matmul in run order.
   3. The inputs: n, then n elements.
   4. The Z7 witness (`frWitnessWords`): the version 1, the field id 2, the
      element size 8, the matrix count, the words of each matrix (the rank 2,
      the row count, the column count, the entries), the stage count, then
      the words of each stage (the rank, the dims, the aux word 0, the
      entries). The entries are in row-major order.
   An element is 8 limbs of 32 bits, low limb first. Halo2 is BN254 only
   (flag 79), so the field id of block 1 must be 2.

   The word reader and the block 1 checks are a copy of the src/llzk.c
   functions (flag 84 (a)), so src/llzk.c does not change.

   The printer checks all of the words first. A refusal exits 1 with one
   message line on stderr and writes no file. Part c-1 of slice Z9c has only
   the checks: an accepted entry exits 0 and writes no file. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum {
  TAG_CONST = 1,
  TAG_ADD = 2,
  TAG_SUB = 3,
  TAG_MUL = 4,
  TAG_OUT = 5,
  TAG_EQ = 6,
  TAG_REFUSE = 7
};

/* The words before the first instruction. */
enum { HEADER_WORDS = 7 };

/* The limbs of one BN254 element. */
enum { LIMBS = 8 };

/* The rows at the end of a column that MockProver does not let a circuit use
   (Z9a: 5 blinding rows and 1 more). */
enum { UNUSABLE_ROWS = 6 };

static const char *const SHORT = "the block is short";
static const char *const NOT_WRITTEN = "an output is not written";
static const char *const NOT_WORD = "a word is not a decimal number below 2^32";
static const char *const NO_MEMORY = "no memory";

/* The BN254 scalar field modulus r, low limb first. */
static const uint32_t BN254_R[LIMBS] = {4026531841u, 1138881939u, 2042196113u, 674490440u,
                                        2172737629u, 3092268470u, 3778125865u, 811880050u};

typedef struct {
  uint32_t *word;
  size_t count;
  size_t cap;
} Words;

/* Block 1. end is the first word after the block. vals is the count of value
   numbers: the inputs, the weights and one for each instruction that is not
   eq. */
typedef struct {
  const uint32_t *word;
  size_t count;
  uint32_t field;
  uint32_t size;
  uint32_t inputs;
  uint32_t weights;
  uint32_t outputs;
  uint32_t instrs;
  size_t end;
  uint64_t vals;
} Block;

/* One witness matrix: the first word of its entries and its entry count. */
typedef struct {
  size_t at;
  uint64_t entries;
} Matrix;

/* The checked entry. map is the first matrix index of block 2, input the first
   word of the input elements and last the first word of the entries of the last
   stage. rows has one row for each value number, and k is the least k with
   2^k >= rows + UNUSABLE_ROWS (D5). */
typedef struct {
  Block circuit;
  uint32_t matmuls;
  size_t map;
  size_t input;
  uint32_t matrices;
  Matrix *matrix;
  size_t last;
  uint64_t rows;
  uint32_t k;
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
  if (digits && !push(w, (uint32_t)v))
    return NO_MEMORY;
  return NULL;
}

/* operands gives the operand count of a tag, or 0 for a tag that is not known. */
static size_t operands(uint32_t tag, uint32_t size)
{
  switch (tag) {
  case TAG_CONST:
    return size;
  case TAG_ADD:
  case TAG_SUB:
  case TAG_MUL:
  case TAG_OUT:
  case TAG_EQ:
    return 2;
  case TAG_REFUSE:
    return 1;
  default:
    return 0;
  }
}

/* below reports if the limbs k are below the limbs q (both low limb first). */
static int below(const uint32_t *k, const uint32_t *q, uint32_t size)
{
  for (uint32_t i = size; i > 0; i--)
    if (k[i - 1] != q[i - 1])
      return k[i - 1] < q[i - 1];
  return 0;
}

/* operand checks the value number v. vals is the count of values so far and
   base the first result value. Only an eq (any = 1) can read an out value. */
static const char *operand(uint32_t v, uint64_t base, uint64_t vals, const unsigned char *is_out, int any)
{
  if (v >= vals)
    return "a value number is not defined yet";
  if (!any && v >= base && is_out[v - base])
    return "an out value is an operand of an op that is not eq";
  return NULL;
}

/* walk checks each instruction. is_out has one entry for each result value and
   written one entry for each output. The words after the instructions are the
   other blocks, so walk records the end of block 1 and the value count. */
static const char *walk(Block *b, unsigned char *is_out, unsigned char *written)
{
  const uint64_t base = (uint64_t)b->inputs + b->weights;
  uint64_t vals = base;
  size_t at = HEADER_WORDS;
  for (uint32_t k = 0; k < b->instrs; k++) {
    if (at >= b->count)
      return SHORT;
    const uint32_t tag = b->word[at];
    const size_t need = operands(tag, b->size);
    if (need == 0)
      return "the tag is not known";
    if (b->count - at - 1 < need)
      return SHORT;
    const uint32_t *op = b->word + at + 1;
    const char *msg = NULL;
    switch (tag) {
    case TAG_CONST:
      if (!below(op, BN254_R, b->size))
        msg = "a const is not below the field modulus";
      break;
    case TAG_ADD:
    case TAG_SUB:
    case TAG_MUL:
      msg = operand(op[0], base, vals, is_out, 0);
      if (msg == NULL)
        msg = operand(op[1], base, vals, is_out, 0);
      break;
    case TAG_OUT:
      if (op[0] >= b->outputs)
        return "an output number is not below the output count";
      if (written[op[0]])
        return "an output is written twice";
      written[op[0]] = 1;
      msg = operand(op[1], base, vals, is_out, 0);
      break;
    case TAG_EQ:
      msg = operand(op[0], base, vals, is_out, 1);
      if (msg == NULL)
        msg = operand(op[1], base, vals, is_out, 1);
      break;
    case TAG_REFUSE:
      return op[0] == 1 ? "lookup is refused (flag 69 (a))" : "the refuse code is not known";
    default:
      return "the tag is not known";
    }
    if (msg != NULL)
      return msg;
    if (tag != TAG_EQ) {
      is_out[vals - base] = tag == TAG_OUT;
      vals++;
    }
    at += 1 + need;
  }
  for (uint32_t o = 0; o < b->outputs; o++)
    if (!written[o])
      return NOT_WRITTEN;
  b->end = at;
  b->vals = vals;
  return NULL;
}

/* check reads the header into b and checks all of block 1. It returns NULL or
   a refusal. */
static const char *check(Block *b, const Words *w)
{
  if (w->count < HEADER_WORDS)
    return SHORT;
  b->word = w->word;
  b->count = w->count;
  if (w->word[0] != 1)
    return "the version is not 1";
  b->field = w->word[1];
  if (b->field != 2)
    return "the field id is not 2";
  b->size = LIMBS;
  if (w->word[2] != b->size)
    return "the element size does not agree with the field id";
  b->inputs = w->word[3];
  b->weights = w->word[4];
  b->outputs = w->word[5];
  b->instrs = w->word[6];
  /* Each output needs one out instruction. */
  if (b->outputs > b->instrs)
    return NOT_WRITTEN;
  unsigned char *is_out = calloc((size_t)b->instrs + 1, 1);
  unsigned char *written = calloc((size_t)b->outputs + 1, 1);
  const char *msg = is_out == NULL || written == NULL ? NO_MEMORY : walk(b, is_out, written);
  free(is_out);
  free(written);
  return msg;
}

/* elements checks that count elements start at the word at and that each
   element is below r. It returns NULL or a refusal. */
static const char *elements(const Words *w, size_t at, uint64_t count)
{
  if ((w->count - at) / LIMBS < count)
    return SHORT;
  for (uint64_t e = 0; e < count; e++)
    if (!below(w->word + at + e * LIMBS, BN254_R, LIMBS))
      return "an element is not below the field modulus";
  return NULL;
}

/* matrices reads the matrices of block 4 from the word *at into e->matrix. */
static const char *matrices(Entry *e, const Words *w, size_t *at)
{
  for (uint32_t i = 0; i < e->matrices; i++) {
    if (w->count - *at < 3)
      return SHORT;
    const uint32_t *head = w->word + *at;
    if (head[0] != 2)
      return "a matrix rank is not 2";
    const uint64_t entries = (uint64_t)head[1] * head[2];
    const char *msg = elements(w, *at + 3, entries);
    if (msg != NULL)
      return msg;
    e->matrix[i].at = *at + 3;
    e->matrix[i].entries = entries;
    *at += 3 + entries * LIMBS;
  }
  return NULL;
}

/* mapped checks the matrix map of block 2: each matrix index is below the
   matrix count, and the entries of the mapped matrices are the m weights. */
static const char *mapped(const Entry *e, const Words *w)
{
  uint64_t sum = 0;
  for (uint32_t i = 0; i < e->matmuls; i++) {
    const uint32_t k = w->word[e->map + i];
    if (k >= e->matrices)
      return "a matrix index is not below the matrix count";
    sum += e->matrix[k].entries;
  }
  if (sum != e->circuit.weights)
    return "the mapped matrix entries are not the weight count";
  return NULL;
}

/* stages reads the stages of block 4 from the word *at. The entry count of the
   last stage must be the output count. */
static const char *stages(Entry *e, const Words *w, size_t *at)
{
  if (*at >= w->count)
    return SHORT;
  const uint32_t count = w->word[*at];
  *at += 1;
  if (count == 0)
    return "the witness has no stage";
  uint64_t last = 0;
  for (uint32_t s = 0; s < count; s++) {
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
      if (dim != 0 && entries > (w->count / LIMBS) / dim)
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
    e->last = *at;
    last = entries;
    *at += entries * LIMBS;
  }
  if (last != e->circuit.outputs)
    return "the last stage entry count is not the output count";
  return NULL;
}

/* circuit_k gives the least k with 2^k >= rows + UNUSABLE_ROWS. A smaller k
   makes MockProver panic (Z9a), so the printer computes k. */
static uint32_t circuit_k(uint64_t rows)
{
  uint32_t k = 0;
  while (((uint64_t)1 << k) < rows + UNUSABLE_ROWS)
    k++;
  return k;
}

/* check_entry checks the 4 blocks into e. It returns NULL or a refusal. */
static const char *check_entry(Entry *e, const Words *w)
{
  const char *msg = check(&e->circuit, w);
  if (msg != NULL)
    return msg;
  size_t at = e->circuit.end;
  /* Block 2: the matrix map. */
  if (at >= w->count)
    return SHORT;
  e->matmuls = w->word[at];
  e->map = at + 1;
  if (w->count - e->map < e->matmuls)
    return SHORT;
  at = e->map + e->matmuls;
  /* Block 3: the inputs. */
  if (at >= w->count)
    return SHORT;
  if (w->word[at] != e->circuit.inputs)
    return "the input count is not the circuit input count";
  e->input = at + 1;
  msg = elements(w, e->input, e->circuit.inputs);
  if (msg != NULL)
    return msg;
  at = e->input + (size_t)e->circuit.inputs * LIMBS;
  /* Block 4: the witness header, the matrices and the stages. */
  if (w->count - at < 4)
    return SHORT;
  if (w->word[at] != 1 || w->word[at + 1] != 2 || w->word[at + 2] != LIMBS)
    return "the witness header is not 1 2 8";
  e->matrices = w->word[at + 3];
  at += 4;
  if (e->matrices > (w->count - at) / 3)
    return SHORT;
  e->matrix = calloc((size_t)e->matrices + 1, sizeof *e->matrix);
  if (e->matrix == NULL)
    return NO_MEMORY;
  msg = matrices(e, w, &at);
  if (msg == NULL)
    msg = mapped(e, w);
  if (msg == NULL)
    msg = stages(e, w, &at);
  if (msg != NULL)
    return msg;
  if (at != w->count)
    return "a word follows the witness block";
  e->rows = e->circuit.vals;
  e->k = circuit_k(e->rows);
  return NULL;
}

int main(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "usage: halo2print NAME DIR\n");
    return 2;
  }
  /* Part c-2 of slice Z9c writes the crate NAME into DIR. */
  (void)argv;
  Words w = {NULL, 0, 0};
  Entry e;
  e.matrix = NULL;
  const char *msg = read_words(&w);
  if (msg == NULL)
    msg = check_entry(&e, &w);
  free(e.matrix);
  free(w.word);
  if (msg != NULL) {
    fprintf(stderr, "halo2print: %s\n", msg);
    return 1;
  }
  return 0;
}

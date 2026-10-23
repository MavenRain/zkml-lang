/* The LLZK printer (slice Z8c, flag 68 (c)). It reads the LLZK words of one
   module on stdin and writes the module as LLZK text on stdout. The words are
   one line of decimal numbers: the `langc eval` output of an entry that
   `llzkWords` (domain/domain.lang) gives.

   The words (D10): the version 1, the field id (1 BabyBear, 2 BN254), the
   element size (1 for BabyBear, 8 for BN254), the input count n, the weight
   count m, the output count p, the instruction count, then the instructions.
   Each instruction is a tag and its operands: 1 const k (element size words,
   low limb first), 2 add i j, 3 sub i j, 4 mul i j, 5 out o i, 6 eq i j,
   7 refuse r. The values 0 .. n - 1 are the inputs and n .. n + m - 1 are the
   weights. A const, add, sub, mul or out gives the next value. The value of an
   out is the value of the member @o, and only an eq can read it.

   The module (flag 70): one struct @Main with a public member @o for each
   output. @compute writes the members. @constrain reads them and has the eq
   constraints. The inputs are public arguments and the weights are private
   arguments (O4).

   The printer checks all of the block first, then prints. A refusal exits 1
   with one message line on stderr and writes nothing on stdout. */
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

static const char *const SHORT = "the block is short";
static const char *const NOT_WRITTEN = "an output is not written";
static const char *const NOT_WORD = "a word is not a decimal number below 2^32";
static const char *const NO_MEMORY = "no memory";

/* The field moduli, low limb first. */
static const uint32_t BABYBEAR_P[1] = {2013265921u};
static const uint32_t BN254_R[8] = {4026531841u, 1138881939u, 2042196113u, 674490440u,
                                    2172737629u, 3092268470u, 3778125865u, 811880050u};

typedef struct {
  uint32_t *word;
  size_t count;
  size_t cap;
} Words;

typedef struct {
  const uint32_t *word;
  size_t count;
  uint32_t field;
  uint32_t size;
  uint32_t inputs;
  uint32_t weights;
  uint32_t outputs;
  uint32_t instrs;
} Block;

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
   written one entry for each output. */
static const char *walk(const Block *b, unsigned char *is_out, unsigned char *written)
{
  const uint64_t base = (uint64_t)b->inputs + b->weights;
  const uint32_t *modulus = b->field == 1 ? BABYBEAR_P : BN254_R;
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
      if (!below(op, modulus, b->size))
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
  if (at != b->count)
    return "the block has extra words";
  for (uint32_t o = 0; o < b->outputs; o++)
    if (!written[o])
      return NOT_WRITTEN;
  return NULL;
}

/* check reads the header into b and checks all of the block. It returns NULL
   or a refusal. */
static const char *check(Block *b, const Words *w)
{
  if (w->count < HEADER_WORDS)
    return SHORT;
  b->word = w->word;
  b->count = w->count;
  if (w->word[0] != 1)
    return "the version is not 1";
  b->field = w->word[1];
  if (b->field != 1 && b->field != 2)
    return "the field id is not 1 or 2";
  b->size = b->field == 1 ? 1 : 8;
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

static const char *type_of(const Block *b)
{
  return b->field == 1 ? "!felt.type<\"babybear\">" : "!felt.type<\"bn254\">";
}

/* print_value prints the name of the value v: %xI for an input, %wJ for a
   weight, %N for a result. */
static void print_value(const Block *b, uint64_t v)
{
  const uint64_t base = (uint64_t)b->inputs + b->weights;
  if (v < b->inputs) {
    printf("%%x%llu", (unsigned long long)v);
    return;
  }
  if (v < base) {
    printf("%%w%llu", (unsigned long long)(v - b->inputs));
    return;
  }
  printf("%%%llu", (unsigned long long)(v - base));
}

/* print_decimal prints the limbs (low limb first) as a decimal number (flag 67
   (a)). Each step divides the number by 10^9 and keeps the remainder. */
static void print_decimal(const uint32_t *limbs, uint32_t size)
{
  uint32_t q[8] = {0};
  uint32_t chunks[10]; /* 2^256 < 10^81: at most 9 chunks of 9 digits */
  size_t count = 0;
  uint32_t top = size;
  for (uint32_t i = 0; i < size; i++)
    q[i] = limbs[i];
  while (top > 0 && q[top - 1] == 0)
    top--;
  do {
    uint64_t rem = 0;
    for (uint32_t i = top; i > 0; i--) {
      const uint64_t cur = rem << 32 | q[i - 1];
      q[i - 1] = (uint32_t)(cur / 1000000000u);
      rem = cur % 1000000000u;
    }
    chunks[count++] = (uint32_t)rem;
    while (top > 0 && q[top - 1] == 0)
      top--;
  } while (top > 0);
  printf("%u", (unsigned)chunks[count - 1]);
  for (size_t i = count - 1; i > 0; i--)
    printf("%09u", (unsigned)chunks[i - 1]);
}

/* print_arith prints `%N = felt.OP %a, %b : T, T`. */
static void print_arith(const Block *b, uint64_t n, const char *name, uint32_t a, uint32_t c)
{
  const char *t = type_of(b);
  printf("      ");
  print_value(b, n);
  printf(" = felt.%s ", name);
  print_value(b, a);
  printf(", ");
  print_value(b, c);
  printf(" : %s, %s\n", t, t);
}

/* print_eq prints `constrain.eq %a, %b : T, T`. */
static void print_eq(const Block *b, uint32_t a, uint32_t c)
{
  const char *t = type_of(b);
  printf("      constrain.eq ");
  print_value(b, a);
  printf(", ");
  print_value(b, c);
  printf(" : %s, %s\n", t, t);
}

static const char *felt_name(uint32_t tag)
{
  return tag == TAG_ADD ? "add" : tag == TAG_SUB ? "sub" : "mul";
}

/* print_body prints the instructions of @compute (constrain 0) or of
   @constrain (constrain 1). In @compute an out writes the member and an eq
   prints nothing. In @constrain an out reads the member. */
static void print_body(const Block *b, int constrain)
{
  const char *t = type_of(b);
  uint64_t vals = (uint64_t)b->inputs + b->weights;
  size_t at = HEADER_WORDS;
  for (uint32_t k = 0; k < b->instrs; k++) {
    const uint32_t tag = b->word[at];
    const uint32_t *op = b->word + at + 1;
    switch (tag) {
    case TAG_CONST:
      printf("      ");
      print_value(b, vals);
      printf(" = felt.const ");
      print_decimal(op, b->size);
      printf(" : %s\n", t);
      vals++;
      break;
    case TAG_ADD:
    case TAG_SUB:
    case TAG_MUL:
      print_arith(b, vals, felt_name(tag), op[0], op[1]);
      vals++;
      break;
    case TAG_OUT:
      if (constrain) {
        printf("      ");
        print_value(b, vals);
        printf(" = struct.readm %%self[@o%u] : !struct.type<@Main>, %s\n", (unsigned)op[0], t);
      } else {
        printf("      struct.writem %%self[@o%u] = ", (unsigned)op[0]);
        print_value(b, op[1]);
        printf(" : !struct.type<@Main>, %s\n", t);
      }
      vals++;
      break;
    case TAG_EQ:
      if (constrain)
        print_eq(b, op[0], op[1]);
      break;
    default:
      break;
    }
    at += 1 + operands(tag, b->size);
  }
}

/* print_args prints the inputs (public) and the weights (private). first is 1
   when no argument is before them. */
static void print_args(const Block *b, int first)
{
  const char *t = type_of(b);
  for (uint32_t i = 0; i < b->inputs; i++)
    printf("%s%%x%u: %s {llzk.pub}", first && i == 0 ? "" : ", ", (unsigned)i, t);
  for (uint32_t j = 0; j < b->weights; j++)
    printf("%s%%w%u: %s", first && b->inputs == 0 && j == 0 ? "" : ", ", (unsigned)j, t);
}

static void print_module(const Block *b)
{
  printf("module attributes {llzk.lang = \"zkml-lang\", llzk.main = !struct.type<@Main>} {\n");
  printf("  struct.def @Main {\n");
  for (uint32_t o = 0; o < b->outputs; o++)
    printf("    struct.member @o%u : %s {llzk.pub}\n", (unsigned)o, type_of(b));
  printf("    function.def @compute(");
  print_args(b, 1);
  printf(") -> !struct.type<@Main> {\n");
  printf("      %%self = struct.new : !struct.type<@Main>\n");
  print_body(b, 0);
  printf("      function.return %%self : !struct.type<@Main>\n");
  printf("    }\n");
  printf("    function.def @constrain(%%self: !struct.type<@Main>");
  print_args(b, 0);
  printf(") {\n");
  print_body(b, 1);
  printf("      function.return\n");
  printf("    }\n");
  printf("  }\n");
  printf("}\n");
}

int main(void)
{
  Words w = {NULL, 0, 0};
  Block b;
  const char *msg = read_words(&w);
  if (msg == NULL)
    msg = check(&b, &w);
  if (msg != NULL) {
    fprintf(stderr, "llzkprint: %s\n", msg);
    free(w.word);
    return 1;
  }
  print_module(&b);
  free(w.word);
  if (fflush(stdout) != 0) {
    fprintf(stderr, "llzkprint: the write to stdout failed\n");
    return 1;
  }
  return 0;
}

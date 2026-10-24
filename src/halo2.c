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

   The printer checks NAME (a to z and 0 to 9) and all of the words first. A
   refusal exits 1 with one message line on stderr and writes no file. Then it
   writes the crate `zkml-halo2-NAME`: DIR/Cargo.toml and DIR/src/lib.rs (the
   Z9a sample form: the tables and the constants of the entry, then fixed
   text). A write that fails is a refusal. An accepted entry exits 0 and prints
   nothing. */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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
static const char *const BAD_NAME = "the name is empty or has a character that is not a to z or 0 to 9";

/* The u64 limbs of one element in the printed Rust: 2 limbs of 32 bits each. */
enum { U64_LIMBS = 4 };

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

/* The fixed text of lib.rs (the Z9a sample form), one string for each line. */

/* LIB_HEAD: the doc comment after its first line, and the use block. */
static const char *const LIB_HEAD[] = {
  "//!",
  "//! build/halo2print prints this crate from the words of the entry (the",
  "//! `langc eval` output), so do not edit it: print it again.",
  "//!",
  "//! The layout: the advice columns a, b and c, one instance column, one fixed",
  "//! column kc, and the selectors s_add, s_sub, s_mul and s_const. One region has",
  "//! one row for each value number. The instance column holds INPUTS, then",
  "//! OUTPUTS.",
  "",
  "use halo2_proofs::{",
  "    circuit::{AssignedCell, Layouter, Region, SimpleFloorPlanner, Value},",
  "    halo2curves::bn256::Fr,",
  "    plonk::{",
  "        Advice, Circuit, Column, ConstraintSystem, Error, Expression, Fixed, Instance, Selector,",
  "    },",
  "    poly::Rotation,",
  "};",
};

/* LIB_ROW: the Row enum. */
static const char *const LIB_ROW[] = {
  "/// One row of the region. The row offset is the value number.",
  "#[derive(Clone, Copy, Debug)]",
  "pub enum Row {",
  "    /// c is the instance row i.",
  "    Input(usize),",
  "    /// c is the weight j (private, no gate).",
  "    Weight(usize),",
  "    /// c is the constant k (the gate s_const).",
  "    Const([u64; 4]),",
  "    /// c = value i + value j (the gate s_add).",
  "    Add(usize, usize),",
  "    /// c = value i - value j (the gate s_sub).",
  "    Sub(usize, usize),",
  "    /// c = value i * value j (the gate s_mul).",
  "    Mul(usize, usize),",
  "    /// c is the output o. The instance row INPUT_COUNT + o holds it.",
  "    Out(usize),",
  "}",
};

/* LIB_BODY: the config, the circuit, the helpers and the tests up to
   rejects_mutated_output. */
static const char *const LIB_BODY[] = {
  "/// The columns and the selectors of the D5 layout.",
  "#[derive(Clone, Debug)]",
  "pub struct ExportConfig {",
  "    a: Column<Advice>,",
  "    b: Column<Advice>,",
  "    c: Column<Advice>,",
  "    instance: Column<Instance>,",
  "    kc: Column<Fixed>,",
  "    s_add: Selector,",
  "    s_sub: Selector,",
  "    s_mul: Selector,",
  "    s_const: Selector,",
  "}",
  "",
  "/// The circuit: the private witness values (the weights and the outputs).",
  "#[derive(Clone, Debug)]",
  "pub struct ExportCircuit {",
  "    weights: Vec<Value<Fr>>,",
  "    outputs: Vec<Value<Fr>>,",
  "}",
  "",
  "type Cell = AssignedCell<Fr, Fr>;",
  "",
  "impl ExportCircuit {",
  "    /// The circuit with known weights and outputs.",
  "    pub fn new(weights: &[Fr], outputs: &[Fr]) -> Self {",
  "        Self {",
  "            weights: known(weights),",
  "            outputs: known(outputs),",
  "        }",
  "    }",
  "",
  "    fn assign_row(",
  "        &self,",
  "        config: &ExportConfig,",
  "        region: &mut Region<'_, Fr>,",
  "        cells: &[Cell],",
  "        row: Row,",
  "        offset: usize,",
  "    ) -> Result<Cell, Error> {",
  "        match row {",
  "            Row::Input(i) => region.assign_advice_from_instance(",
  "                || \"input\",",
  "                config.instance,",
  "                i,",
  "                config.c,",
  "                offset,",
  "            ),",
  "            Row::Weight(j) => witness(&self.weights, j)",
  "                .and_then(|w| region.assign_advice(|| \"weight\", config.c, offset, || w)),",
  "            Row::Const(k) => config",
  "                .s_const",
  "                .enable(region, offset)",
  "                .and_then(|()| {",
  "                    region.assign_fixed(|| \"kc\", config.kc, offset, || {",
  "                        Value::known(Fr::from_raw(k))",
  "                    })",
  "                })",
  "                .and_then(|_| {",
  "                    region.assign_advice(|| \"const\", config.c, offset, || {",
  "                        Value::known(Fr::from_raw(k))",
  "                    })",
  "                }),",
  "            Row::Add(i, j) => binary_row(",
  "                config,",
  "                region,",
  "                cells,",
  "                (config.s_add, |x: Fr, y: Fr| x + y),",
  "                (i, j),",
  "                offset,",
  "            ),",
  "            Row::Sub(i, j) => binary_row(",
  "                config,",
  "                region,",
  "                cells,",
  "                (config.s_sub, |x: Fr, y: Fr| x - y),",
  "                (i, j),",
  "                offset,",
  "            ),",
  "            Row::Mul(i, j) => binary_row(",
  "                config,",
  "                region,",
  "                cells,",
  "                (config.s_mul, |x: Fr, y: Fr| x * y),",
  "                (i, j),",
  "                offset,",
  "            ),",
  "            Row::Out(o) => witness(&self.outputs, o)",
  "                .and_then(|v| region.assign_advice(|| \"out\", config.c, offset, || v)),",
  "        }",
  "    }",
  "}",
  "",
  "impl Circuit<Fr> for ExportCircuit {",
  "    type Config = ExportConfig;",
  "    type FloorPlanner = SimpleFloorPlanner;",
  "",
  "    fn without_witnesses(&self) -> Self {",
  "        Self {",
  "            weights: unknown(&self.weights),",
  "            outputs: unknown(&self.outputs),",
  "        }",
  "    }",
  "",
  "    fn configure(meta: &mut ConstraintSystem<Fr>) -> ExportConfig {",
  "        let config = ExportConfig {",
  "            a: meta.advice_column(),",
  "            b: meta.advice_column(),",
  "            c: meta.advice_column(),",
  "            instance: meta.instance_column(),",
  "            kc: meta.fixed_column(),",
  "            s_add: meta.selector(),",
  "            s_sub: meta.selector(),",
  "            s_mul: meta.selector(),",
  "            s_const: meta.selector(),",
  "        };",
  "        meta.enable_equality(config.a);",
  "        meta.enable_equality(config.b);",
  "        meta.enable_equality(config.c);",
  "        meta.enable_equality(config.instance);",
  "        binary_gate(meta, &config, (\"add\", config.s_add), |a, b, c| a + b - c);",
  "        binary_gate(meta, &config, (\"sub\", config.s_sub), |a, b, c| a - b - c);",
  "        binary_gate(meta, &config, (\"mul\", config.s_mul), |a, b, c| a * b - c);",
  "        meta.create_gate(\"const\", |q| {",
  "            let s = q.query_selector(config.s_const);",
  "            let c = q.query_advice(config.c, Rotation::cur());",
  "            let kc = q.query_fixed(config.kc, Rotation::cur());",
  "            [s * (c - kc)]",
  "        });",
  "        config",
  "    }",
  "",
  "    fn synthesize(&self, config: ExportConfig, mut layouter: impl Layouter<Fr>) -> Result<(), Error> {",
  "        let cells = layouter.assign_region(",
  "            || \"export\",",
  "            |mut region| {",
  "                PROGRAM",
  "                    .iter()",
  "                    .zip(0..)",
  "                    .try_fold(Vec::new(), |cells, (row, offset)| {",
  "                        self.assign_row(&config, &mut region, &cells, *row, offset)",
  "                            .map(|cell| cells.into_iter().chain(Some(cell)).collect())",
  "                    })",
  "                    .and_then(|cells| {",
  "                        EQS.iter()",
  "                            .try_for_each(|(i, j)| {",
  "                                cell_at(&cells, *i).and_then(|left| {",
  "                                    cell_at(&cells, *j).and_then(|right| {",
  "                                        region.constrain_equal(left.cell(), right.cell())",
  "                                    })",
  "                                })",
  "                            })",
  "                            .map(|()| cells)",
  "                    })",
  "            },",
  "        )?;",
  "        PROGRAM",
  "            .iter()",
  "            .zip(cells.iter())",
  "            .try_for_each(|(row, cell)| match row {",
  "                Row::Out(o) => {",
  "                    layouter.constrain_instance(cell.cell(), config.instance, INPUT_COUNT + o)",
  "                }",
  "                Row::Input(_)",
  "                | Row::Weight(_)",
  "                | Row::Const(_)",
  "                | Row::Add(..)",
  "                | Row::Sub(..)",
  "                | Row::Mul(..) => Ok(()),",
  "            })",
  "    }",
  "}",
  "",
  "type Poly = fn(Expression<Fr>, Expression<Fr>, Expression<Fr>) -> Expression<Fr>;",
  "",
  "fn binary_gate(",
  "    meta: &mut ConstraintSystem<Fr>,",
  "    config: &ExportConfig,",
  "    gate: (&'static str, Selector),",
  "    poly: Poly,",
  ") {",
  "    let (name, selector) = gate;",
  "    meta.create_gate(name, |q| {",
  "        let s = q.query_selector(selector);",
  "        let a = q.query_advice(config.a, Rotation::cur());",
  "        let b = q.query_advice(config.b, Rotation::cur());",
  "        let c = q.query_advice(config.c, Rotation::cur());",
  "        [s * poly(a, b, c)]",
  "    });",
  "}",
  "",
  "fn binary_row(",
  "    config: &ExportConfig,",
  "    region: &mut Region<'_, Fr>,",
  "    cells: &[Cell],",
  "    gate: (Selector, fn(Fr, Fr) -> Fr),",
  "    operands: (usize, usize),",
  "    offset: usize,",
  ") -> Result<Cell, Error> {",
  "    let (selector, op) = gate;",
  "    let (i, j) = operands;",
  "    let left = cell_at(cells, i)?;",
  "    let right = cell_at(cells, j)?;",
  "    selector.enable(region, offset)?;",
  "    let a = left.copy_advice(|| \"a\", region, config.a, offset)?;",
  "    let b = right.copy_advice(|| \"b\", region, config.b, offset)?;",
  "    let value = a",
  "        .value()",
  "        .copied()",
  "        .zip(b.value().copied())",
  "        .map(|(x, y)| op(x, y));",
  "    region.assign_advice(|| \"c\", config.c, offset, || value)",
  "}",
  "",
  "fn cell_at(cells: &[Cell], n: usize) -> Result<&Cell, Error> {",
  "    cells.get(n).ok_or(Error::Synthesis)",
  "}",
  "",
  "fn witness(values: &[Value<Fr>], n: usize) -> Result<Value<Fr>, Error> {",
  "    values.get(n).copied().ok_or(Error::Synthesis)",
  "}",
  "",
  "fn known(values: &[Fr]) -> Vec<Value<Fr>> {",
  "    values.iter().copied().map(Value::known).collect()",
  "}",
  "",
  "fn unknown(values: &[Value<Fr>]) -> Vec<Value<Fr>> {",
  "    values.iter().map(|_| Value::unknown()).collect()",
  "}",
  "",
  "/// The field elements of limb rows (canonical by the D3 check).",
  "pub fn field(limbs: &[[u64; 4]]) -> Vec<Fr> {",
  "    limbs.iter().copied().map(Fr::from_raw).collect()",
  "}",
  "",
  "/// The instance column: the inputs, then the outputs.",
  "pub fn instance(inputs: &[Fr], outputs: &[Fr]) -> Vec<Fr> {",
  "    inputs.iter().chain(outputs).copied().collect()",
  "}",
  "",
  "#[cfg(test)]",
  "mod tests {",
  "    use super::{field, instance, ExportCircuit, INPUTS, K, OUTPUTS, WEIGHTS};",
  "    use halo2_proofs::{arithmetic::Field, dev::MockProver, halo2curves::bn256::Fr};",
  "",
  "    fn verify(weights: &[Fr], outputs: &[Fr]) -> Result<(), String> {",
  "        MockProver::run(",
  "            K,",
  "            &ExportCircuit::new(weights, outputs),",
  "            vec![instance(&field(&INPUTS), outputs)],",
  "        )",
  "        .map_err(|e| format!(\"run: {e:?}\"))",
  "        .and_then(|prover| {",
  "            prover",
  "                .verify()",
  "                .map_err(|failures| format!(\"verify: {failures:?}\"))",
  "        })",
  "    }",
  "",
  "    fn bump_first(values: &[Fr]) -> Vec<Fr> {",
  "        values",
  "            .first()",
  "            .map(|v| *v + Fr::ONE)",
  "            .into_iter()",
  "            .chain(values.iter().skip(1).copied())",
  "            .collect()",
  "    }",
  "",
  "    fn rejects(verdict: Result<(), String>) -> Result<(), String> {",
  "        verdict.map_or_else(",
  "            |_| Ok(()),",
  "            |()| Err(\"verify accepted a mutated witness\".to_string()),",
  "        )",
  "    }",
  "",
  "    #[test]",
  "    fn accepts_z7_witness() -> Result<(), String> {",
  "        verify(&field(&WEIGHTS), &field(&OUTPUTS))",
  "    }",
  "",
  "    #[test]",
  "    fn rejects_mutated_output() -> Result<(), String> {",
  "        rejects(verify(&field(&WEIGHTS), &bump_first(&field(&OUTPUTS))))",
  "    }",
};

/* LIB_WEIGHT_TEST: the test rejects_mutated_weight (only when m > 0). */
static const char *const LIB_WEIGHT_TEST[] = {
  "",
  "    #[test]",
  "    fn rejects_mutated_weight() -> Result<(), String> {",
  "        rejects(verify(&bump_first(&field(&WEIGHTS)), &field(&OUTPUTS)))",
  "    }",
};

/* LINES gives the lines of a fixed text and their count. */
#define LINES(text) (text), sizeof(text) / sizeof *(text)

/* put_lines prints count lines. */
static void put_lines(FILE *f, const char *const *line, size_t count)
{
  for (size_t i = 0; i < count; i++)
    fprintf(f, "%s\n", line[i]);
}

/* put_element prints the element at word as U64_LIMBS u64 limbs: 2 limbs of
   32 bits into one u64, low limb first (D5). */
static void put_element(FILE *f, const uint32_t *word)
{
  for (int i = 0; i < U64_LIMBS; i++)
    fprintf(f, "%s%llu", i == 0 ? "[" : ", ",
            (unsigned long long)word[2 * i] | (unsigned long long)word[2 * i + 1] << 32);
  fputs("]", f);
}

/* put_elements prints count elements from the word at, one line each. */
static void put_elements(FILE *f, const Words *w, size_t at, uint64_t count)
{
  for (uint64_t i = 0; i < count; i++) {
    fputs("    ", f);
    put_element(f, w->word + at + i * LIMBS);
    fputs(",\n", f);
  }
}

/* open_table prints the doc line and the head of a table of count items. The
   items follow, then "];". */
static void open_table(FILE *f, const char *doc, const char *head, uint64_t count)
{
  fprintf(f, "\n/// %s\n%s%llu] = [%s", doc, head, (unsigned long long)count, count == 0 ? "" : "\n");
}

/* put_instrs prints a line for each instruction of block 1: the eq
   instructions (eqs = 1, the EQS pairs) or the others (eqs = 0, the PROGRAM
   rows after the input and the weight rows). */
static void put_instrs(FILE *f, const Block *b, int eqs)
{
  size_t at = HEADER_WORDS;
  for (uint32_t k = 0; k < b->instrs; k++) {
    const uint32_t tag = b->word[at];
    const uint32_t *op = b->word + at + 1;
    if (eqs && tag == TAG_EQ)
      fprintf(f, "    (%u, %u),\n", op[0], op[1]);
    if (!eqs && tag == TAG_CONST) {
      fputs("    Row::Const(", f);
      put_element(f, op);
      fputs("),\n", f);
    }
    if (!eqs && tag == TAG_ADD)
      fprintf(f, "    Row::Add(%u, %u),\n", op[0], op[1]);
    if (!eqs && tag == TAG_SUB)
      fprintf(f, "    Row::Sub(%u, %u),\n", op[0], op[1]);
    if (!eqs && tag == TAG_MUL)
      fprintf(f, "    Row::Mul(%u, %u),\n", op[0], op[1]);
    if (!eqs && tag == TAG_OUT)
      fprintf(f, "    Row::Out(%u),\n", op[0]);
    at += 1 + operands(tag, b->size);
  }
}

/* The crate to write: the name and the checked entry. */
typedef struct {
  const char *name;
  const Entry *e;
  const Words *w;
} Crate;

/* put_cargo prints Cargo.toml (the Z9a sample form, flag 79 (a)). The crate is
   a member of the workspace of make halo2-check, so it has no [workspace]. */
static void put_cargo(FILE *f, const Crate *c)
{
  fprintf(f,
          "[package]\n"
          "name = \"zkml-halo2-%s\"\n"
          "version = \"0.1.0\"\n"
          "edition = \"2021\"\n"
          "license = \"MIT OR Apache-2.0\"\n"
          "description = \"The Halo2 circuit of the zkml-lang entry %sHalo2 (printed by halo2print)\"\n"
          "publish = false\n"
          "\n"
          "[dependencies]\n"
          "# Flag 79 (a): PSE halo2_proofs v0.3.0 (the tag is the commit\n"
          "# 73408a140737d8336490452193b21f5a7a94e7de).\n"
          "halo2_proofs = { git = \"https://github.com/privacy-scaling-explorations/halo2\", tag = \"v0.3.0\" }\n",
          c->name, c->name);
}

/* put_lib prints lib.rs: the constants and the tables of the entry (D5, D6),
   then the fixed text. A crate with no weight has no rejects_mutated_weight
   test (D7). */
static void put_lib(FILE *f, const Crate *c)
{
  const Entry *e = c->e;
  const Block *b = &e->circuit;
  fprintf(f, "//! zkml-halo2-%s: the Halo2 circuit of the zkml-lang entry `%sHalo2`.\n", c->name, c->name);
  put_lines(f, LINES(LIB_HEAD));
  fprintf(f,
          "\n/// The circuit size is 2^K rows: the least k with 2^k at or above ROWS plus\n"
          "/// the 6 rows that MockProver keeps (5 blinding rows and 1 more row).\n"
          "pub const K: u32 = %u;\n"
          "\n/// The count of the value rows.\n"
          "pub const ROWS: usize = %llu;\n"
          "\n/// The count of the inputs. The instance rows 0 .. INPUT_COUNT hold the inputs.\n"
          "pub const INPUT_COUNT: usize = %u;\n",
          e->k, (unsigned long long)e->rows, b->inputs);
  open_table(f, "The inputs: 4 limbs of 64 bits each, low limb first.", "pub const INPUTS: [[u64; 4]; ",
             b->inputs);
  put_elements(f, c->w, e->input, b->inputs);
  fputs("];\n", f);
  /* Weight n + j is entry j of the mapped matrices in map order, each in
     row-major order (the llzkWords numbering). */
  open_table(f, "The weights, in weight-number order.", "pub const WEIGHTS: [[u64; 4]; ", b->weights);
  for (uint32_t i = 0; i < e->matmuls; i++) {
    const Matrix *m = &e->matrix[c->w->word[e->map + i]];
    put_elements(f, c->w, m->at, m->entries);
  }
  fputs("];\n", f);
  open_table(f, "The outputs: the last trace stage.", "pub const OUTPUTS: [[u64; 4]; ", b->outputs);
  put_elements(f, c->w, e->last, b->outputs);
  fputs("];\n\n", f);
  put_lines(f, LINES(LIB_ROW));
  fprintf(f, "\n/// The rows of `%sHalo2`, in value-number order.\npub const PROGRAM: [Row; ROWS] = [%s", c->name,
          e->rows == 0 ? "" : "\n");
  for (uint32_t i = 0; i < b->inputs; i++)
    fprintf(f, "    Row::Input(%u),\n", i);
  for (uint32_t j = 0; j < b->weights; j++)
    fprintf(f, "    Row::Weight(%u),\n", j);
  put_instrs(f, b, 0);
  fputs("];\n", f);
  open_table(f, "The eq instructions: each is a copy constraint on the c cells of 2 rows.",
             "pub const EQS: [(usize, usize); ", b->instrs - (b->vals - b->inputs - b->weights));
  put_instrs(f, b, 1);
  fputs("];\n\n", f);
  put_lines(f, LINES(LIB_BODY));
  if (b->weights > 0)
    put_lines(f, LINES(LIB_WEIGHT_TEST));
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

/* write_crate writes DIR/Cargo.toml and DIR/src/lib.rs. It returns 0 when a
   write fails. Render both files before replacing either destination. Keep
   backups until both replacements succeed, so a refusal preserves an existing
   crate. The private staging directory also prevents concurrent writers from
   sharing temporary files. */
static int write_crate(const char *dir, const Crate *c)
{
  enum { CARGO, LIB, SRC, STAGE, CARGO_NEW, LIB_NEW, CARGO_OLD, LIB_OLD, PATHS };
  const char *const suffix[PATHS] = {
    "/Cargo.toml", "/src/lib.rs", "/src", "/.halo2print",
    "/.halo2print/Cargo.toml", "/.halo2print/lib.rs",
    "/.halo2print/Cargo.toml.old", "/.halo2print/lib.rs.old"
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
    fprintf(stderr, "usage: halo2print NAME DIR\n");
    return 2;
  }
  Words w = {NULL, 0, 0};
  Entry e;
  e.matrix = NULL;
  const char *msg = name_ok(argv[1]) ? read_words(&w) : BAD_NAME;
  if (msg == NULL)
    msg = check_entry(&e, &w);
  const Crate c = {argv[1], &e, &w};
  const int written = msg == NULL && write_crate(argv[2], &c);
  free(e.matrix);
  free(w.word);
  if (msg != NULL) {
    fprintf(stderr, "halo2print: %s\n", msg);
    return 1;
  }
  if (!written) {
    fprintf(stderr, "halo2print: the write to %s failed\n", argv[2]);
    return 1;
  }
  return 0;
}

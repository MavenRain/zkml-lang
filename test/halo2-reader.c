/* Regression checks for stage shapes in the Halo2 word reader. */
#include <string.h>
#define main halo2print_main
#include "../src/halo2.c"
#undef main

static int check_shape(const char *name, const uint32_t *dims, uint32_t rank, const char *expected)
{
  /* An empty circuit, matrix map and input block, followed by one stage. */
  uint32_t word[32] = {1, 2, 8, 0, 0, 0, 0, 0, 0, 1, 2, 8, 0, 1};
  size_t count = 14;
  word[count++] = rank;
  for (uint32_t d = 0; d < rank; d++)
    word[count++] = dims[d];
  word[count++] = 0;
  Words w = {word, count, count};
  Entry e = {0};
  const char *msg = check_entry(&e, &w);
  free(e.matrix);
  if ((msg == NULL && expected == NULL) ||
      (msg != NULL && expected != NULL && strcmp(msg, expected) == 0))
    return 0;
  fprintf(stderr, "FAIL halo2 shape %s: want %s, got %s\n", name,
          expected == NULL ? "accept" : expected, msg == NULL ? "accept" : msg);
  return 1;
}

int main(void)
{
  const uint32_t trailing_zero[] = {10, 0};
  const uint32_t leading_zero[] = {0, 10};
  const uint32_t overflowing_prefix[] = {UINT32_MAX, UINT32_MAX, UINT32_MAX, 0};
  const uint32_t oversized[] = {10};
  const uint32_t overflowing[] = {UINT32_MAX, UINT32_MAX, UINT32_MAX};
  int failed = 0;
  failed += check_shape("trailing zero", trailing_zero, 2, NULL);
  failed += check_shape("leading zero", leading_zero, 2, NULL);
  failed += check_shape("overflowing prefix before zero", overflowing_prefix, 4, NULL);
  failed += check_shape("oversized nonempty", oversized, 1, SHORT);
  failed += check_shape("overflowing nonempty", overflowing, 3, SHORT);
  if (failed != 0)
    return 1;
  puts("halo2 reader: 5 checked");
  return 0;
}

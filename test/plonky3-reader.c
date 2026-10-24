/* Regression checks for the Plonky3 word reader. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define main plonky3print_main
#include "../src/plonky3.c"
#undef main

static int check_shape(const char *name, const uint32_t *dims, uint32_t rank, const char *expected)
{
  /* One zero stage, an empty input block and no matrix, followed by the
     witness stage. */
  uint32_t word[32] = {1, 2, 0, 1, 1, 1, 0, 1};
  size_t count = 8;
  word[count++] = rank;
  for (uint32_t d = 0; d < rank; d++)
    word[count++] = dims[d];
  word[count++] = 0;
  Words w = {word, count, count};
  Entry e = {0};
  const char *msg = check_entry(&e, &w);
  free(e.stage);
  free(e.matrix);
  if ((msg == NULL && expected == NULL) ||
      (msg != NULL && expected != NULL && strcmp(msg, expected) == 0))
    return 0;
  fprintf(stderr, "FAIL plonky3 shape %s: want %s, got %s\n", name,
          expected == NULL ? "accept" : expected, msg == NULL ? "accept" : msg);
  return 1;
}

static int check_read_error(void)
{
  const char text[] = "0 2 3 5 1 1 1 0 0\n";
  int descriptors[2];
  const int saved = dup(STDIN_FILENO);
  if (saved < 0 || pipe(descriptors) != 0) {
    if (saved >= 0)
      close(saved);
    fputs("FAIL plonky3 read error: cannot prepare input\n", stderr);
    return 1;
  }
  const ssize_t count = write(descriptors[1], text, sizeof text - 1);
  close(descriptors[1]);
  if (count != (ssize_t)(sizeof text - 1) || dup2(descriptors[0], STDIN_FILENO) < 0) {
    close(descriptors[0]);
    close(saved);
    fputs("FAIL plonky3 read error: cannot redirect input\n", stderr);
    return 1;
  }
  close(descriptors[0]);
  clearerr(stdin);
  /* Keep a complete entry in the stdio buffer, then make the next read fail. */
  const int first = getchar();
  const int buffered = first != EOF && ungetc(first, stdin) != EOF;
  close(STDIN_FILENO);
  Words w = {NULL, 0, 0};
  const char *msg = read_words(&w);
  const int read_failed = ferror(stdin);
  const int restored = dup2(saved, STDIN_FILENO) >= 0;
  close(saved);
  clearerr(stdin);
  free(w.word);
  if (buffered && read_failed && restored && msg != NULL &&
      strcmp(msg, "the input is not read") == 0)
    return 0;
  fprintf(stderr, "FAIL plonky3 read error: want refusal, got %s\n",
          msg == NULL ? "accept" : msg);
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
  failed += check_read_error();
  if (failed != 0)
    return 1;
  puts("plonky3 reader: 6 checked");
  return 0;
}

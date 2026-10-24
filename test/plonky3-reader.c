/* Regression checks for the Plonky3 word reader and crate writer. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static int writer_rename(const char *from, const char *to);
#define rename writer_rename
#define main plonky3print_main
#include "../src/plonky3.c"
#undef main
#undef rename

static int rename_count;
static int fail_rename;

static int writer_rename(const char *from, const char *to)
{
  if (++rename_count == fail_rename) {
    errno = EIO;
    return -1;
  }
  return rename(from, to);
}

static int text_file(const char *path, const char *text)
{
  FILE *f = fopen(path, "w");
  if (f == NULL)
    return 0;
  const int ok = fputs(text, f) >= 0;
  return fclose(f) == 0 && ok;
}

static int has_text(const char *path, const char *text)
{
  char buffer[64];
  FILE *f = fopen(path, "r");
  if (f == NULL)
    return 0;
  const size_t count = fread(buffer, 1, sizeof buffer, f);
  const int ok = count == strlen(text) && memcmp(buffer, text, count) == 0 && !ferror(f);
  return fclose(f) == 0 && ok;
}

/* A refused write must preserve both existing files, even if installation of
   the second file fails after the first has been installed. */
static int check_write_refusal(int blocker)
{
  static unsigned int directory_number;
  const char *base = getenv("TMPDIR");
  char dir[512];
  const int length = snprintf(dir, sizeof dir, "%s/zkml-plonky3-write-%lu-%u",
                              base != NULL && base[0] != '\0' ? base : "/tmp",
                              (unsigned long)getpid(), directory_number++);
  if (length < 0 || (size_t)length >= sizeof dir || mkdir(dir, 0700) != 0) {
    fprintf(stderr, "FAIL plonky3 writer refusal %d: cannot make the directory %s\n", blocker, dir);
    return 1;
  }
  char cargo[sizeof dir + sizeof "/Cargo.toml"];
  char src[sizeof dir + sizeof "/src"];
  char lib[sizeof dir + sizeof "/src/lib.rs"];
  char stage[sizeof dir + sizeof "/.plonky3print"];
  snprintf(cargo, sizeof cargo, "%s/Cargo.toml", dir);
  snprintf(src, sizeof src, "%s/src", dir);
  snprintf(lib, sizeof lib, "%s/src/lib.rs", dir);
  snprintf(stage, sizeof stage, "%s/.plonky3print", dir);
  /* No stage, an empty input block and an empty witness. */
  uint32_t word[] = {0, 0, 1, 1, 1, 0, 0};
  Words w = {word, sizeof word / sizeof *word, sizeof word / sizeof *word};
  Entry e = {0};
  uint32_t left[1] = {0};
  const Crate c = {"empty", &e, &w, left};
  int ok = check_entry(&e, &w) == NULL && text_file(cargo, "keep manifest\n");
  if (blocker == 0)
    ok = ok && text_file(src, "keep src\n");
  else
    ok = ok && mkdir(src, 0700) == 0 &&
         (blocker == 1 ? mkdir(lib, 0700) == 0 : text_file(lib, "keep source\n"));
  rename_count = 0;
  fail_rename = blocker >= 2 ? blocker - 1 : 0;
  if (ok)
    ok = !write_crate(dir, &c) && has_text(cargo, "keep manifest\n");
  fail_rename = 0;
  struct stat st;
  if (blocker == 0)
    ok = ok && has_text(src, "keep src\n");
  else if (blocker == 1)
    ok = ok && stat(lib, &st) == 0 && S_ISDIR(st.st_mode);
  else
    ok = ok && has_text(lib, "keep source\n");
  ok = ok && stat(stage, &st) != 0 && errno == ENOENT;
  free(e.stage);
  free(e.matrix);
  remove(lib);
  remove(src);
  remove(cargo);
  remove(dir);
  if (!ok)
    fprintf(stderr, "FAIL plonky3 writer refusal %d: existing files changed or artifacts remain\n", blocker);
  return !ok;
}

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

/* These accepted entries exposed crates whose generated tests failed: an
   empty trace, assert_zero panic text, and mutations with no effect. */
static int check_render(const char *name, uint32_t *word, size_t count,
                        const char *want, const char *absent)
{
  Words w = {word, count, count};
  Entry e = {0};
  const char *msg = check_entry(&e, &w);
  uint32_t *left = msg == NULL ? calloc((size_t)e.width + 1, sizeof *left) : NULL;
  FILE *f = tmpfile();
  int ok = msg == NULL && left != NULL && f != NULL;
  if (ok) {
    const Crate c = {name, &e, &w, left};
    put_lib(f, &c);
    ok = !ferror(f) && fseek(f, 0, SEEK_SET) == 0;
    if (ok) {
      char text[16384];
      const size_t length = fread(text, 1, sizeof text - 1, f);
      text[length] = '\0';
      ok = !ferror(f) && feof(f) && strstr(text, want) != NULL &&
           (absent == NULL || strstr(text, absent) == NULL);
    }
  }
  if (f != NULL && fclose(f) != 0)
    ok = 0;
  free(left);
  free(e.stage);
  free(e.matrix);
  if (!ok)
    fprintf(stderr, "FAIL plonky3 render %s: expected %s without %s\n",
            name, want, absent == NULL ? "a forbidden fragment" : absent);
  return !ok;
}

#define RENDER(name, words, want, absent) check_render(name, words, sizeof words / sizeof *words, want, absent)

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
  for (int blocker = 0; blocker < 6; blocker++)
    failed += check_write_refusal(blocker);
  uint32_t empty[] = {0, 0, 1, 1, 1, 0, 0};
  uint32_t zero[] = {1, 2, 1, 3, 1, 1, 1, 0, 1, 1, 1, 0, 0};
  uint32_t emptycols[] = {1, 1, 0, 0, 0, 1, 1, 1, 1, 2, 1, 0, 1, 1, 1, 0, 0};
  uint32_t zeroinput[] = {1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 2, 1, 1, 2, 1, 1, 1, 0, 0};
  uint32_t emptyoutput[] = {1, 2, 1, 3, 1, 1, 1, 0, 1, 1, 0, 0};
  uint32_t emptyfirst[] = {2, 2, 2, 1, 3, 1, 1, 1, 0, 2, 1, 0, 0, 1, 1, 0, 0};
  uint32_t zeros[] = {2, 2, 2, 1, 3, 1, 1, 1, 0, 2, 1, 1, 0, 0, 1, 1, 0, 0};
  const char *const zero_panic = "#[should_panic(expected = \"constraints had nonzero value on row 0\")]";
  failed += RENDER("emptywidth", empty, "pub const WIDTH: usize = 1;", "LAST_STAGE");
  failed += RENDER("emptypad", empty, "pub const ROW: [u32; WIDTH] = [0];", "rejects_mutated_output");
  failed += RENDER("emptyconstraint", empty, "builder.assert_zero(c0);", NULL);
  failed += RENDER("zero", zero, zero_panic, "values didn't match on row 0");
  failed += RENDER("emptycols", emptycols, zero_panic, "values didn't match on row 0");
  failed += RENDER("zeroinput", zeroinput, "fn accepts_mutated_weight()", "rejects_mutated_weight");
  failed += RENDER("emptyoutput", emptyoutput, "fn accepts_z7_trace()", "LAST_STAGE");
  failed += RENDER("emptyfirst", emptyfirst, "fn rejects_mutated_output()", "FIRST_STAGE");
  failed += RENDER("zeros", zeros, "fn rejects_mutated_stage()", "values didn't match on row 0");
  if (failed != 0)
    return 1;
  puts("plonky3 reader: 6 checked");
  puts("plonky3 writer refusals: 6 checked");
  puts("plonky3 render: 9 checked");
  return 0;
}

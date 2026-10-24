/* Regression checks for the Halo2 word reader and crate writer. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
static int writer_rename(const char *from, const char *to);
#define rename writer_rename
#define main halo2print_main
#include "../src/halo2.c"
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
  const int length = snprintf(dir, sizeof dir, "%s/zkml-halo2-write-%lu-%u",
                              base != NULL && base[0] != '\0' ? base : "/tmp",
                              (unsigned long)getpid(), directory_number++);
  if (length < 0 || (size_t)length >= sizeof dir || mkdir(dir, 0700) != 0) {
    fprintf(stderr, "FAIL halo2 writer refusal %d: cannot make the directory %s\n", blocker, dir);
    return 1;
  }
  char cargo[sizeof dir + sizeof "/Cargo.toml"];
  char src[sizeof dir + sizeof "/src"];
  char lib[sizeof dir + sizeof "/src/lib.rs"];
  char stage[sizeof dir + sizeof "/.halo2print"];
  snprintf(cargo, sizeof cargo, "%s/Cargo.toml", dir);
  snprintf(src, sizeof src, "%s/src", dir);
  snprintf(lib, sizeof lib, "%s/src/lib.rs", dir);
  snprintf(stage, sizeof stage, "%s/.halo2print", dir);
  uint32_t word[] = {1, 2, 8, 0, 0, 0, 0, 0, 0, 1, 2, 8, 0, 1, 1, 0, 0};
  Words w = {word, sizeof word / sizeof *word, sizeof word / sizeof *word};
  Entry e = {0};
  const Crate c = {"empty", &e, &w};
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
  free(e.matrix);
  remove(lib);
  remove(src);
  remove(cargo);
  remove(dir);
  if (!ok)
    fprintf(stderr, "FAIL halo2 writer refusal %d: existing files changed or artifacts remain\n", blocker);
  return !ok;
}

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
  for (int blocker = 0; blocker < 6; blocker++)
    failed += check_write_refusal(blocker);
  if (failed != 0)
    return 1;
  puts("halo2 reader: 5 checked");
  puts("halo2 writer refusals: 6 checked");
  return 0;
}

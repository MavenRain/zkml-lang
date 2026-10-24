#!/bin/sh
# The kit gate. `make check` runs it from the kit directory after the build.
set -eu
fail=0
count=0
for prog in examples/*.lang; do
  out=$(build/langc check "$prog" 2>&1) || true
  count=$((count + 1))
  if [ "$out" != "ok" ]; then
    echo "FAIL $prog: $out"
    fail=$((fail + 1))
  fi
done
echo "examples: $count checked, $fail failed"

tmp=${TMPDIR:-/tmp}/langc-gate.$$
mkdir -p "$tmp"
awk 'BEGIN { printf "def deep : Nat := " } { } END {
  for (i = 0; i < 1100; i++) printf "("; printf "1"; for (i = 0; i < 1100; i++) printf ")"; print "" }' </dev/null >"$tmp/deep.lang"

# refusals DIR LABEL: each line of DIR/expect.txt is `FILE CODE`. `langc check`
# must exit 1 and print `langc: CODE: ...` on stderr.
refusals() {
  checked=0
  while read -r file code; do
    path="$1/$file"
    [ "$file" = "deep.lang" ] && path="$tmp/deep.lang"
    status=0
    build/langc check "$path" 2>"$tmp/err" >/dev/null || status=$?
    first=$(head -n 1 "$tmp/err")
    checked=$((checked + 1))
    case "$status:$first" in
      "1:langc: $code: "*) ;;
      *) echo "FAIL $file: want exit 1 and $code, got exit $status: $first"; fail=$((fail + 1)) ;;
    esac
  done <"$1/expect.txt"
  echo "$2 refusals: $checked checked"
}
refusals test/parse parse
refusals test/check check

# The arena row: build/langc-256 is langc with a 256 MiB arena (the shipped
# langc has 1 GiB). Each example must check, and the 4 Fr refusals of
# test/check/expect.txt must give their codes.
capped=0
for prog in examples/*.lang; do
  out=$(build/langc-256 check "$prog" 2>&1) || true
  capped=$((capped + 1))
  if [ "$out" != "ok" ]; then
    echo "FAIL langc-256 $prog: $out"
    fail=$((fail + 1))
  fi
done
rg '^fr-(run-residual-wrong|run-op-nonzero|matvec-wrong|run-lookup-wrong)\.lang ' test/check/expect.txt >"$tmp/capped.txt"
while read -r file code; do
  status=0
  build/langc-256 check "test/check/$file" 2>"$tmp/err" >/dev/null || status=$?
  first=$(head -n 1 "$tmp/err")
  capped=$((capped + 1))
  case "$status:$first" in
    "1:langc: $code: "*) ;;
    *) echo "FAIL langc-256 $file: want exit 1 and $code, got exit $status: $first"; fail=$((fail + 1)) ;;
  esac
done <"$tmp/capped.txt"
echo "langc-256: $capped checked"

# A refl refusal shows both normal forms.
build/langc check test/check/bad-refl.lang 2>"$tmp/err" >/dev/null || true
if ! rg -q -F 'the left side normalizes to 1201, the right side normalizes to 1200' "$tmp/err"; then
  echo "FAIL bad-refl.lang: the message does not show both normal forms"
  fail=$((fail + 1))
fi

# Each line of test/eval/expect.txt is `FILE NAME [ARGS] => OUTPUT`. `langc eval`
# must exit 0 and print OUTPUT. A trap also prints `langc: EVAL_OVERFLOW: NAME: ...`
# on stderr.
evals=0
while IFS= read -r line; do
  want=${line#* => }
  set -- ${line%% => *}
  file=$1
  name=$2
  shift 2
  status=0
  got=$(build/langc eval "examples/$file.lang" "$name" "$@" 2>"$tmp/err") || status=$?
  first=$(head -n 1 "$tmp/err")
  want_err=""
  [ "$want" = "trap" ] && want_err="langc: EVAL_OVERFLOW: $name: "
  evals=$((evals + 1))
  case "$status|$got|$first" in
    "0|$want|$want_err"*) ;;
    *) echo "FAIL eval $file $name: want $want, got exit $status: $got $first"; fail=$((fail + 1)) ;;
  esac
done <test/eval/expect.txt
echo "evals: $evals checked"

# eval_refused STATUS CODE NAME [ARGS]: `langc eval examples/entries.lang NAME
# ARGS` must exit STATUS and print `langc: CODE: NAME: ...` on stderr.
eval_refused() {
  want_status=$1
  code=$2
  shift 2
  status=0
  build/langc eval examples/entries.lang "$@" 2>"$tmp/err" >/dev/null || status=$?
  case "$status:$(head -n 1 "$tmp/err")" in
    "$want_status:langc: $code: $1: "*) ;;
    *) echo "FAIL eval $*: want exit $want_status and $code, got exit $status"; fail=$((fail + 1)) ;;
  esac
}
eval_refused 2 EVAL_ARGS addPrice 1
eval_refused 2 EVAL_ARGS addPrice x 1
eval_refused 2 EVAL_ARGS addPrice 18446744073709551616 1
eval_refused 2 EVAL_ARGS pick 2 4 9
eval_refused 1 EVAL_ENTRY nope

# The LLZK printer rows (slice Z8c). The words of a row are test/llzk/CASE.words,
# or else the `langc eval` output of the entry CASEWords of examples/llzk.lang.
llzk_words() {
  if [ -f "test/llzk/$1.words" ]; then
    cp "test/llzk/$1.words" "$tmp/words"
  else
    build/langc eval examples/llzk.lang "${1}Words" >"$tmp/words" 2>/dev/null || true
  fi
}
# llzk_golden CASE: build/llzkprint prints the module test/llzk/CASE.mlir.
llzks=0
llzk_golden() {
  llzk_words "$1"
  status=0
  build/llzkprint <"$tmp/words" >"$tmp/llzk.mlir" 2>/dev/null || status=$?
  llzks=$((llzks + 1))
  if [ "$status" -ne 0 ] || ! cmp -s "$tmp/llzk.mlir" "test/llzk/$1.mlir"; then
    echo "FAIL llzk $1: exit $status or a different module"
    fail=$((fail + 1))
  fi
}
# llzk_refused CASE MESSAGE: build/llzkprint exits 1, prints the one line
# `llzkprint: MESSAGE` on stderr and prints nothing on stdout.
llzk_refusals=0
llzk_refused() {
  llzk_words "$1"
  status=0
  build/llzkprint <"$tmp/words" >"$tmp/llzk.mlir" 2>"$tmp/err" || status=$?
  llzk_refusals=$((llzk_refusals + 1))
  case "$status|$(cat "$tmp/err")|$(wc -c <"$tmp/llzk.mlir" | tr -d ' ')" in
    "1|llzkprint: $2|0") ;;
    *) echo "FAIL llzk refusal $1: want exit 1 and $2, got exit $status"; fail=$((fail + 1)) ;;
  esac
}
# decimal.words has the consts 0, 1, r - 1 and 2^32 (D4); r.words has r.
for name in matmul2 residual2 seq2 zero and decimal; do
  llzk_golden "$name"
done
echo "llzk: $llzks checked"
llzk_refused lookup 'lookup is refused (flag 69 (a))'
llzk_refused version 'the version is not 1'
llzk_refused field 'the field id is not 1 or 2'
llzk_refused count 'the block has extra words'
llzk_refused short 'the block is short'
llzk_refused forward 'a value number is not defined yet'
llzk_refused outadd 'an out value is an operand of an op that is not eq'
llzk_refused twice 'an output is written twice'
llzk_refused r 'a const is not below the field modulus'
echo "llzk refusals: $llzk_refusals checked"

# The Halo2 printer rows (slice Z9c). The words of a row are test/halo2/CASE.words,
# or else the `langc eval` output of the entry CASEHalo2 of examples/halo2.lang.
halo2_words() {
  if [ -f "test/halo2/$1.words" ]; then
    cp "test/halo2/$1.words" "$tmp/words"
  else
    build/langc eval examples/halo2.lang "${1}Halo2" >"$tmp/words" 2>/dev/null || true
  fi
}
# halo2_golden CASE: build/halo2print exits 0, prints nothing and writes the
# crate $tmp/halo2/CASE. Its Cargo.toml and src/lib.rs are the same as
# test/halo2/CASE/Cargo.toml and test/halo2/CASE/src/lib.rs.
halo2s=0
halo2_golden() {
  halo2_words "$1"
  status=0
  build/halo2print "$1" "$tmp/halo2/$1" <"$tmp/words" >"$tmp/out" 2>"$tmp/err" || status=$?
  halo2s=$((halo2s + 1))
  if [ "$status" -ne 0 ] || [ -s "$tmp/out" ] || [ -s "$tmp/err" ] ||
    ! cmp -s "$tmp/halo2/$1/Cargo.toml" "test/halo2/$1/Cargo.toml" ||
    ! cmp -s "$tmp/halo2/$1/src/lib.rs" "test/halo2/$1/src/lib.rs"; then
    echo "FAIL halo2 $1: exit $status or a different crate"
    fail=$((fail + 1))
  fi
}
# halo2_refused CASE MESSAGE: build/halo2print exits 1, prints the one line
# `halo2print: MESSAGE` on stderr, prints nothing on stdout and does not make
# the directory $tmp/halo2/CASE.
halo2_refusals=0
halo2_refused() {
  halo2_words "$1"
  status=0
  build/halo2print "$1" "$tmp/halo2/$1" <"$tmp/words" >"$tmp/out" 2>"$tmp/err" || status=$?
  halo2_refusals=$((halo2_refusals + 1))
  case "$status|$(cat "$tmp/err")|$(wc -c <"$tmp/out" | tr -d ' ')|$([ -e "$tmp/halo2/$1" ] && echo written)" in
    "1|halo2print: $2|0|") ;;
    *) echo "FAIL halo2 refusal $1: want exit 1 and $2, got exit $status"; fail=$((fail + 1)) ;;
  esac
}
# The printer makes DIR and DIR/src only, so the parent of DIR must exist.
mkdir -p "$tmp/halo2"
for name in matmul2 residual2 seq2 zero wrap; do
  halo2_golden "$name"
done
echo "halo2: $halo2s checked"
# Each refusal file is the matmul2Halo2 words with one change (D3).
halo2_refused field 'the field id is not 2'
halo2_refused index 'a matrix index is not below the matrix count'
halo2_refused sum 'the mapped matrix entries are not the weight count'
halo2_refused inputs 'the input count is not the circuit input count'
halo2_refused stage 'the last stage entry count is not the output count'
halo2_refused element 'an element is not below the field modulus'
halo2_refused extra 'a word follows the witness block'
# There is no test/halo2/Bad.words, so the words are empty. The printer checks
# NAME before it reads the words.
halo2_refused Bad 'the name is empty or has a character that is not a to z or 0 to 9'
# The write refusal: DIR is a regular file, so the printer cannot make DIR.
: >"$tmp/halo2/file"
halo2_words matmul2
status=0
build/halo2print file "$tmp/halo2/file" <"$tmp/words" >"$tmp/out" 2>"$tmp/err" || status=$?
halo2_refusals=$((halo2_refusals + 1))
case "$status|$(cat "$tmp/err")|$(wc -c <"$tmp/out" | tr -d ' ')" in
  "1|halo2print: the write to $tmp/halo2/file failed|0") ;;
  *) echo "FAIL halo2 refusal file: want exit 1 and the write failure, got exit $status"; fail=$((fail + 1)) ;;
esac
echo "halo2 refusals: $halo2_refusals checked"

status=0
build/langc 2>/dev/null || status=$?
[ "$status" -eq 2 ] || { echo "FAIL usage: want exit 2, got $status"; fail=$((fail + 1)); }
status=0
build/langc check "$tmp/missing.lang" 2>/dev/null || status=$?
[ "$status" -eq 2 ] || { echo "FAIL missing file: want exit 2, got $status"; fail=$((fail + 1)); }

# The IR: each example lowers, with one func line for each entry.
irs=0
for f in examples/*.lang; do
  irs=$((irs + 1))
  status=0
  build/langc ir "$f" > "$tmp/ir.txt" 2> "$tmp/ir.err" || status=$?
  funcs=$(rg -c '^func ' "$tmp/ir.txt" || echo 0)
  entries=$(build/langc abi "$f" | rg -c '.' || echo 0)
  [ "$status" -eq 0 ] && [ "$funcs" -eq "$entries" ] || { echo "FAIL ir $f: exit $status, $funcs funcs for $entries entries: $(head -n 1 "$tmp/ir.err")"; fail=$((fail + 1)); }
done
echo "ir: $irs lowered"
status=0
build/langc check test/ir/closure.lang > /dev/null || status=$?
[ "$status" -eq 0 ] || { echo "FAIL closure.lang: check exit $status"; fail=$((fail + 1)); }
status=0
first=$(build/langc ir test/ir/closure.lang 2>&1 > /dev/null) || status=$?
case "$status:$first" in
  "1:langc: REFUSE_RUNTIME_CLOSURE: "*) ;;
  *) echo "FAIL closure.lang: want exit 1 and REFUSE_RUNTIME_CLOSURE, got exit $status: $first"; fail=$((fail + 1)) ;;
esac
status=0
build/langc check test/ir/path.lang > /dev/null || status=$?
[ "$status" -eq 0 ] || { echo "FAIL path.lang: check exit $status"; fail=$((fail + 1)); }
status=0
first=$(build/langc ir test/ir/path.lang 2>&1 > /dev/null) || status=$?
case "$status:$first" in
  "1:langc: REFUSE_RUNTIME_PATH: "*) ;;
  *) echo "FAIL path.lang: want exit 1 and REFUSE_RUNTIME_PATH, got exit $status: $first"; fail=$((fail + 1)) ;;
esac
status=0
build/langc check test/ir/hit-path.lang > /dev/null || status=$?
[ "$status" -eq 0 ] || { echo "FAIL hit-path.lang: check exit $status"; fail=$((fail + 1)); }
status=0
first=$(build/langc ir test/ir/hit-path.lang 2>&1 > /dev/null) || status=$?
case "$status:$first" in
  "1:langc: REFUSE_RUNTIME_PATH: "*) ;;
  *) echo "FAIL hit-path.lang: want exit 1 and REFUSE_RUNTIME_PATH, got exit $status: $first"; fail=$((fail + 1)) ;;
esac
status=0
build/langc check test/ir/qiit-tree.lang > /dev/null || status=$?
[ "$status" -eq 0 ] || { echo "FAIL qiit-tree.lang: check exit $status"; fail=$((fail + 1)); }
status=0
first=$(build/langc ir test/ir/qiit-tree.lang 2>&1 > /dev/null) || status=$?
case "$status:$first" in
  "1:langc: REFUSE_RUNTIME_TREE: "*) ;;
  *) echo "FAIL qiit-tree.lang: want exit 1 and REFUSE_RUNTIME_TREE, got exit $status: $first"; fail=$((fail + 1)) ;;
esac
status=0
build/langc check test/ir/rose-tree.lang > /dev/null || status=$?
[ "$status" -eq 0 ] || { echo "FAIL rose-tree.lang: check exit $status"; fail=$((fail + 1)); }
status=0
first=$(build/langc ir test/ir/rose-tree.lang 2>&1 > /dev/null) || status=$?
case "$status:$first" in
  "1:langc: REFUSE_RUNTIME_TREE: bad: a fold over the mutual block of Rose "*) ;;
  *) echo "FAIL rose-tree.lang: want exit 1 and the mutual block refusal, got exit $status: $first"; fail=$((fail + 1)) ;;
esac
status=0
build/langc check test/ir/locals.lang > /dev/null || status=$?
[ "$status" -eq 0 ] || { echo "FAIL locals.lang: check exit $status"; fail=$((fail + 1)); }
status=0
first=$(build/langc ir test/ir/locals.lang 2>&1 > /dev/null) || status=$?
case "$status:$first" in
  "1:langc: REFUSE_LOCALS: big: "*) ;;
  *) echo "FAIL locals.lang: want exit 1 and REFUSE_LOCALS, got exit $status: $first"; fail=$((fail + 1)) ;;
esac
got=$(build/langc eval test/ir/lazy.lang lazy 3 2>&1) || true
[ "$got" = "3" ] || { echo "FAIL lazy.lang: want 3, got $got"; fail=$((fail + 1)); }
build/langc ir test/ir/lazy.lang > /dev/null || { echo "FAIL lazy.lang: ir"; fail=$((fail + 1)); }
for want in "foldOnes 3:6" "mapUp 3:18" "bindUp 3:36" "filterUp 5:3" "trapHead 5:1" "trapSum 5:trap" "trapSum 2:3" \
  "filterTrap 2:3" "filterTrap 5:trap" "bindTrap 5:1" "stackSum 3:15" "stackTrap 0:7" "stackTrap 2:5"; do
  call=${want%%:*}
  got=$(build/langc eval test/ir/spine.lang ${call% *} ${call#* } 2>"$tmp/err") || true
  want_err=""
  [ "${want#*:}" = "trap" ] && want_err="langc: EVAL_OVERFLOW: ${call% *}: "
  case "$got|$(head -n 1 "$tmp/err")" in
    "${want#*:}|$want_err"*) ;;
    *) echo "FAIL spine.lang $call: want ${want#*:}, got $got"; fail=$((fail + 1)) ;;
  esac
done
build/langc ir test/ir/spine.lang > /dev/null || { echo "FAIL spine.lang: ir"; fail=$((fail + 1)); }
status=0
build/langc check test/ir/tree.lang > /dev/null || status=$?
[ "$status" -eq 0 ] || { echo "FAIL tree.lang: check exit $status"; fail=$((fail + 1)); }
status=0
first=$(build/langc ir test/ir/tree.lang 2>&1 > /dev/null) || status=$?
case "$status:$first" in
  "1:langc: REFUSE_RUNTIME_TREE: "*) ;;
  *) echo "FAIL tree.lang: want exit 1 and REFUSE_RUNTIME_TREE, got exit $status: $first"; fail=$((fail + 1)) ;;
esac

# The Wasm target: each module builds and is valid. Then the differential: on
# the grid of test/grid.awk, node and `langc eval` give the same result for
# each entry (trap == trap). The stderr of eval is not merged into stdout.
wasms=0
cases=0
mismatches=0
for f in examples/*.lang test/ir/spine.lang test/ir/lazy.lang test/ir/proof-trap.lang; do
  w="$tmp/$(basename "$f" .lang).wasm"
  wasms=$((wasms + 1))
  if ! build/langc build "$f" -o "$w" 2>"$tmp/w.err" || ! wasm-validate "$w" 2>>"$tmp/w.err" ||
    ! wasm-objdump -x "$w" >/dev/null 2>>"$tmp/w.err"; then
    echo "FAIL wasm $f: $(head -n 1 "$tmp/w.err")"
    fail=$((fail + 1))
    continue
  fi
  build/langc ir "$f" | awk -f test/grid.awk >"$tmp/cases.txt"
  build/langc abi "$f" >"$tmp/abi.txt"
  : >"$tmp/want.txt"
  while read -r call; do
    status=0
    want=$(build/langc eval "$f" $call 2>/dev/null) || status=$?
    [ "$status" -eq 0 ] || want="eval-exit-$status"
    echo "$want" >>"$tmp/want.txt"
  done <"$tmp/cases.txt"
  if ! node test/run-wasm.mjs "$w" - "$tmp/abi.txt" <"$tmp/cases.txt" >"$tmp/got.txt" 2>"$tmp/w.err"; then
    echo "FAIL node $f: $(head -n 1 "$tmp/w.err")"
    fail=$((fail + 1))
    continue
  fi
  cases=$((cases + $(rg -c '.' "$tmp/cases.txt" || echo 0)))
  paste -d '|' "$tmp/cases.txt" "$tmp/want.txt" "$tmp/got.txt" | awk -F '|' '$2 != $3' >"$tmp/bad.txt"
  bad=$(rg -c '.' "$tmp/bad.txt" || echo 0)
  mismatches=$((mismatches + bad))
  [ "$bad" -eq 0 ] || { echo "FAIL differential $f: $bad mismatches, first $(head -n 1 "$tmp/bad.txt")"; fail=$((fail + 1)); }
done
echo "wasm: $wasms modules valid"
echo "differential: $cases cases, $mismatches mismatches"
# The grid gives the BabyBear p - 1 and p values (D8) to a Nat parameter of an
# entry with no loop.
printf 'func f (nat)\n' | awk -f test/grid.awk >"$tmp/grid.txt"
[ "$(rg -c -x 'f (2013265920|2013265921)' "$tmp/grid.txt")" = 2 ] ||
  { echo "FAIL grid p values"; fail=$((fail + 1)); }
# A Flag argument larger than 1 traps (langc eval refuses it with EVAL_ARGS).
got=$(node test/run-wasm.mjs "$tmp/entries.wasm" pick 2 4 9 2>&1) || true
[ "$got" = "trap" ] || { echo "FAIL wasm pick 2 4 9: want trap, got $got"; fail=$((fail + 1)); }
rm -rf "$tmp"

if rg -n '\x{2013}|\x{2014}' . >/dev/null; then
  echo "FAIL: an em-dash or en-dash is in the kit"
  fail=$((fail + 1))
fi
echo "gate: $fail failures"
[ "$fail" -eq 0 ]

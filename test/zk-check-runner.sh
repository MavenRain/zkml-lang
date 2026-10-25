#!/bin/sh
# Check the failure handling of the zk-check, halo2-check and plonky3-check
# recipes without an LLVM installation or a Rust toolchain.
set -eu
root=$(pwd)
tmp=$(mktemp -d "${TMPDIR:-/tmp}/zk-check-runner.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir -p "$tmp/build" "$tmp/bin" "$tmp/test/llzk/reject" "$tmp/test/halo2" \
  "$tmp/test/plonky3"
for name in matmul2 residual2 seq2 zero and decimal; do
  : >"$tmp/test/llzk/$name.mlir"
done
: >"$tmp/test/llzk/decimal.words"
echo 'version = 4' >"$tmp/test/halo2/Cargo.lock"
echo 'version = 4' >"$tmp/test/plonky3/Cargo.lock"

# The producer writes valid words even when it reports failure.
cat >"$tmp/build/langc" <<'EOF'
#!/bin/sh
echo '1 1 1 0 0 0 0'
exit "${ZK_CHECK_LANGC_STATUS:-0}"
EOF
cat >"$tmp/build/llzkprint" <<'EOF'
#!/bin/sh
cat
EOF
cat >"$tmp/bin/llzk-opt" <<'EOF'
#!/bin/sh
case "$1" in
  */reject/matmul2-m1-type.mlir)
    echo 'expects different type than prior uses' >&2; exit 1 ;;
  */reject/matmul2-m2-undef.mlir)
    echo 'use of undeclared SSA value name' >&2; exit 1 ;;
esac
EOF

# The printer writes a crate with the mutant tests of D7: zero has no weight.
cat >"$tmp/build/halo2print" <<'EOF'
#!/bin/sh
cat >/dev/null
if [ "$1" = "${HALO2_CHECK_PRINT_FAIL:-}" ]; then exit 1; fi
mkdir -p "$2/src"
: >"$2/Cargo.toml"
echo 'fn rejects_mutated_output' >"$2/src/lib.rs"
if [ "$1" != zero ]; then echo 'fn rejects_mutated_weight' >>"$2/src/lib.rs"; fi
EOF
# The test command exits 2 if the recipe does not use the pinned lock.
cat >"$tmp/bin/cargo-test" <<'EOF'
#!/bin/sh
cmp -s build/halo2/Cargo.lock test/halo2/Cargo.lock || exit 2
case " $* " in
  *" --offline --locked "*) ;;
  *) exit 2 ;;
esac
case " $* " in
  *" -p zkml-halo2-${HALO2_CHECK_TEST_FAIL:-} "*) exit 1 ;;
esac
EOF
# The Plonky3 printer writes a crate with the rejects counts of the goldens:
# matmul2 2, residual2 3, seq2 3, wrap 2, zero 1.
cat >"$tmp/build/plonky3print" <<'EOF'
#!/bin/sh
cat >/dev/null
if [ "$1" = "${PLONKY3_CHECK_PRINT_FAIL:-}" ]; then exit 1; fi
mkdir -p "$2/src"
: >"$2/Cargo.toml"
echo 'fn rejects_mutated_output' >"$2/src/lib.rs"
if [ "$1" != zero ]; then echo 'fn rejects_mutated_weight' >>"$2/src/lib.rs"; fi
case "$1" in
  residual2 | seq2) echo 'fn rejects_mutated_stage' >>"$2/src/lib.rs" ;;
esac
EOF
cat >"$tmp/bin/cargo-test-plonky3" <<'EOF'
#!/bin/sh
cmp -s build/plonky3/Cargo.lock test/plonky3/Cargo.lock || exit 2
case " $* " in
  *" --offline --locked "*) ;;
  *) exit 2 ;;
esac
case " $* " in
  *" -p zkml-plonky3-${PLONKY3_CHECK_TEST_FAIL:-} "*) exit 1 ;;
esac
EOF

# A failed copy can leave usable words, so the recipe must check its status.
ZK_CHECK_REAL_CP=$(command -v cp)
export ZK_CHECK_REAL_CP
cat >"$tmp/bin/cp" <<'EOF'
#!/bin/sh
"$ZK_CHECK_REAL_CP" "$@" || exit "$?"
exit "${ZK_CHECK_COPY_STATUS:-0}"
EOF
chmod +x "$tmp/build/langc" "$tmp/build/llzkprint" "$tmp/build/halo2print" \
  "$tmp/build/plonky3print" "$tmp/bin/llzk-opt" "$tmp/bin/cargo-test" \
  "$tmp/bin/cargo-test-plonky3" "$tmp/bin/cp"
PATH="$tmp/bin:$PATH"
export PATH

run() {
  (cd "$tmp" && make -s -f "$root/Makefile" -o build/langc -o build/llzkprint \
    "LLZK_OPT=$tmp/bin/llzk-opt" zk-check) >"$tmp/out" 2>&1
}
run_halo2() {
  (cd "$tmp" && make -s -f "$root/Makefile" -o build/langc -o build/halo2print \
    "CARGO_TEST=$tmp/bin/cargo-test" halo2-check) >"$tmp/out" 2>&1
}
# halo2_refused VARIABLE VALUE WHAT TEXT: with VARIABLE=VALUE, halo2-check
# fails and prints TEXT.
halo2_refused() {
  if (export "$1=$2"; run_halo2); then
    echo "FAIL zk-check runner: halo2-check ignored $3"
    exit 1
  fi
  case "$(cat "$tmp/out")" in
    *"$4"*) ;;
    *) cat "$tmp/out"; exit 1 ;;
  esac
}
run_plonky3() {
  (cd "$tmp" && make -s -f "$root/Makefile" -o build/langc -o build/plonky3print \
    "CARGO_TEST=$tmp/bin/cargo-test-plonky3" plonky3-check) >"$tmp/out" 2>&1
}
# plonky3_refused VARIABLE VALUE WHAT TEXT: with VARIABLE=VALUE, plonky3-check
# fails and prints TEXT.
plonky3_refused() {
  if (export "$1=$2"; run_plonky3); then
    echo "FAIL zk-check runner: plonky3-check ignored $3"
    exit 1
  fi
  case "$(cat "$tmp/out")" in
    *"$4"*) ;;
    *) cat "$tmp/out"; exit 1 ;;
  esac
}
run || { cat "$tmp/out"; exit 1; }
test "$(cat "$tmp/out")" = 'zk-check: 12 accepted, 2 rejected'
run_halo2 || { cat "$tmp/out"; exit 1; }
test "$(cat "$tmp/out")" = 'halo2-check: 5 accepted, 9 rejected'
halo2_refused ZK_CHECK_LANGC_STATUS 1 'a failed word evaluation' \
  'FAIL halo2-check matmul2: langc eval fails'
halo2_refused HALO2_CHECK_PRINT_FAIL seq2 'a failed crate print' \
  'FAIL halo2-check seq2: halo2print refuses the words'
halo2_refused HALO2_CHECK_TEST_FAIL seq2 'a failed test run' \
  'halo2-check: 4 accepted, 7 rejected'
run_plonky3 || { cat "$tmp/out"; exit 1; }
test "$(cat "$tmp/out")" = 'plonky3-check: 5 accepted, 11 rejected'
plonky3_refused ZK_CHECK_LANGC_STATUS 1 'a failed word evaluation' \
  'FAIL plonky3-check matmul2: langc eval fails'
plonky3_refused PLONKY3_CHECK_PRINT_FAIL seq2 'a failed crate print' \
  'FAIL plonky3-check seq2: plonky3print refuses the words'
plonky3_refused PLONKY3_CHECK_TEST_FAIL seq2 'a failed test run' \
  'plonky3-check: 4 accepted, 8 rejected'
# A subshell for each row: sh can keep an assignment before a function call
# after the call, and the next row must not see it.
if (export ZK_CHECK_LANGC_STATUS=1; run); then
  echo 'FAIL zk-check runner: ignored a failed word evaluation'
  exit 1
fi
if (export ZK_CHECK_COPY_STATUS=1; run); then
  echo 'FAIL zk-check runner: ignored a failed words copy'
  exit 1
fi
echo 'zk-check runner: 11 checked'

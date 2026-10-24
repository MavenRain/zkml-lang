#!/bin/sh
# Check the failure handling of the zk-check and halo2-check recipes without
# an LLVM installation or a Rust toolchain.
set -eu
root=$(pwd)
tmp=$(mktemp -d "${TMPDIR:-/tmp}/zk-check-runner.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir -p "$tmp/build" "$tmp/bin" "$tmp/test/llzk/reject" "$tmp/test/halo2"
for name in matmul2 residual2 seq2 zero and decimal; do
  : >"$tmp/test/llzk/$name.mlir"
done
: >"$tmp/test/llzk/decimal.words"
echo 'version = 4' >"$tmp/test/halo2/Cargo.lock"

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

# A failed copy can leave usable words, so the recipe must check its status.
ZK_CHECK_REAL_CP=$(command -v cp)
export ZK_CHECK_REAL_CP
cat >"$tmp/bin/cp" <<'EOF'
#!/bin/sh
"$ZK_CHECK_REAL_CP" "$@" || exit "$?"
exit "${ZK_CHECK_COPY_STATUS:-0}"
EOF
chmod +x "$tmp/build/langc" "$tmp/build/llzkprint" "$tmp/build/halo2print" \
  "$tmp/bin/llzk-opt" "$tmp/bin/cargo-test" "$tmp/bin/cp"
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
echo 'zk-check runner: 7 checked'

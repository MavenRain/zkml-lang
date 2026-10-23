#!/bin/sh
# Check the zk-check recipe's failure handling without an LLVM installation.
set -eu
root=$(pwd)
tmp=$(mktemp -d "${TMPDIR:-/tmp}/zk-check-runner.XXXXXX")
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir -p "$tmp/build" "$tmp/bin" "$tmp/test/llzk/reject"
for name in matmul2 residual2 seq2 zero and decimal; do
  : >"$tmp/test/llzk/$name.mlir"
done
: >"$tmp/test/llzk/decimal.words"

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

# A failed copy can leave usable words, so the recipe must check its status.
ZK_CHECK_REAL_CP=$(command -v cp)
export ZK_CHECK_REAL_CP
cat >"$tmp/bin/cp" <<'EOF'
#!/bin/sh
"$ZK_CHECK_REAL_CP" "$@" || exit "$?"
exit "${ZK_CHECK_COPY_STATUS:-0}"
EOF
chmod +x "$tmp/build/langc" "$tmp/build/llzkprint" "$tmp/bin/llzk-opt" "$tmp/bin/cp"
PATH="$tmp/bin:$PATH"
export PATH

run() {
  (cd "$tmp" && make -s -f "$root/Makefile" -o build/langc -o build/llzkprint \
    "LLZK_OPT=$tmp/bin/llzk-opt" zk-check) >"$tmp/out" 2>&1
}
run || { cat "$tmp/out"; exit 1; }
test "$(cat "$tmp/out")" = 'zk-check: 12 accepted, 2 rejected'
if ZK_CHECK_LANGC_STATUS=1 run; then
  echo 'FAIL zk-check runner: ignored a failed word evaluation'
  exit 1
fi
if ZK_CHECK_COPY_STATUS=1 run; then
  echo 'FAIL zk-check runner: ignored a failed words copy'
  exit 1
fi
echo 'zk-check runner: 3 checked'

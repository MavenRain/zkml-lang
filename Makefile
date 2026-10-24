# Builds the tcc-wasm kit with TinyCC. See README.md.
TCC ?= tcc
CC ?= cc
CFLAGS = -std=c99 -Wall -Werror -Isrc
CLANG_FLAGS = -std=c99 -Wall -Wextra -Wswitch-enum -Werror -fsyntax-only -Isrc
FRONT = src/front/base.c src/front/lexer.c src/front/parser.c src/front/front.c src/front/eval.c src/front/check.c src/front/lower.c
TARGET = src/wasm.c
SRC = src/main.c $(FRONT) $(TARGET)
FRONT_TOOL = test/fronttool.c $(filter-out src/front/front.c src/front/lower.c,$(FRONT))
HEADERS = $(wildcard src/*.h src/front/*.h)

.PHONY: build check check-clang clean halo2-check zk-check

build: build/langc build/fronttool build/llzkprint build/halo2print

build/domain.c: domain/domain.lang gen/embed.c
	mkdir -p build
	$(TCC) -run gen/embed.c domain/domain.lang build/domain.c

build/langc: $(SRC) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) -o build/langc $(SRC) build/domain.c

# The arena row of test/gate.sh: the same langc with a 256 MiB arena.
build/langc-256: $(SRC) $(HEADERS) build/domain.c
	$(TCC) $(CFLAGS) '-DARENA_LIMIT_BYTES=((size_t)256 << 20)' -o build/langc-256 $(SRC) build/domain.c

build/fronttool: $(FRONT_TOOL) $(HEADERS)
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/fronttool $(FRONT_TOOL)

# The LLZK printer (slice Z8c, flag 68 (c)): the `langc eval` words of a module
# to LLZK text. src/main.c does not use it.
build/llzkprint: src/llzk.c
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/llzkprint src/llzk.c

# The Halo2 printer (slice Z9c, flag 78 (a)): the `langc eval` words of a
# Halo2 entry to a Rust crate. src/main.c does not use it.
build/halo2print: src/halo2.c
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/halo2print src/halo2.c

build/halo2-reader: test/halo2-reader.c src/halo2.c
	mkdir -p build
	$(TCC) $(CFLAGS) -o build/halo2-reader test/halo2-reader.c

check-clang: build/domain.c
	$(CC) $(CLANG_FLAGS) $(SRC) build/domain.c
	$(CC) $(CLANG_FLAGS) $(FRONT_TOOL)
	$(CC) $(CLANG_FLAGS) src/llzk.c
	$(CC) $(CLANG_FLAGS) src/halo2.c
	$(CC) $(CLANG_FLAGS) test/halo2-reader.c

check: build/langc build/langc-256 build/fronttool build/llzkprint build/halo2print build/halo2-reader check-clang
	build/halo2-reader
	sh test/gate.sh
	sh test/review.sh
	sh test/zk-check-runner.sh
	build/fronttool

# The LLZK tool row (slice Z8d, D11). llzk-opt must accept each module of
# test/llzk and the printer output of each llzk case of test/gate.sh, and must
# reject the 2 mutants of test/llzk/reject. `check` does not run this row,
# because llzk-opt needs an LLVM and MLIR build (O6). Set LLZK_OPT to the
# llzk-opt of an LLZK 2.1.2 install.
LLZK_OPT ?= /Users/oobi/Documents/zkml-lang-work/tools/llzk/bin/llzk-opt

zk-check: build/langc build/llzkprint
	@set -e; mkdir -p build/zk; ok=0; no=0; fail=0; \
	command -v "$(LLZK_OPT)" >/dev/null || { echo "zk-check: no llzk-opt at $(LLZK_OPT); set LLZK_OPT"; exit 1; }; \
	accept() { \
	  if "$(LLZK_OPT)" "$$1" >/dev/null 2>build/zk/err; then ok=$$((ok + 1)); \
	  else echo "FAIL zk-check $$1: llzk-opt does not accept the module"; cat build/zk/err; fail=$$((fail + 1)); fi; \
	}; \
	reject() { \
	  status=0; "$(LLZK_OPT)" "$$1" >/dev/null 2>build/zk/err || status=$$?; \
	  case "$$status|$$(cat build/zk/err)" in \
	    "1|"*"$$2"*) no=$$((no + 1)) ;; \
	    *) echo "FAIL zk-check $$1: want exit 1 and $$2, got exit $$status"; fail=$$((fail + 1)) ;; \
	  esac; \
	}; \
	for f in test/llzk/*.mlir; do accept "$$f"; done; \
	for n in matmul2 residual2 seq2 zero and decimal; do \
	  if [ -f "test/llzk/$$n.words" ]; then cp "test/llzk/$$n.words" "build/zk/$$n.words"; \
	  else build/langc eval examples/llzk.lang "$${n}Words" >"build/zk/$$n.words"; fi; \
	  if build/llzkprint <"build/zk/$$n.words" >"build/zk/$$n.mlir"; then accept "build/zk/$$n.mlir"; \
	  else echo "FAIL zk-check $$n: llzkprint refuses the words"; fail=$$((fail + 1)); fi; \
	done; \
	reject test/llzk/reject/matmul2-m1-type.mlir 'expects different type than prior uses'; \
	reject test/llzk/reject/matmul2-m2-undef.mlir 'use of undeclared SSA value name'; \
	echo "zk-check: $$ok accepted, $$no rejected"; \
	[ "$$fail" -eq 0 ] && [ "$$no" -eq 2 ]


# The Halo2 Rust row (slice Z9d, D8, D9). halo2print writes the crate of each
# Halo2 entry of examples/halo2.lang into build/halo2. cargo tests each crate
# with the lock test/halo2/Cargo.lock: the MockProver must accept the Z7
# witness and reject each mutated witness. `check` does not run this row,
# because it needs a Rust toolchain and halo2_proofs v0.3.0 in the cargo cache
# (--offline). Set CARGO_TEST='cargo test' if testledger is not installed.
CARGO_TEST ?= testledger -- cargo test

halo2-check: build/langc build/halo2print
	@set -e; mkdir -p build/halo2; ok=0; no=0; fail=0; \
	crates='matmul2 residual2 seq2 wrap zero'; \
	for n in $$crates; do \
	  build/langc eval examples/halo2.lang "$${n}Halo2" >"build/halo2/$$n.words" || \
	    { echo "FAIL halo2-check $$n: langc eval fails"; exit 1; }; \
	  build/halo2print "$$n" "build/halo2/$$n" <"build/halo2/$$n.words" || \
	    { echo "FAIL halo2-check $$n: halo2print refuses the words"; exit 1; }; \
	done; \
	{ printf '[workspace]\nresolver = "2"\nmembers = ['; sep=; \
	  for n in $$crates; do printf '%s"%s"' "$$sep" "$$n"; sep=', '; done; \
	  printf ']\n'; } >build/halo2/Cargo.toml; \
	cp test/halo2/Cargo.lock build/halo2/Cargo.lock; \
	for n in $$crates; do \
	  if $(CARGO_TEST) --offline --locked -j 2 --manifest-path build/halo2/Cargo.toml \
	    --target-dir build/halo2/target -p "zkml-halo2-$$n"; then \
	    ok=$$((ok + 1)); \
	    no=$$((no + $$(awk '/fn rejects_/ { k++ } END { print k + 0 }' "build/halo2/$$n/src/lib.rs"))); \
	  else echo "FAIL halo2-check $$n: cargo test fails"; fail=$$((fail + 1)); fi; \
	done; \
	echo "halo2-check: $$ok accepted, $$no rejected"; \
	[ "$$fail" -eq 0 ] && [ "$$ok" -eq 5 ] && [ "$$no" -eq 9 ]

clean:
	rm -rf build

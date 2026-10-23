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

.PHONY: build check check-clang clean zk-check

build: build/langc build/fronttool build/llzkprint

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

check-clang: build/domain.c
	$(CC) $(CLANG_FLAGS) $(SRC) build/domain.c
	$(CC) $(CLANG_FLAGS) $(FRONT_TOOL)
	$(CC) $(CLANG_FLAGS) src/llzk.c

check: build/langc build/langc-256 build/fronttool build/llzkprint check-clang
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

clean:
	rm -rf build

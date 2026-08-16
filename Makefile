#
# # Micro Minesweeper
#
# ## Makefile -- Build and Test
#
# @description A minimal C Minesweeper for the terminal
# @author [Jon Ruttan](jonruttan@gmail.com)
# @copyright 2025 Jon Ruttan
# @license MIT
#
# ## Usage
#
# ### Build
#
# ```sh
# make
# ```
#
# ### Play
#
# ```sh
# make run LEVEL=intermediate
# ```
#
# ### Test
#
# ```sh
# make test
# make sanitize
# ```
#
# ## Targets
#
# - `all` -- build the binary (default)
# - `check` -- build, then run the suite
# - `test` -- run the suite
# - `sanitize` -- run the suite under ASan and UBSan
# - `verify-runner` -- hold the submodule to RUNNER_VERSION
# - `submodule` -- check out the test-runner submodule
# - `run` -- play, honours LEVEL
# - `clean` -- remove build output
#

CC       ?= cc
CFLAGS   ?= -Wall -Wextra -Wno-unused-parameter
CFLAGS   += -O2

BIN      := minesweeper
SRC      := minesweeper.c

RUNNER   := test-runner/test-runner.sh
RUNNER_H := test-runner/include/test-runner.h
SPECS    := tests

# The test-runner release this project is pinned to. `verify-runner` holds this
# and the submodule to the same version, so the two cannot drift apart
# unnoticed -- every `make test` checks it first.
RUNNER_VERSION := 1.6.1

# Level for `make run`, one of: classic beginner intermediate expert
LEVEL ?= classic

SAN := -fsanitize=address,undefined -fno-omit-frame-pointer

.PHONY: all check test sanitize verify-runner submodule run clean

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

# Build and test, which is what CI runs.
check: all test

test: verify-runner
	sh $(RUNNER) $(SPECS)

sanitize: verify-runner
	CFLAGS="$(CFLAGS) $(SAN)" sh $(RUNNER) $(SPECS)

verify-runner:
	@test -f $(RUNNER_H) || { \
		echo "ERROR: $(RUNNER_H) is missing."; \
		echo "       The test-runner submodule is not checked out."; \
		echo "       Run: make submodule"; \
		exit 1; \
	}
	@have=`sed -n 's/^#define TEST_RUNNER_VERSION "\(.*\)"/\1/p' $(RUNNER_H)`; \
	if [ "$$have" != "$(RUNNER_VERSION)" ]; then \
		echo "ERROR: test-runner version drift."; \
		echo "       Makefile expects: $(RUNNER_VERSION)"; \
		echo "       submodule has:    $$have"; \
		echo "       Re-pin the submodule, or update RUNNER_VERSION in the Makefile."; \
		exit 1; \
	fi; \
	echo "test-runner $$have (pinned)"

submodule:
	git submodule update --init --recursive

run: $(BIN)
	./$(BIN) $(LEVEL)

clean:
	rm -f $(BIN)
	rm -rf $(BIN).dSYM

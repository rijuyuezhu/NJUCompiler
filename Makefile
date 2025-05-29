export WORK_DIR = $(shell pwd)
export BUILD_DIR = $(WORK_DIR)/build
export CFLAGS = -I$(WORK_DIR)/src -Wall -Wextra -g -ggdb -std=c99
export VALGRIND_ARGS = --leak-check=yes --show-leak-kinds=all --errors-for-leak-kinds=all --exit-on-first-error=yes --error-exitcode=1 -q
export ARGS

all: lib
	@$(MAKE) -C tests --no-print-directory -s

run:
	@$(MAKE) -C src run --no-print-directory -s

runvg:
	@$(MAKE) -C src runvg --no-print-directory -s

test: lib
	@$(MAKE) -C tests test --no-print-directory -s

testvg: lib
	@$(MAKE) -C tests testvg --no-print-directory -s

lib:
	@$(MAKE) -C src --no-print-directory -s

clean:
	@$(MAKE) -C src clean --no-print-directory -s
	@$(MAKE) -C tests clean --no-print-directory -s
	@rm -rf $(BUILD_DIR) $(WORK_DIR)/tests/vgcore.*

.PHONY: all test testvg lib  clean

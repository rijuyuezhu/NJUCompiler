export WORK_DIR = $(shell pwd)
export BUILD_DIR = $(WORK_DIR)/build
export CFLAGS = -I$(WORK_DIR)/src -Wall -Wextra -g -std=c99
export ARGS

all: lib
	@$(MAKE) -C tests --no-print-directory -s

test: lib
	@$(MAKE) -C tests test --no-print-directory -s

testvg: lib
	@$(MAKE) -C tests testvg --no-print-directory -s

lib:
	@$(MAKE) -C src --no-print-directory -s

clean:
	@rm -rf $(BUILD_DIR) compile_commands.json $(WORK_DIR)/tests/vgcore.*

.PHONY: all test testvg lib  clean

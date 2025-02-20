SRCS = $(notdir $(shell find ./ -name "*.c"))
DEPS = $(addprefix $(BUILD_DIR)/tests/, $(addsuffix .d,$(SRCS)))
OBJS = $(addprefix $(BUILD_DIR)/tests/, $(addsuffix .o,$(SRCS)))
TEST_EXE = $(BUILD_DIR)/tests/test_main
VALGRIND_ARGS = --leak-check=yes --show-leak-kinds=all --errors-for-leak-kinds=all --exit-on-first-error=yes --error-exitcode=1 -q
$(shell mkdir -p $(BUILD_DIR)/tests)

all: $(TEST_EXE)

test: $(TEST_EXE)
	@echo + RUN $(notdir $(TEST_EXE)) >&2
	@$(TEST_EXE) $(ARGS)

testvg: $(TEST_EXE)
	@valgrind $(VALGRIND_ARGS) $(TEST_EXE) $(ARGS)

$(BUILD_DIR)/tests/%.c.o: %.c
	@echo + CC tests/$(notdir $<) >&2
	@$(CC) $(CFLAGS) -c $< -o $@ -MMD

$(TEST_EXE): $(OBJS)
	@echo + LD $(notdir $@) >&2
	@$(CC) $(CFLAGS) $^ -o $@ -L$(BUILD_DIR) -lcompiler

-include $(DEPS)

.PHONY: all test testvg

CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -std=c99
$(shell mkdir -p $(BUILD_DIR)/src)

SRCS = $(shell find ./ -name "*.c")
DEPS = $(addprefix $(BUILD_DIR)/src/, $(addsuffix .d,$(SRCS)))
OBJS = $(addprefix $(BUILD_DIR)/src/, $(addsuffix .o,$(SRCS)))
LFILE = $(shell find ./ -name "*.l")
YFILE = $(shell find ./ -name "*.y")
LFC = $(shell find ./ -name "*.l" | sed s/[^/]*\\.l/lex.yy.c/)
YFC = $(shell find ./ -name "*.y" | sed s/[^/]*\\.y/syntax.tab.c/)
LFO = $(addprefix $(BUILD_DIR)/src/, $(addsuffix .o,$(LFC)))
YFO = $(addprefix $(BUILD_DIR)/src/, $(addsuffix .o,$(YFC)))

all: $(BUILD_DIR)/src/parser $(BUILD_DIR)/libcompiler.a

$(BUILD_DIR)/src/parser: $(YFO) $(filter-out $(LFO),$(OBJS))
	@echo + LD src/$(notdir $@) >&2
	@$(CC) -o $@ $(filter-out $(LFO),$(OBJS)) -lfl -ly

$(BUILD_DIR)/libcompiler.a: $(YFO) $(filter-out $(LFO) $(BUILD_DIR)/src/./main.c.o,$(OBJS))
	@echo + AR $(notdir $@) >&2
	@ar rcs $@ $(filter-out $(LFO) $(BUILD_DIR)/src/./main.o,$(OBJS)) 

$(YFO): $(LFC) $(YFC)
	@echo + CC src/$(notdir $(YFC)) >&2
	@$(CC) -c $(YFC) -o $(YFO)

$(LFC): $(LFILE)
	@echo + FLEX src/$(notdir $(LFILE)) >&2
	@$(FLEX) -o $(LFC) $(LFILE)

$(YFC): $(YFILE)
	@echo + BISON src/$(notdir $(YFILE)) >&2
	@$(BISON) -o $(YFC) -d -v $(YFILE)

$(BUILD_DIR)/src/%.c.o: %.c
	@echo + CC src/$(notdir $<) >&2
	@$(CC) $(CFLAGS) -c $< -o $@ -MMD

-include $(DEPS)

.PHONY: clean all

clean:
	@rm -f *.yy.c *.tab.c *.tab.h syntax.output

CC = gcc
FLEX = flex
BISON = bison
CFLAGS = -std=c99
$(shell mkdir -p $(BUILD_DIR)/src)

CFILES = $(shell find ./ -name "*.c")
OBJS := $(CFILES:.c=.o)
OBJS := $(addprefix $(BUILD_DIR)/src/,$(OBJS))
DEPS = $(addprefix $(BUILD_DIR)/src/, $(addsuffix .d,$(CFILES)))
LFILE = $(shell find ./ -name "*.l")
YFILE = $(shell find ./ -name "*.y")
LFC = $(shell find ./ -name "*.l" | sed s/[^/]*\\.l/lex.yy.c/)
YFC = $(shell find ./ -name "*.y" | sed s/[^/]*\\.y/syntax.tab.c/)
LFO := $(LFC:.c=.o)
LFO := $(addprefix $(BUILD_DIR)/src/,$(LFO))
YFO := $(YFC:.c=.o)
YFO := $(addprefix $(BUILD_DIR)/src/,$(YFO))

all: $(BUILD_DIR)/src/parser $(BUILD_DIR)/libcompiler.a

$(BUILD_DIR)/src/parser: $(YFO) $(filter-out $(LFO),$(OBJS))
	@echo + LD src/$(notdir $@) >&2
	@$(CC) -o $@ $(filter-out $(LFO),$(OBJS)) # -lfl -ly

$(BUILD_DIR)/libcompiler.a: $(YFO) $(filter-out $(LFO) $(BUILD_DIR)/src/./main.o,$(OBJS))
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

$(BUILD_DIR)/src/%.o: %.c
	@echo + CC src/$(notdir $<) >&2
	@$(CC) $(CFLAGS) -c $< -o $@ -MMD

-include $(DEPS)

.PHONY: clean all

clean:
	@rm -f *.yy.c *.tab.c *.tab.h syntax.output
# test:
# 	@./parser ../Test/test1.cmm

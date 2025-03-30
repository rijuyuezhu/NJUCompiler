#pragma once
#include "utils.h"

struct IRManager;
struct String;
struct AstNode;

typedef struct IRGenerator {
    struct IRManager *ir_manager;
    bool gen_ir_error;

    struct String *builder;
} IRGenerator;

void MTD(IRGenerator, init, /, struct IRManager *ir_manager);
FUNC_STATIC DEFAULT_DROPER(IRGenerator);

struct String MTD(IRGenerator, gen_ir, /, struct AstNode *node);

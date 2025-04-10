#pragma once
#include "utils.h"

struct IRManager;
struct TypeManager;
struct SymbolManager;
struct String;
struct AstNode;

typedef struct IRGenerator {
    struct IRManager *ir_manager;
    struct TypeManager *type_manager;
    struct SymbolManager *symbol_manager;
    bool gen_ir_error;
} IRGenerator;

void MTD(IRGenerator, init, /, struct IRManager *ir_manager,
         struct TypeManager *type_manager,
         struct SymbolManager *symbol_manager);

FUNC_STATIC DEFAULT_DROPER(IRGenerator);

struct String MTD(IRGenerator, gen_ir, /, struct AstNode *node);

#define report_genir_err(...)                                                  \
    ({                                                                         \
        printf("Cannot translate: " VA_ARGS_FIRST(                             \
            __VA_ARGS__) ".\n" VA_ARGS_EXCEPT_FIRST(__VA_ARGS__));             \
        self->gen_ir_error = true;                                             \
    })

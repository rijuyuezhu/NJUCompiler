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

#define report_genir_err(info)                                                 \
    ({                                                                         \
        printf("Cannot translate: %s.\n", info);                               \
        self->gen_ir_error = true;                                             \
    })

#define report_genir_err_fmt(format, ...)                                      \
    ({                                                                         \
        printf("Cannot translate: %s" format ".\n", ##__VA_ARGS__);            \
        self->gen_ir_error = true;                                             \
    })

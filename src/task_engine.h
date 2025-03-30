#pragma once

#include "ir.h"
#include "symbol.h"
#include "type.h"
#include "utils.h"

typedef struct TaskEngine {
    const char *input_file;
    const char *ir_file;

    // Lexical & Syntax analysis
    struct AstNode *ast_root;
    bool ast_error;

    // Semantic analysis
    TypeManager type_manager;
    SymbolManager symbol_manager;
    bool semantic_error;

    // gen ir
    IRManager ir_manager;
    bool gen_ir_error;
} TaskEngine;

void MTD(TaskEngine, init, /, const char *src_file, const char *ir_file);
void MTD(TaskEngine, drop, /);

/* Lexical & Syntax */
void MTD(TaskEngine, parse_ast, /);
void MTD(TaskEngine, print_ast, /);

/* Semantic */
void MTD(TaskEngine, analyze_semantic, /, bool add_builtin);

/* Gen IR */
void MTD(TaskEngine, gen_ir, /);

#pragma once

#include "symbol.h"
#include "type.h"
#include "utils.h"

typedef struct TaskEngine {
    const char *input_file;

    // Lexical & Syntax analysis
    struct AstNode *ast_root;
    bool ast_error;

    // Semantic analysis
    TypeManager type_manager;
    SymbolManager symbol_manager;
    bool semantic_error;
} TaskEngine;

void MTD(TaskEngine, init, /, const char *file);
void MTD(TaskEngine, drop, /);

/* Lexical & Syntax */
void MTD(TaskEngine, parse_ast, /);
void MTD(TaskEngine, print_ast, /);

/* Semantic */
void MTD(TaskEngine, analyze_semantic, /);

#pragma once

#include "ast.h"
#include "utils.h"
typedef struct TaskEngine {
    const char *input_file;

    // Lexical & Syntax analysis
    AstNode *ast_root;
    bool ast_error;

    // Semantic analysis

} TaskEngine;

void MTD(TaskEngine, init, /, const char *file);
void MTD(TaskEngine, drop, /);
void MTD(TaskEngine, parse_ast, /);
void MTD(TaskEngine, print_ast, /);

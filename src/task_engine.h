#pragma once

#include "ast.h"
#include "utils.h"
typedef struct TaskEngine {
    const char *input_file;

    AstNode *ast_root;
    bool ast_error;
} TaskEngine;

void MTD(TaskEngine, init, /, const char *file);
void MTD(TaskEngine, drop, /);
void MTD(TaskEngine, parse_ast, /);
void MTD(TaskEngine, print_ast, /);

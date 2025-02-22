#pragma once

#include "ast.h"
#include "utils.h"
typedef struct TaskEngine {
    const char *input_file;

    AstNode *ast_root;
} TaskEngine;

void MTD(TaskEngine, init, /, const char *file);
void MTD(TaskEngine, drop, /);
void MTD(TaskEngine, analyze_ast, /);
void MTD(TaskEngine, output_ast, /);

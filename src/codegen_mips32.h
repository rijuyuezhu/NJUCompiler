#pragma once
#include "str.h"
#include "utils.h"

struct IRManager;
typedef struct CodegenMips32 {
    String result;
    struct IRManager *ir_manager;
} CodegenMips32;

void MTD(CodegenMips32, init, /, struct IRManager *ir_manager);

void MTD(CodegenMips32, drop, /);

void MTD(CodegenMips32, generate, /);

void MTD(CodegenMips32, add_predefined_snippets, /);

void MTD(CodegenMips32, generate_func, /, usize start, usize end,
         usize func_cnt); // [start, end)

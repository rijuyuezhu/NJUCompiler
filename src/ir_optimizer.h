#pragma once
#include "utils.h"

struct IRProgram;
struct IRFunction;
struct IRBasicBlock;
struct IRStmtBase;
typedef struct IROptimizer {
    struct IRProgram *program;
} IROptimizer;

void MTD(IROptimizer, init, /, struct IRProgram *program);
FUNC_STATIC DEFAULT_DROPER(IROptimizer);
void MTD(IROptimizer, optimize, /);
void MTD(IROptimizer, optimize_func, /, struct IRFunction *func);
bool MTD(IROptimizer, optimize_func_const_prop, /, struct IRFunction *func);
bool MTD(IROptimizer, optimize_func_simple_redundant_ops, /,
         struct IRFunction *func);
bool MTD(IROptimizer, optimize_func_avali_exp, /, struct IRFunction *func);
bool MTD(IROptimizer, optimize_func_copy_prop, /, struct IRFunction *func);
bool MTD(IROptimizer, optimize_func_dead_code_eliminate, /,
         struct IRFunction *func);
bool MTD(IROptimizer, optimize_func_useless_label_strip, /,
         struct IRFunction *func);

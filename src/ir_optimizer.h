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
void MTD(IROptimizer, optimize_func_const_prop, /, struct IRFunction *func);
void MTD(IROptimizer, optimize_func_simple_redundant_ops, /,
         struct IRFunction *func);
void MTD(IROptimizer, optimize_func_avali_exp, /, struct IRFunction *func);

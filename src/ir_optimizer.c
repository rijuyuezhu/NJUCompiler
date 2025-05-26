#include "ir_optimizer.h"
#include "da_avaliexp.h"
#include "da_constprop.h"
#include "da_copyprop.h"
#include "da_solver.h"
#include "ir_function.h"
#include "ir_program.h"
#include "utils.h"

void MTD(IROptimizer, init, /, IRProgram *program) { self->program = program; }
void MTD(IROptimizer, optimize, /) {
    for (usize i = 0; i < self->program->functions.size; i++) {
        IRFunction *func = &self->program->functions.data[i];
        CALL(IROptimizer, *self, optimize_func, /, func);
    }
    CALL(IRProgram, *self->program, rename_var_labels, /);
}

void MTD(IROptimizer, optimize_func, /, IRFunction *func) {
    for (usize i = 0; i < 5; i++) {
        CALL(IROptimizer, *self, optimize_func_const_prop, /, func);
        CALL(IROptimizer, *self, optimize_func_simple_redundant_ops, /, func);
        CALL(IROptimizer, *self, optimize_func_copy_prop, /, func);
        CALL(IROptimizer, *self, optimize_func_avali_exp, /, func);
        CALL(IROptimizer, *self, optimize_func_copy_prop, /, func);
        CALL(IROptimizer, *self, optimize_func_dead_code_eliminate, /, func);
    }

    while (CALL(IROptimizer, *self, optimize_func_dead_code_eliminate, /, func))
        ;
    CALL(IROptimizer, *self, optimize_func_useless_label_strip, /, func);
    CALL(IROptimizer, *self, optimize_func_dead_code_eliminate, /, func);
}

bool MTD(IROptimizer, optimize_func_const_prop, /, IRFunction *func) {
    ConstPropDA const_prop = CREOBJ(ConstPropDA, /);
    NSCALL(DAWorkListSolver, solve, /, TOBASE(&const_prop), func);
    bool updated = CALL(ConstPropDA, const_prop, const_fold, /, func);
    DROPOBJ(ConstPropDA, const_prop);
    return updated;
}

bool MTD(IROptimizer, optimize_func_avali_exp, /, IRFunction *func) {
    AvaliExpDA avali_exp = CREOBJ(AvaliExpDA, /);
    CALL(AvaliExpDA, avali_exp, prepare, /, func);
    NSCALL(DAWorkListSolver, solve, /, TOBASE(&avali_exp), func);
    CALL(AvaliExpDA, avali_exp, clean_redundant_exp, /, func);
    DROPOBJ(AvaliExpDA, avali_exp);
    return CALL(IRFunction, *func, remove_dead_stmt, /);
}

bool MTD(IROptimizer, optimize_func_copy_prop, /, struct IRFunction *func) {
    CopyPropDA copy_prop = CREOBJ(CopyPropDA, /);
    NSCALL(DAWorkListSolver, solve, /, TOBASE(&copy_prop), func);
    bool updated = CALL(CopyPropDA, copy_prop, copy_propagate, /, func);
    DROPOBJ(CopyPropDA, copy_prop);
    return updated;
}

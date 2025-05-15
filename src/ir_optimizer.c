#include "ir_optimizer.h"
#include "da_constprop.h"
#include "da_solver.h"
#include "ir_function.h"
#include "ir_program.h"

void MTD(IROptimizer, init, /, IRProgram *program) { self->program = program; }
void MTD(IROptimizer, optimize, /) {
    for (usize i = 0; i < self->program->functions.size; i++) {
        IRFunction *func = &self->program->functions.data[i];
        CALL(IROptimizer, *self, optimize_func, /, func);
    }
}

void MTD(IROptimizer, optimize_func, /, IRFunction *func) {
    CALL(IROptimizer, *self, optimize_func_constprop, /, func);
}

void MTD(IROptimizer, optimize_func_constprop, /, IRFunction *func) {
    ConstPropDA const_prop = CREOBJ(ConstPropDA, /);
    NSCALL(DAWorkListSolver, solve, /, TOBASE(&const_prop), func);
    CALL(ConstPropDA, const_prop, const_fold, /, func);
    DROPOBJ(ConstPropDA, const_prop);
}

#include "ir_optimizer.h"
#include "da_avaliexp.h"
#include "da_constprop.h"
#include "da_solver.h"
#include "ir_basic_block.h"
#include "ir_function.h"
#include "ir_program.h"
#include "ir_stmt.h"
#include "utils.h"

void MTD(IROptimizer, init, /, IRProgram *program) { self->program = program; }
void MTD(IROptimizer, optimize, /) {
    for (usize i = 0; i < self->program->functions.size; i++) {
        IRFunction *func = &self->program->functions.data[i];
        CALL(IROptimizer, *self, optimize_func, /, func);
    }
}

void MTD(IROptimizer, optimize_func, /, IRFunction *func) {
    CALL(IROptimizer, *self, optimize_func_const_prop, /, func);
    CALL(IROptimizer, *self, optimize_func_simple_redundant_ops, /, func);
    CALL(IROptimizer, *self, optimize_func_avali_exp, /, func);
}

void MTD(IROptimizer, optimize_func_const_prop, /, IRFunction *func) {
    ConstPropDA const_prop = CREOBJ(ConstPropDA, /);
    NSCALL(DAWorkListSolver, solve, /, TOBASE(&const_prop), func);
    CALL(ConstPropDA, const_prop, const_fold, /, func);
    DROPOBJ(ConstPropDA, const_prop);
}

static bool MTD(IRFunction, optimize_func_simple_redundant_ops_callback, /,
                ATTR_UNUSED IRBasicBlock *bb, ListDynIRStmtNode *stmt_it,
                ATTR_UNUSED void *extra_args) {
    IRStmtBase **stmt = &stmt_it->data;
    if ((*stmt)->kind != IRStmtKindArith) {
        return false;
    }
    IRStmtArith *arith = (IRStmtArith *)*stmt;
    usize dst = arith->dst;
    ArithopKind aop = arith->aop;
    IRValue src1 = arith->src1;
    IRValue src2 = arith->src2;

    if (aop == ArithopAdd) {
        if (src1.is_const && src1.const_val == 0) {
            *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, src2);
            DROPOBJHEAP(IRStmtArith, arith);
        } else if (src2.is_const && src2.const_val == 0) {
            *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, src1);
            DROPOBJHEAP(IRStmtArith, arith);
        }
    } else if (aop == ArithopSub) {
        if (src2.is_const && src2.const_val == 0) {
            *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, src1);
            DROPOBJHEAP(IRStmtArith, arith);
        } else if (!src1.is_const && !src2.is_const && src1.var == src2.var) {
            IRValue zero = NSCALL(IRValue, from_const, /, 0);
            *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, zero);
            DROPOBJHEAP(IRStmtArith, arith);
        }
    } else if (aop == ArithopMul) {
        if (src1.is_const && src1.const_val == 1) {
            *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, src2);
            DROPOBJHEAP(IRStmtArith, arith);
        } else if (src2.is_const && src2.const_val == 1) {
            *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, src1);
            DROPOBJHEAP(IRStmtArith, arith);
        }
    } else if (aop == ArithopDiv) {
        if (src2.is_const && src2.const_val == 1) {
            *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, src1);
            DROPOBJHEAP(IRStmtArith, arith);
        } else if (!src1.is_const && !src2.is_const && src1.var == src2.var) {
            IRValue one = NSCALL(IRValue, from_const, /, 1);
            *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, one);
            DROPOBJHEAP(IRStmtArith, arith);
        }
    } else {
        PANIC("should not reach");
    }
    return false;
}

void MTD(IROptimizer, optimize_func_simple_redundant_ops, /, IRFunction *func) {
    CALL(IRFunction, *func, iter_stmt, /,
         MTDNAME(IRFunction, optimize_func_simple_redundant_ops_callback),
         NULL);
}

void MTD(IROptimizer, optimize_func_avali_exp, /, IRFunction *func) {
    AvaliExpDA avali_exp = CREOBJ(AvaliExpDA, /);
    CALL(AvaliExpDA, avali_exp, prepare, /, func);
    NSCALL(DAWorkListSolver, solve, /, TOBASE(&avali_exp), func);
    CALL(AvaliExpDA, avali_exp, clean_redundant_exp, /, func);
    CALL(IRFunction, *func, remove_dead_stmt, /);
    DROPOBJ(AvaliExpDA, avali_exp);
}

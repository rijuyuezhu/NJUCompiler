#include "ir_optimizer.h"
#include "da_constprop.h"
#include "da_solver.h"
#include "ir_basic_block.h"
#include "ir_function.h"
#include "ir_program.h"
#include "ir_stmt.h"

void MTD(IROptimizer, init, /, IRProgram *program) { self->program = program; }
void MTD(IROptimizer, optimize, /) {
    for (usize i = 0; i < self->program->functions.size; i++) {
        IRFunction *func = &self->program->functions.data[i];
        CALL(IROptimizer, *self, optimize_func, /, func);
    }
}

void MTD(IROptimizer, optimize_func, /, IRFunction *func) {
    CALL(IROptimizer, *self, optimize_func_constprop, /, func);
    CALL(IROptimizer, *self, optimize_func_simple_redundant_ops, /, func);
}

void MTD(IROptimizer, optimize_func_constprop, /, IRFunction *func) {
    ConstPropDA const_prop = CREOBJ(ConstPropDA, /);
    NSCALL(DAWorkListSolver, solve, /, TOBASE(&const_prop), func);
    CALL(ConstPropDA, const_prop, const_fold, /, func);
    DROPOBJ(ConstPropDA, const_prop);
}

void MTD(IROptimizer, iter_stmt, /, IRFunction *func,
         StmtIterCallback callback) {
    for (ListBasicBlockNode *bbit = func->basic_blocks.head; bbit;
         bbit = bbit->next) {
        IRBasicBlock *bb = &bbit->data;
        for (ListDynIRStmtNode *stmtit = bb->stmts.head; stmtit;
             stmtit = stmtit->next) {
            callback(self, func, bb, &stmtit->data);
        }
    }
}

static void MTD(IROptimizer, optimize_func_simple_redundant_ops_callback, /,
                ATTR_UNUSED IRFunction *func, ATTR_UNUSED IRBasicBlock *bb,
                IRStmtBase **stmt) {
    if ((*stmt)->kind != IRStmtKindArith) {
        return;
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
}

void MTD(IROptimizer, optimize_func_simple_redundant_ops, /, IRFunction *func) {
    CALL(IROptimizer, *self, iter_stmt, /, func,
         MTDNAME(IROptimizer, optimize_func_simple_redundant_ops_callback));
}

#include "ir_function.h"
#include "ir_optimizer.h"

static bool MTD(IRFunction, optimize_func_simple_redundant_ops_callback, /,
                ATTR_UNUSED IRBasicBlock *bb, ListDynIRStmtNode *stmt_it,
                void *extra_args) {
    IRStmtBase **stmt = &stmt_it->data;
    bool *updated = extra_args;
    if ((*stmt)->kind == IRStmtKindArith) {
        IRStmtArith *arith = (IRStmtArith *)*stmt;
        usize dst = arith->dst;
        ArithopKind aop = arith->aop;
        IRValue src1 = arith->src1;
        IRValue src2 = arith->src2;

        if (aop == ArithopAdd) {
            if (src1.is_const && src1.const_val == 0) {
                *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, src2);
                DROPOBJHEAP(IRStmtArith, arith);
                *updated = true;
            } else if (src2.is_const && src2.const_val == 0) {
                *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, src1);
                DROPOBJHEAP(IRStmtArith, arith);
                *updated = true;
            } else if (src1.is_const && src1.const_val < 0) {
                // (-#x) + src2 => src2 - #x
                arith->src1 = src2;
                arith->src2 = NSCALL(IRValue, from_const, /, -src1.const_val);
                arith->aop = ArithopSub;
                *updated = true;
            } else if (src2.is_const && src2.const_val < 0) {
                // src1 + (-#x) => src1 - #x
                arith->src2 = NSCALL(IRValue, from_const, /, -src2.const_val);
                arith->aop = ArithopSub;
                *updated = true;
            }
        } else if (aop == ArithopSub) {
            if (src2.is_const && src2.const_val == 0) {
                *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, src1);
                DROPOBJHEAP(IRStmtArith, arith);
                *updated = true;
            } else if (!src1.is_const && !src2.is_const &&
                       src1.var == src2.var) {
                IRValue zero = NSCALL(IRValue, from_const, /, 0);
                *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, zero);
                DROPOBJHEAP(IRStmtArith, arith);
                *updated = true;
            } else if (src2.is_const && src2.const_val < 0) {
                // src1 - (-#x) => src1 + #x
                arith->src2 = NSCALL(IRValue, from_const, /, -src2.const_val);
                arith->aop = ArithopAdd;
                *updated = true;
            }
        } else if (aop == ArithopMul) {
            if (src1.is_const && src1.const_val == 1) {
                *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, src2);
                DROPOBJHEAP(IRStmtArith, arith);
                *updated = true;
            } else if (src2.is_const && src2.const_val == 1) {
                *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, src1);
                DROPOBJHEAP(IRStmtArith, arith);
                *updated = true;
            } else if (src1.is_const && src1.const_val == 2) {
                // 2 * src2 => src2 + src2
                arith->src1 = src2;
                arith->aop = ArithopAdd;
                *updated = true;
            } else if (src2.is_const && src2.const_val == 2) {
                // src1 * 2 => src1 + src1
                arith->src2 = src1;
                arith->aop = ArithopAdd;
                *updated = true;
            } else if (src1.is_const && src1.const_val == -1) {
                // -1 * src2 => #0 - src2
                arith->src1 = NSCALL(IRValue, from_const, /, 0);
                arith->aop = ArithopSub;
                *updated = true;
            } else if (src2.is_const && src2.const_val == -1) {
                // src1 * -1 => #0 - src1
                arith->src2 = src1;
                arith->src1 = NSCALL(IRValue, from_const, /, 0);
                arith->aop = ArithopSub;
                *updated = true;
            } else if ((src1.is_const && src1.const_val == 0) ||
                       (src2.is_const && src2.const_val == 0)) {
                IRValue zero = NSCALL(IRValue, from_const, /, 0);
                *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, zero);
                DROPOBJHEAP(IRStmtArith, arith);
                *updated = true;
            }
        } else if (aop == ArithopDiv) {
            if (src2.is_const && src2.const_val == 1) {
                *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, src1);
                DROPOBJHEAP(IRStmtArith, arith);
                *updated = true;
            }
            if (src2.is_const && src2.const_val == -1) {
                // v / #-1 => #0 - v
                arith->src2 = src1;
                arith->src1 = NSCALL(IRValue, from_const, /, 0);
                arith->aop = ArithopSub;
                *updated = true;
            } else if (!src1.is_const && !src2.is_const &&
                       src1.var == src2.var) {
                IRValue one = NSCALL(IRValue, from_const, /, 1);
                *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, one);
                DROPOBJHEAP(IRStmtArith, arith);
                *updated = true;
            } else if (src2.is_const && src2.const_val == 0) {
                // treat division by zero as 0.
                IRValue zero = NSCALL(IRValue, from_const, /, 0);
                *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, zero);
                DROPOBJHEAP(IRStmtArith, arith);
                *updated = true;
            } else if (src1.is_const && src1.const_val == 0) {
                IRValue zero = NSCALL(IRValue, from_const, /, 0);
                *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, dst, zero);
                DROPOBJHEAP(IRStmtArith, arith);
                *updated = true;
            }
        } else {
            PANIC("should not reach");
        }
    }

    if ((*stmt)->kind == IRStmtKindAssign) {
        IRStmtAssign *assign = (IRStmtAssign *)*stmt;
        if (!assign->src.is_const) {
            usize dst = assign->dst;
            usize src = assign->src.var;
            if (dst == src) {
                CALL(ListDynIRStmt, bb->stmts, remove, /, stmt_it);
                *updated = true;
            }
        }
    }
    return false;
}

bool MTD(IROptimizer, optimize_func_simple_redundant_ops, /, IRFunction *func) {
    bool updated = false;
    CALL(IRFunction, *func, iter_stmt, /,
         MTDNAME(IRFunction, optimize_func_simple_redundant_ops_callback),
         &updated);
    return updated;
}

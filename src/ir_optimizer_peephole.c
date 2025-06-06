#include "ir_basic_block.h"
#include "ir_function.h"
#include "ir_optimizer.h"

typedef struct PeepholeEngine {
    IRFunction *func;

    bool updated;
} PeepholeEngine;

static void MTD(PeepholeEngine, init, /, IRFunction *func) {
    self->func = func;
    self->updated = false;
}

static void MTD(PeepholeEngine, drop, /) {}

static bool MTD(IRFunction, phopt_1_callback, /, ATTR_UNUSED IRBasicBlock *bb,
                ListDynIRStmtNode *stmt_it, void *extra_args) {
    if (!stmt_it->prev) {
        return false;
    }
    PeepholeEngine *engine = extra_args;
    IRStmtBase *stmt = stmt_it->data;
    IRStmtBase *last_stmt = stmt_it->prev->data;
    if (stmt->kind != IRStmtKindAssign || last_stmt->kind != IRStmtKindArith) {
        return false;
    }
    IRStmtAssign *stmt_assign = (IRStmtAssign *)stmt;
    IRStmtArith *last_stmt_arith = (IRStmtArith *)last_stmt;
    usize t = last_stmt_arith->dst;
    usize v = stmt_assign->dst;
    IRValue src = stmt_assign->src;
    if (src.is_const || src.var != t) {
        return false;
    }
    // ok!
    last_stmt_arith->dst = v;
    stmt_assign->src = (IRValue){.is_const = false, .var = v};
    stmt_assign->dst = t;
    engine->updated = true;
    return false;
}

// handle cases like:
// t := v + 1
// v := t
//
// =>
//
// v := v + 1
// t := v
//
// (copy propagation & dce later)
static void MTD(PeepholeEngine, phopt_1, /) {
    CALL(IRFunction, *self->func, iter_stmt, /,
         MTDNAME(IRFunction, phopt_1_callback), self);
}

// d = s + c
static bool resolve_add_sub(IRStmtBase *stmt, usize *d, usize *s, int *c) {
    if (stmt->kind != IRStmtKindArith) {
        return false;
    }
    IRStmtArith *arith = (IRStmtArith *)stmt;
    ArithopKind aop = arith->aop;
    usize dst = arith->dst;
    IRValue src1 = arith->src1;
    IRValue src2 = arith->src2;
    if (aop == ArithopAdd) {
        if (!src1.is_const && src2.is_const) {
            // dst = src1 + #src2
            *d = dst;
            *s = src1.var;
            *c = src2.const_val;
            return true;
        } else if (src1.is_const && !src2.is_const) {
            // dst = src2 + #src1
            *d = dst;
            *s = src2.var;
            *c = src1.const_val;
            return true;
        } else {
            return false;
        }
    } else if (aop == ArithopSub) {
        if (!src1.is_const && src2.is_const) {
            // dst = src1 - #src2
            *d = dst;
            *s = src1.var;
            *c = -src2.const_val;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

static void MTD(PeepholeEngine, phopt_2_add_sub, /, ListDynIRStmtNode *now) {
    usize d;
    usize s;
    int c;
    if (!resolve_add_sub(now->data, &d, &s, &c)) {
        return;
    }
    if (d == s) {
        return;
    }
    for (ListDynIRStmtNode *it = now->next; it; it = it->next) {
        IRStmtBase *stmt = it->data;
        usize d2;
        usize s2;
        int c2;
        if (resolve_add_sub(stmt, &d2, &s2, &c2)) {
            if (s2 == d) {
                // d = s + c
                // ...
                // d2 = s2(d) + c2
                //
                // => d2 = s + (c + c2)
                IRValue src1 = NSCALL(IRValue, from_var, /, s);
                IRValue src2 = NSCALL(IRValue, from_const, /, c + c2);
                IRStmtBase *old_stmt = stmt;
                stmt = (IRStmtBase *)CREOBJHEAP(IRStmtArith, /, d2, src1, src2,
                                                ArithopAdd);
                it->data = stmt;
                VDROPOBJHEAP(IRStmtBase, old_stmt);
            }
        }
        usize def = VCALL(IRStmtBase, *stmt, get_def, /);
        if (def != (usize)-1 && (def == d || def == s)) {
            // redef; stop here
            return;
        }
    }
}

// d = s * c
static bool resolve_mul(IRStmtBase *stmt, usize *d, usize *s, int *c) {
    if (stmt->kind != IRStmtKindArith) {
        return false;
    }
    IRStmtArith *arith = (IRStmtArith *)stmt;
    ArithopKind aop = arith->aop;
    usize dst = arith->dst;
    IRValue src1 = arith->src1;
    IRValue src2 = arith->src2;
    if (aop == ArithopMul) {
        if (!src1.is_const && src2.is_const) {
            // dst = src1 * #src2
            *d = dst;
            *s = src1.var;
            *c = src2.const_val;
            return true;
        } else if (src1.is_const && !src2.is_const) {
            // dst = src2 * #src1
            *d = dst;
            *s = src2.var;
            *c = src1.const_val;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

static void MTD(PeepholeEngine, phopt_2_mul, /, ListDynIRStmtNode *now) {
    usize d;
    usize s;
    int c;
    if (!resolve_mul(now->data, &d, &s, &c)) {
        return;
    }
    if (d == s) {
        return;
    }
    for (ListDynIRStmtNode *it = now->next; it; it = it->next) {
        IRStmtBase *stmt = it->data;
        usize d2;
        usize s2;
        int c2;
        if (resolve_mul(stmt, &d2, &s2, &c2)) {
            if (s2 == d) {
                // d = s * c
                // ...
                // d2 = s2(d) * c2
                //
                // => d2 = s * (c * c2)
                IRValue src1 = NSCALL(IRValue, from_var, /, s);
                IRValue src2 = NSCALL(IRValue, from_const, /, c * c2);
                IRStmtBase *old_stmt = stmt;
                stmt = (IRStmtBase *)CREOBJHEAP(IRStmtArith, /, d2, src1, src2,
                                                ArithopMul);
                it->data = stmt;
                VDROPOBJHEAP(IRStmtBase, old_stmt);
            }
        }
        usize def = VCALL(IRStmtBase, *stmt, get_def, /);
        if (def != (usize)-1 && (def == d || def == s)) {
            // redef; stop here
            return;
        }
    }
}

// handle concecutive arithmetic operations
static void MTD(PeepholeEngine, phopt_2, /) {
    IRFunction *func = self->func;
    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        for (ListDynIRStmtNode *stmt_it = bb->stmts.head; stmt_it;
             stmt_it = stmt_it->next) {
            CALL(PeepholeEngine, *self, phopt_2_add_sub, /, stmt_it);
        }
    }
    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        for (ListDynIRStmtNode *stmt_it = bb->stmts.head; stmt_it;
             stmt_it = stmt_it->next) {
            CALL(PeepholeEngine, *self, phopt_2_mul, /, stmt_it);
        }
    }
}

static void MTD(PeepholeEngine, optimize, /) {
    CALL(PeepholeEngine, *self, phopt_1, /);
    CALL(PeepholeEngine, *self, phopt_2, /);
}

bool MTD(IROptimizer, optimize_func_peephole, /, IRFunction *func) {
    PeepholeEngine engine = CREOBJ(PeepholeEngine, /, func);
    CALL(PeepholeEngine, engine, optimize, /);
    bool updated = engine.updated;
    DROPOBJ(PeepholeEngine, engine);
    return updated;
}

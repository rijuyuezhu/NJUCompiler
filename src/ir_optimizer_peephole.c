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

static void MTD(PeepholeEngine, optimize, /) {
    CALL(PeepholeEngine, *self, phopt_1, /);
}

bool MTD(IROptimizer, optimize_func_peephole, /, IRFunction *func) {
    PeepholeEngine engine = CREOBJ(PeepholeEngine, /, func);
    CALL(PeepholeEngine, engine, optimize, /);
    bool updated = engine.updated;
    DROPOBJ(PeepholeEngine, engine);
    return updated;
}

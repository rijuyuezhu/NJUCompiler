#include "ir_function.h"
#include "ir_optimizer.h"
#include "ir_stmt.h"

typedef struct PeepholeEngine {
    IRFunction *func;

    MapUSizeToSetPtr var_to_defs;
    MapUSizeToSetPtr var_to_uses;

    bool updated;
} PeepholeEngine;

static void MTD(PeepholeEngine, init, /, IRFunction *func) {
    self->func = func;
    CALL(MapUSizeToSetPtr, self->var_to_defs, init, /);
    CALL(MapUSizeToSetPtr, self->var_to_uses, init, /);
    self->updated = false;
}

static void MTD(PeepholeEngine, drop, /) {
    DROPOBJ(MapUSizeToSetPtr, self->var_to_uses);
    DROPOBJ(MapUSizeToSetPtr, self->var_to_defs);
}

static bool MTD(IRFunction, collect_info_callback, /,
                ATTR_UNUSED IRBasicBlock *bb, ListDynIRStmtNode *stmt_it,
                void *extra_args) {
    IRStmtBase *stmt = stmt_it->data;
    PeepholeEngine *engine = extra_args;
    usize def = VCALL(IRStmtBase, *stmt, get_def, /);
    if (def != (usize)-1) {
        MapUSizeToSetPtrIterator defstmts_iter =
            CALL(MapUSizeToSetPtr, engine->var_to_defs, find_owned, /, def);
        if (!defstmts_iter) {
            SetPtr empty_defstmts = CREOBJ(SetPtr, /);
            MapUSizeToSetPtrInsertResult res =
                CALL(MapUSizeToSetPtr, engine->var_to_defs, insert, /, def,
                     empty_defstmts);
            ASSERT(res.inserted);
            defstmts_iter = res.node;
        }
        SetPtr *defstmts = &defstmts_iter->value;
        CALL(SetPtr, *defstmts, insert, /, stmt, ZERO_SIZE);
    }
    SliceIRValue uses = VCALL(IRStmtBase, *stmt, get_use, /);
    for (usize i = 0; i < uses.size; i++) {
        usize use = uses.data[i].var;
        MapUSizeToSetPtrIterator usestmts_iter =
            CALL(MapUSizeToSetPtr, engine->var_to_uses, find_owned, /, use);
        if (!usestmts_iter) {
            SetPtr empty_usestmts = CREOBJ(SetPtr, /);
            MapUSizeToSetPtrInsertResult res =
                CALL(MapUSizeToSetPtr, engine->var_to_uses, insert, /, use,
                     empty_usestmts);
            ASSERT(res.inserted);
            usestmts_iter = res.node;
        }
        SetPtr *usestmts = &usestmts_iter->value;
        CALL(SetPtr, *usestmts, insert, /, stmt, ZERO_SIZE);
    }
    return false;
}
static void MTD(PeepholeEngine, collect_info, /) {
    IRFunction *func = self->func;
    CALL(IRFunction, *func, iter_stmt, /,
         MTDNAME(IRFunction, collect_info_callback), self);
}

static void MTD(PeepholeEngine, prepare, /) {
    CALL(PeepholeEngine, *self, collect_info, /);
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
// (copy propagation later)
static bool MTD(IRFunction, phopt_1, /, ATTR_UNUSED IRBasicBlock *bb,
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

static void MTD(PeepholeEngine, optimize, /) {
    CALL(IRFunction, *self->func, iter_stmt, /, MTDNAME(IRFunction, phopt_1),
         self);
}

bool MTD(IROptimizer, optimize_func_peephole, /, IRFunction *func) {
    PeepholeEngine engine = CREOBJ(PeepholeEngine, /, func);
    CALL(PeepholeEngine, engine, prepare, /);
    CALL(PeepholeEngine, engine, optimize, /);
    bool updated = engine.updated;
    DROPOBJ(PeepholeEngine, engine);
    return updated;
}

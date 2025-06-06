#include "ir_program.h"
#include "loop_opt.h"

typedef struct DerivedIndVarRec {
    // key = k * base_ind + b; replaced with key = derived_copy
    usize key;
    usize base_ind;
    usize derived_copy;
    int k;
    int b;
} DerivedIndVarRec;

DECLARE_MAPPING(MapVarToDerivedInfo, usize, DerivedIndVarRec, FUNC_STATIC,
                GENERATOR_PLAIN_KEY, GENERATOR_PLAIN_VALUE,
                GENERATOR_PLAIN_COMPARATOR);

typedef struct LoopIndVar {
    LoopOpt *loop_opt;
    LoopInfo *loop_info;

    // in-loop information
    MapUSizeToSetPtr var_to_defs;
    MapUSizeToSetPtr var_to_uses;

    // ind var
    MapVarToDerivedInfo var_to_derived_info;
    MapUSizeToSetUSize base_family;
} LoopIndVar;

static void MTD(LoopIndVar, init, /, LoopOpt *loop_opt, LoopInfo *loop_info) {
    self->loop_opt = loop_opt;
    self->loop_info = loop_info;
    CALL(MapUSizeToSetPtr, self->var_to_defs, init, /);
    CALL(MapUSizeToSetPtr, self->var_to_uses, init, /);
    CALL(MapVarToDerivedInfo, self->var_to_derived_info, init, /);
    CALL(MapUSizeToSetUSize, self->base_family, init, /);
}

static void MTD(LoopIndVar, drop, /) {
    DROPOBJ(MapUSizeToSetUSize, self->base_family);
    DROPOBJ(MapVarToDerivedInfo, self->var_to_derived_info);
    DROPOBJ(MapUSizeToSetPtr, self->var_to_defs);
    DROPOBJ(MapUSizeToSetPtr, self->var_to_uses);
}

static void MTD(LoopIndVar, add_info, /, IRStmtBase *stmt) {
    usize def = VCALL(IRStmtBase, *stmt, get_def, /);
    if (def != (usize)-1) {
        MapUSizeToSetPtrIterator defstmts_iter =
            CALL(MapUSizeToSetPtr, self->var_to_defs, find_owned, /, def);
        if (!defstmts_iter) {
            SetPtr empty_defstmts = CREOBJ(SetPtr, /);
            MapUSizeToSetPtrInsertResult res =
                CALL(MapUSizeToSetPtr, self->var_to_defs, insert, /, def,
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
            CALL(MapUSizeToSetPtr, self->var_to_uses, find_owned, /, use);
        if (!usestmts_iter) {
            SetPtr empty_usestmts = CREOBJ(SetPtr, /);
            MapUSizeToSetPtrInsertResult res =
                CALL(MapUSizeToSetPtr, self->var_to_uses, insert, /, use,
                     empty_usestmts);
            ASSERT(res.inserted);
            usestmts_iter = res.node;
        }
        SetPtr *usestmts = &usestmts_iter->value;
        CALL(SetPtr, *usestmts, insert, /, stmt, ZERO_SIZE);
    }
}

static void MTD(LoopIndVar, collect_info, /) {
    // var_to_defs, var_to_uses
    LoopInfo *loop_info = self->loop_info;
    for (SetPtrIterator it = CALL(SetPtr, loop_info->nodes, begin, /); it;
         it = CALL(SetPtr, loop_info->nodes, next, /, it)) {
        IRBasicBlock *bb = it->key;
        for (ListDynIRStmtNode *it_stmt = bb->stmts.head; it_stmt;
             it_stmt = it_stmt->next) {
            IRStmtBase *stmt = it_stmt->data;
            CALL(LoopIndVar, *self, add_info, /, stmt);
        }
    }
}

static bool MTD(LoopIndVar, is_stride_stmt, /, IRStmtBase *stmt, int *stride) {
    if (stmt->kind != IRStmtKindArith) {
        return false;
    }
    IRStmtArith *arith_stmt = (IRStmtArith *)stmt;
    usize dst = arith_stmt->dst;
    IRValue src1 = arith_stmt->src1;
    IRValue src2 = arith_stmt->src2;
    if (arith_stmt->aop == ArithopAdd) {
        if (!src1.is_const && dst == src1.var && src2.is_const) {
            // x = x + c
            *stride = src2.const_val;
            return true;
        } else if (!src2.is_const && dst == src2.var && src1.is_const) {
            // x = c + x
            *stride = src1.const_val;
            return true;
        } else {
            return false;
        }
    } else if (arith_stmt->aop == ArithopSub) {
        if (!src1.is_const && dst == src1.var && src2.is_const) {
            // x = x - c
            *stride = -src2.const_val;
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}
static void MTD(LoopIndVar, find_base_ind_vars, /) {
    for (MapUSizeToSetPtrIterator it =
             CALL(MapUSizeToSetPtr, self->var_to_defs, begin, /);
         it; it = CALL(MapUSizeToSetPtr, self->var_to_defs, next, /, it)) {
        usize def = it->key;
        SetPtr *defstmts = &it->value;
        bool is_base_ind_var = true;
        for (SetPtrIterator def_it = CALL(SetPtr, *defstmts, begin, /); def_it;
             def_it = CALL(SetPtr, *defstmts, next, /, def_it)) {
            IRStmtBase *stmt = def_it->key;
            int stride;
            bool is_stride_stmt =
                CALL(LoopIndVar, *self, is_stride_stmt, /, stmt, &stride);
            if (!is_stride_stmt) {
                is_base_ind_var = false;
                break;
            }
        }
        if (is_base_ind_var) {
            SetUSize empty_family = CREOBJ(SetUSize, /);
            MapUSizeToSetUSizeInsertResult res =
                CALL(MapUSizeToSetUSize, self->base_family, insert, /, def,
                     empty_family);
            ASSERT(res.inserted);
        }
    }
}

static void MTD(LoopIndVar, collect_base_ind_var, /) {
    CALL(LoopIndVar, *self, collect_info, /);
    CALL(LoopIndVar, *self, find_base_ind_vars, /);
}

static bool MTD(LoopIndVar, is_derive_stmt, /, IRStmtBase *stmt,
                DerivedIndVarRec *rec) {
    if (stmt->kind != IRStmtKindArith) {
        return false;
    }
    IRStmtArith *arith_stmt = (IRStmtArith *)stmt;
    usize dst = arith_stmt->dst;
    IRValue src1 = arith_stmt->src1;
    IRValue src2 = arith_stmt->src2;
    ArithopKind op = arith_stmt->aop;
    bool possible = false;
    if (op == ArithopAdd) {
        if (!src1.is_const && src2.is_const) {
            // key = 1 * src1 + #src2
            rec->key = dst;
            rec->base_ind = src1.var;
            rec->k = 1;
            rec->b = src2.const_val;
            possible = true;
        } else if (!src2.is_const && src1.is_const) {
            // key = 1 * src2 + #src1
            rec->key = dst;
            rec->base_ind = src2.var;
            rec->k = 1;
            rec->b = src1.const_val;
            possible = true;
        }
    } else if (op == ArithopSub) {
        if (!src1.is_const && src2.is_const) {
            // key = 1 * src1 - #src2
            rec->key = dst;
            rec->base_ind = src1.var;
            rec->k = 1;
            rec->b = -src2.const_val;
            possible = true;
        } else if (!src2.is_const && src1.is_const) {
            // key = -1 * src2 + #src1
            rec->key = dst;
            rec->base_ind = src2.var;
            rec->k = -1;
            rec->b = src1.const_val;
            possible = true;
        }
    } else if (op == ArithopMul) {
        if (!src1.is_const && src2.is_const) {
            // key = #src2 * src1 + 0
            rec->key = dst;
            rec->base_ind = src1.var;
            rec->k = src2.const_val;
            rec->b = 0;
            possible = true;
        } else if (!src2.is_const && src1.is_const) {
            // key = #src1 * src2 + 0
            rec->key = dst;
            rec->base_ind = src2.var;
            rec->k = src1.const_val;
            rec->b = 0;
            possible = true;
        }
    }
    if (!possible) {
        return false;
    }
    usize key = rec->key;
    MapUSizeToSetUSizeIterator key_base_family_it =
        CALL(MapUSizeToSetUSize, self->base_family, find_owned, /, key);
    if (key_base_family_it) {
        return false;
    }
    usize base_ind = rec->base_ind;
    MapUSizeToSetUSizeIterator base_family_it =
        CALL(MapUSizeToSetUSize, self->base_family, find_owned, /, base_ind);
    if (!base_family_it) {
        return false;
    }
    return true;
}

static void VMTD(ReachDefDA, find_derived_ind_vars_callback, /,
                 ListDynIRStmtNode *iter, Any fact, void *extra_args) {
    LoopIndVar *engine = extra_args;
    LoopInfo *loop_info = engine->loop_info;
    RDFact *rd_fact = fact;
    IRStmtBase *stmt = iter->data;
    SliceIRValue uses = VCALL(IRStmtBase, *stmt, get_use, /);
    for (usize i = 0; i < uses.size; i++) {
        IRValue use = uses.data[i];
        if (use.is_const) {
            continue;
        }
        usize use_var = use.var;
        SetPtr *reach_defs = CALL(RDFact, *rd_fact, get, /, use_var);
        MapVarToDerivedInfoIterator derived_it =
            CALL(MapVarToDerivedInfo, engine->var_to_derived_info, find_owned,
                 /, use_var);
        if (!derived_it) {
            // not a derived ind var, skip
            continue;
        }
        bool shall_evict = false;
        for (SetPtrIterator it = CALL(SetPtr, *reach_defs, begin, /); it;
             it = CALL(SetPtr, *reach_defs, next, /, it)) {
            IRStmtBase *reach_def = it->key;
            IRBasicBlock *reach_def_bb =
                CALL(LoopOpt, *engine->loop_opt, get_bb, /, reach_def);
            SetPtrIterator def_in_loop_it =
                CALL(SetPtr, loop_info->nodes, find_owned, /, reach_def_bb);
            if (!def_in_loop_it) {
                shall_evict = true;
                break;
            }
        }
        if (shall_evict) {
            CALL(MapVarToDerivedInfo, engine->var_to_derived_info, erase, /,
                 derived_it);
            MapUSizeToSetPtrIterator defstmts_it = CALL(
                MapUSizeToSetPtr, engine->var_to_defs, find_owned, /, use_var);
            ASSERT(defstmts_it);
            SetPtr *defstmts = &defstmts_it->value;
            ASSERT(defstmts->size == 1);
            IRStmtBase *def_stmt = CALL(SetPtr, *defstmts, begin, /)->key;
            ASSERT(def_stmt->is_dead);
            def_stmt->is_dead = false;
        }
    }
}
static void MTD(LoopIndVar, find_derived_ind_vars, /) {
    // find those statements that are derived from base ind vars
    for (MapUSizeToSetPtrIterator it =
             CALL(MapUSizeToSetPtr, self->var_to_defs, begin, /);
         it; it = CALL(MapUSizeToSetPtr, self->var_to_defs, next, /, it)) {
        usize def = it->key;
        SetPtr *defstmts = &it->value;
        if (defstmts->size != 1) {
            // not a single definition, skip
            continue;
        }
        IRStmtBase *stmt = CALL(SetPtr, *defstmts, begin, /)->key;
        DerivedIndVarRec derived_rec;
        bool is_derived_stmt =
            CALL(LoopIndVar, *self, is_derive_stmt, /, stmt, &derived_rec);
        if (is_derived_stmt) {
            if (stmt->is_dead) {
                // this derived ind var is eliminated in other loops.
                continue;
            }
            stmt->is_dead = true;

            ASSERT(derived_rec.key == def);
            derived_rec.derived_copy = CALL(
                IdxAllocator, self->loop_opt->func->program->var_idx_allocator,
                allocate, /);
            MapVarToDerivedInfoInsertResult res =
                CALL(MapVarToDerivedInfo, self->var_to_derived_info, insert, /,
                     def, derived_rec);
            ASSERT(res.inserted);
        }
    }

    // filter those derived ind vars whose uses are not reached by defs outside
    // the loop
    ReachDefDA *reach_def_da = &self->loop_opt->reach_def_da;
    LoopInfo *loop_info = self->loop_info;
    for (SetPtrIterator it = CALL(SetPtr, loop_info->nodes, begin, /); it;
         it = CALL(SetPtr, loop_info->nodes, next, /, it)) {
        IRBasicBlock *bb = it->key;
        CALL(DataflowAnalysisBase, *TOBASE(reach_def_da), iter_bb, /, bb,
             MTDNAME(ReachDefDA, find_derived_ind_vars_callback), self);
    }

    // now we have all the derived ind vars, we can build the base family
    for (MapVarToDerivedInfoIterator it =
             CALL(MapVarToDerivedInfo, self->var_to_derived_info, begin, /);
         it; it = CALL(MapVarToDerivedInfo, self->var_to_derived_info, next, /,
                       it)) {
        DerivedIndVarRec *rec = &it->value;
        usize base_ind = rec->base_ind;
        MapUSizeToSetUSizeIterator base_family_it = CALL(
            MapUSizeToSetUSize, self->base_family, find_owned, /, base_ind);
        ASSERT(base_family_it);
        SetUSize *derives = &base_family_it->value;
        CALL(SetUSize, *derives, insert, /, rec->key, ZERO_SIZE);
    }
}

static void MTD(LoopIndVar, add_in_preheader, /, DerivedIndVarRec *rec) {
    LoopInfo *loop_info = self->loop_info;
    CALL(LoopInfo, *loop_info, ensure_preheader, /);
    IRBasicBlock *preheader = loop_info->preheader;
    ASSERT(preheader);

    // derive = base_ind * k
    // derive = derive + b
    IRValue src1 = NSCALL(IRValue, from_var, /, rec->base_ind);
    IRValue src2 = NSCALL(IRValue, from_const, /, rec->k);
    IRStmtBase *stmt = (IRStmtBase *)CREOBJHEAP(
        IRStmtArith, /, rec->derived_copy, src1, src2, ArithopMul);
    CALL(IRBasicBlock, *preheader, add_stmt, /, stmt);

    src1 = NSCALL(IRValue, from_var, /, rec->derived_copy);
    src2 = NSCALL(IRValue, from_const, /, rec->b);
    stmt = (IRStmtBase *)CREOBJHEAP(IRStmtArith, /, rec->derived_copy, src1,
                                    src2, ArithopAdd);
    CALL(IRBasicBlock, *preheader, add_stmt, /, stmt);
}

static bool MTD(IRBasicBlock, apply_ind_var_callback, /,
                ListDynIRStmtNode *stmt_it, void *extra_args) {
    LoopIndVar *engine = extra_args;
    IRStmtBase *stmt = stmt_it->data;

    if (stmt->kind != IRStmtKindArith) {
        return false;
    }
    IRStmtArith *arith_stmt = (IRStmtArith *)stmt;
    usize def = arith_stmt->dst;

    // first, it may be a base ind var
    MapUSizeToSetUSizeIterator base_family_it =
        CALL(MapUSizeToSetUSize, engine->base_family, find_owned, /, def);
    if (base_family_it) {
        int stride;
        bool is_stride_stmt =
            CALL(LoopIndVar, *engine, is_stride_stmt, /, stmt, &stride);
        ASSERT(is_stride_stmt);

        SetUSize *derives = &base_family_it->value;
        if (derives->size == 0) {
            return false;
        }
        for (SetUSizeIterator it = CALL(SetUSize, *derives, begin, /); it;
             it = CALL(SetUSize, *derives, next, /, it)) {
            usize derived_var = it->key;
            MapVarToDerivedInfoIterator derive_it =
                CALL(MapVarToDerivedInfo, engine->var_to_derived_info,
                     find_owned, /, derived_var);
            ASSERT(derive_it);
            DerivedIndVarRec *derived_info = &derive_it->value;
            int derived_stride = derived_info->k * stride;
            // add derived_copy = derived_copy + derived_stride
            IRValue src1 =
                NSCALL(IRValue, from_var, /, derived_info->derived_copy);
            IRValue src2 = NSCALL(IRValue, from_const, /, derived_stride);
            IRStmtBase *new_stmt = (IRStmtBase *)CREOBJHEAP(
                IRStmtArith, /, derived_info->derived_copy, src1, src2,
                ArithopAdd);
            CALL(ListDynIRStmt, self->stmts, insert_after, /, stmt_it,
                 new_stmt);
        }
        return true;
    }

    // second, it may be a derived ind var
    MapVarToDerivedInfoIterator derived_it = CALL(
        MapVarToDerivedInfo, engine->var_to_derived_info, find_owned, /, def);
    if (derived_it) {
        ASSERT(stmt->is_dead);
        DerivedIndVarRec *derived_info = &derived_it->value;
        IRValue src = NSCALL(IRValue, from_var, /, derived_info->derived_copy);
        IRStmtBase *new_stmt =
            (IRStmtBase *)CREOBJHEAP(IRStmtAssign, /, derived_info->key, src);
        IRStmtBase *tmp = stmt_it->data;
        stmt_it->data = new_stmt;
        VCALL(IRStmtBase, *tmp, drop, /);
    }
    return false;
}

static bool MTD(LoopIndVar, apply_ind_var, /) {
    for (MapVarToDerivedInfoIterator it =
             CALL(MapVarToDerivedInfo, self->var_to_derived_info, begin, /);
         it; it = CALL(MapVarToDerivedInfo, self->var_to_derived_info, next, /,
                       it)) {
        DerivedIndVarRec *rec = &it->value;
        CALL(LoopIndVar, *self, add_in_preheader, /, rec);
    }

    LoopInfo *loop_info = self->loop_info;
    for (SetPtrIterator it = CALL(SetPtr, loop_info->nodes, begin, /); it;
         it = CALL(SetPtr, loop_info->nodes, next, /, it)) {
        IRBasicBlock *bb = it->key;
        CALL(IRBasicBlock, *bb, iter_stmt, /,
             MTDNAME(IRBasicBlock, apply_ind_var_callback), self);
    }

    return self->var_to_derived_info.size > 0;
}

static void MTD(LoopOpt, init_ind_var_engine, /) {
    for (usize i = 0; i < self->loop_infos_ordered.size; i++) {
        LoopInfo *loop_info = self->loop_infos_ordered.data[i];
        LoopIndVar *engine = CREOBJHEAP(LoopIndVar, /, self, loop_info);
        loop_info->aid_engine = engine;
    }
}

static void MTD(LoopOpt, collect_base_ind_var, /) {
    for (usize i = 0; i < self->loop_infos_ordered.size; i++) {
        LoopInfo *loop_info = self->loop_infos_ordered.data[i];
        LoopIndVar *engine = loop_info->aid_engine;
        CALL(LoopIndVar, *engine, collect_base_ind_var, /);
    }
}

static bool MTD(LoopOpt, apply_ind_var, /) {
    for (usize i = 0; i < self->loop_infos_ordered.size; i++) {
        LoopInfo *loop_info = self->loop_infos_ordered.data[i];
        LoopIndVar *engine = loop_info->aid_engine;
        CALL(LoopIndVar, *engine, find_derived_ind_vars, /);
    }
    bool updated = false;
    for (usize i = 0; i < self->loop_infos_ordered.size; i++) {
        LoopInfo *loop_info = self->loop_infos_ordered.data[i];
        LoopIndVar *engine = loop_info->aid_engine;
        updated = CALL(LoopIndVar, *engine, apply_ind_var, /) || updated;
    }
    return updated;
}

static void MTD(LoopOpt, drop_ind_var_engine, /) {
    for (usize i = 0; i < self->loop_infos_ordered.size; i++) {
        LoopInfo *loop_info = self->loop_infos_ordered.data[i];
        LoopIndVar *engine = loop_info->aid_engine;
        DROPOBJHEAP(LoopIndVar, engine);
        loop_info->aid_engine = NULL;
    }
}

bool MTD(LoopOpt, induction_var_optimize, /) {
    CALL(LoopOpt, *self, init_ind_var_engine, /);
    CALL(LoopOpt, *self, collect_base_ind_var, /);
    bool updated = CALL(LoopOpt, *self, apply_ind_var, /);
    CALL(LoopOpt, *self, fix_preheader, /);
    CALL(LoopOpt, *self, drop_ind_var_engine, /);
    return updated;
}

DEFINE_MAPPING(MapVarToDerivedInfo, usize, DerivedIndVarRec, FUNC_STATIC);

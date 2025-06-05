#include "ir_function.h"
#include "loop_opt.h"

typedef struct LICMEngine {
    LoopOpt *loop_opt;
    LoopInfo *loop_info;

    // in-loop information
    MapUSizeToSetPtr var_to_defs;
    MapUSizeToSetPtr var_to_uses;
    DomFact *dom_all_exit;

    // temps used to find invariants
    // invariant transition graph:
    // (a -> b): a is invaraint => b (possibly) is invariant
    MapPtrToVecPtr itg_succ;
    MapPtrUSize itg_indeg;
    VecPtr itg_worklist;

    VecPtr invariants; //  of IRStmtBase *
    ListPtr motions;
} LICMEngine;

static void MTD(LICMEngine, init, /, LoopOpt *loop_opt, LoopInfo *loop_info) {
    self->loop_opt = loop_opt;
    self->loop_info = loop_info;
    CALL(MapUSizeToSetPtr, self->var_to_defs, init, /);
    CALL(MapUSizeToSetPtr, self->var_to_uses, init, /);
    self->dom_all_exit = NULL;
    CALL(MapPtrToVecPtr, self->itg_succ, init, /);
    CALL(MapPtrUSize, self->itg_indeg, init, /);
    CALL(VecPtr, self->itg_worklist, init, /);
    CALL(VecPtr, self->invariants, init, /);
    CALL(ListPtr, self->motions, init, /);
}

static void MTD(LICMEngine, drop, /) {
    DROPOBJ(ListPtr, self->motions);
    DROPOBJ(VecPtr, self->invariants);
    DROPOBJ(VecPtr, self->itg_worklist);
    DROPOBJ(MapPtrUSize, self->itg_indeg);
    DROPOBJ(MapPtrToVecPtr, self->itg_succ);
    DROPOBJHEAP(DomFact, self->dom_all_exit);
    DROPOBJ(MapUSizeToSetPtr, self->var_to_uses);
    DROPOBJ(MapUSizeToSetPtr, self->var_to_defs);
}

static void MTD(LICMEngine, add_info, /, IRStmtBase *stmt) {
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

static void MTD(LICMEngine, collect_info, /) {
    // var_to_defs, var_to_uses
    LoopInfo *loop_info = self->loop_info;
    for (SetPtrIterator it = CALL(SetPtr, loop_info->nodes, begin, /); it;
         it = CALL(SetPtr, loop_info->nodes, next, /, it)) {
        IRBasicBlock *bb = it->key;
        for (ListDynIRStmtNode *it_stmt = bb->stmts.head; it_stmt;
             it_stmt = it_stmt->next) {
            IRStmtBase *stmt = it_stmt->data;
            CALL(LICMEngine, *self, add_info, /, stmt);
        }
    }

    LoopOpt *loop_opt = self->loop_opt;
    DominatorDA *dom_da = &loop_opt->dom_da;

    self->dom_all_exit = CALL(DominatorDA, *dom_da, new_initial_fact, /);

    for (usize i = 0; i < loop_info->exits.size; i++) {
        IRBasicBlock *exit = loop_info->exits.data[i];
        DomFact *exit_fact = CALL(DominatorDA, *dom_da, get_out_fact, /, exit);
        CALL(DominatorDA, *dom_da, meet_into, /, exit_fact, self->dom_all_exit);
    }
}

static void VMTD(ReachDefDA, find_invariants_callback, /,
                 ListDynIRStmtNode *iter, Any fact, void *extra_args) {
    LICMEngine *engine = extra_args;
    LoopInfo *loop_info = engine->loop_info;
    RDFact *rd_fact = fact;
    IRStmtBase *stmt = iter->data;
    if (stmt->kind != IRStmtKindAssign && stmt->kind != IRStmtKindArith) {
        return;
    }
    SliceIRValue uses = VCALL(IRStmtBase, *stmt, get_use, /);

    bool is_invariant = true;
    for (usize i = 0; i < uses.size; i++) {
        IRValue use = uses.data[i];
        if (use.is_const) {
            continue;
        }
        usize require = use.var;
        SetPtr *reach_defs = CALL(RDFact, *rd_fact, get, /, require);
        IRStmtBase *inside_loop_def = NULL;
        for (SetPtrIterator it = CALL(SetPtr, *reach_defs, begin, /); it;
             it = CALL(SetPtr, *reach_defs, next, /, it)) {
            IRStmtBase *reach_def = it->key;
            IRBasicBlock *reach_def_bb =
                CALL(LoopOpt, *engine->loop_opt, get_bb, /, reach_def);
            SetPtrIterator def_in_loop_it =
                CALL(SetPtr, loop_info->nodes, find_owned, /, reach_def_bb);
            if (def_in_loop_it) {
                inside_loop_def = reach_def;
            }
        }

        if (inside_loop_def) {
            is_invariant = false;

            if (reach_defs->size == 1) {
                // only a inner-loop is reachable
                // add an edge from inside_loop to stmt;
                // add into succ (inside_loop_def -> stmt)
                MapPtrToVecPtrIterator it =
                    CALL(MapPtrToVecPtr, engine->itg_succ, find_owned, /,
                         inside_loop_def);
                if (!it) {
                    VecPtr vec = CREOBJ(VecPtr, /);
                    MapPtrToVecPtrInsertResult res =
                        CALL(MapPtrToVecPtr, engine->itg_succ, insert, /,
                             inside_loop_def, vec);
                    ASSERT(res.inserted);
                    it = res.node;
                }
                VecPtr *succs = &it->value;
                CALL(VecPtr, *succs, push_back, /, stmt);

            } else {
                // hack below to prevent the stmt from being invariant
            }

            // in both cases, we add an in-deg for stmt.
            // For the former, the pred becomes invariant helps
            // the indeg decreasing by 1;
            // for the latter, the self-loop prevents the stmt
            // from being invariant.
            MapPtrUSizeIterator indeg_it =
                CALL(MapPtrUSize, engine->itg_indeg, find_owned, /, stmt);
            if (!indeg_it) {
                MapPtrUSizeInsertResult res =
                    CALL(MapPtrUSize, engine->itg_indeg, insert, /, stmt, 0);
                ASSERT(res.inserted);
                indeg_it = res.node;
            }
            indeg_it->value++;
        }
    }
    if (is_invariant) {
        CALL(VecPtr, engine->itg_worklist, push_back, /, stmt);
    }
}

static void MTD(LICMEngine, find_invariants, /) {
    ReachDefDA *reach_def_da = &self->loop_opt->reach_def_da;
    LoopInfo *loop_info = self->loop_info;

    // find invariants
    for (SetPtrIterator it = CALL(SetPtr, loop_info->nodes, begin, /); it;
         it = CALL(SetPtr, loop_info->nodes, next, /, it)) {
        IRBasicBlock *bb = it->key;
        CALL(DataflowAnalysisBase, *TOBASE(reach_def_da), iter_bb, /, bb,
             MTDNAME(ReachDefDA, find_invariants_callback), self);
    }

    // then topologically sort the invariants

    while (self->itg_worklist.size > 0) {
        IRStmtBase *stmt = *CALL(VecPtr, self->itg_worklist, back, /);
        CALL(VecPtr, self->itg_worklist, pop_back, /);
        CALL(VecPtr, self->invariants, push_back, /, stmt);
        MapPtrToVecPtrIterator it_succ =
            CALL(MapPtrToVecPtr, self->itg_succ, find_owned, /, stmt);
        if (!it_succ) {
            continue;
        }
        VecPtr *succs = &it_succ->value;
        for (usize i = 0; i < succs->size; i++) {
            IRStmtBase *succ = succs->data[i];
            MapPtrUSizeIterator indeg_it =
                CALL(MapPtrUSize, self->itg_indeg, find_owned, /, succ);
            ASSERT(indeg_it);
            indeg_it->value--;
            if (indeg_it->value == 0) {
                CALL(VecPtr, self->itg_worklist, push_back, /, succ);
            }
        }
    }
}

static bool MTD(LICMEngine, check_motion, /, IRStmtBase *stmt) {
    LoopOpt *loop_opt = self->loop_opt;

    // check 1: stmt in blocks that dominates all the exits of the loop
    IRBasicBlock *bb = CALL(LoopOpt, *loop_opt, get_bb, /, stmt);
    if (!CALL(DomFact, *self->dom_all_exit, get, /, bb)) {
        return false;
    }

    // check2: the def is not assigned to elsewhere in the loop
    usize def = VCALL(IRStmtBase, *stmt, get_def, /);
    ASSERT(def != (usize)-1, "def == -1 should not happen: the invariant "
                             "should be an assign or arith stmt");
    MapUSizeToSetPtrIterator defstmts_it =
        CALL(MapUSizeToSetPtr, self->var_to_defs, find_owned, /, def);
    ASSERT(defstmts_it);
    SetPtr *defstmts = &defstmts_it->value;
    if (defstmts->size > 1) {
        return false;
    }

    // check3: dominates all use of def in the loop
    MapUSizeToSetPtrIterator usestmts_it =
        CALL(MapUSizeToSetPtr, self->var_to_uses, find_owned, /, def);
    if (!usestmts_it) {
        // no use, so it is ok
        return true;
    }
    SetPtr *uses = &usestmts_it->value;
    for (SetPtrIterator it = CALL(SetPtr, *uses, begin, /); it;
         it = CALL(SetPtr, *uses, next, /, it)) {
        IRStmtBase *use_stmt = it->key;
        if (!CALL(LoopOpt, *loop_opt, is_dom_stmt, /, stmt, use_stmt) ||
            stmt == use_stmt) {
            return false;
        }
    }
    return true;
}

static void MTD(LICMEngine, find_motions, /) {
    ListPtr *motions = &self->motions;
    for (usize i = 0; i < self->invariants.size; i++) {
        IRStmtBase *invariant = self->invariants.data[i];
        if (CALL(LICMEngine, *self, check_motion, /, invariant)) {
            CALL(ListPtr, *motions, push_back, /, invariant);
        }
    }
}

static void MTD(LICMEngine, collect_motions, /) {
    CALL(LICMEngine, *self, collect_info, /);
    CALL(LICMEngine, *self, find_invariants, /);
    CALL(LICMEngine, *self, find_motions, /);
}

static void MTD(LoopOpt, init_licm_engine, /) {
    for (usize i = 0; i < self->loop_infos_ordered.size; i++) {
        LoopInfo *loop_info = self->loop_infos_ordered.data[i];
        LICMEngine *engine = CREOBJHEAP(LICMEngine, /, self, loop_info);
        loop_info->aid_engine = engine;
    }
}

static void MTD(LoopOpt, collect_motions, /) {
    for (usize i = 0; i < self->loop_infos_ordered.size; i++) {
        LoopInfo *loop_info = self->loop_infos_ordered.data[i];
        LICMEngine *engine = loop_info->aid_engine;
        CALL(LICMEngine, *engine, collect_motions, /);
    }
}

static bool MTD(IRFunction, remove_dead_stmt_without_free_stmt_callback, /,
                IRBasicBlock *bb, ListDynIRStmtNode *stmt_it,
                void *extra_args) {
    bool *updated = extra_args;
    if (stmt_it->data->is_dead) {
        stmt_it->data = NULL; // do not free the stmt itself
        CALL(ListDynIRStmt, bb->stmts, remove, /, stmt_it);
        *updated = true;
    }
    return false;
}

static bool MTD(IRFunction, remove_dead_stmt_without_free_stmt, /) {
    bool updated = false;
    CALL(IRFunction, *self, iter_stmt, /,
         MTDNAME(IRFunction, remove_dead_stmt_without_free_stmt_callback),
         &updated);
    return updated;
}

static bool MTD(LoopOpt, apply_motions, /) {
    // first tag all the motions as dead, and determine the move routine
    for (usize i = self->loop_infos_ordered.size - 1; ~i; i--) {
        LoopInfo *loop_info = self->loop_infos_ordered.data[i];
        LICMEngine *engine = loop_info->aid_engine;
        for (ListPtrNode *it = engine->motions.head, *nxt = NULL; it;
             it = nxt) {
            nxt = it->next;

            IRStmtBase *motion = it->data;
            if (motion->is_dead) {
                // this motion has been moved away in larger loops,
                // and we do not need to move it again.
                CALL(ListPtr, engine->motions, remove, /, it);
                continue;
            }
            motion->is_dead = true;
        }
    }

    IRFunction *func = self->func;
    CALL(IRFunction, *func, remove_dead_stmt_without_free_stmt, /);
    bool updated = false;

    for (usize i = 0; i < self->loop_infos_ordered.size; i++) {
        LoopInfo *loop_info = self->loop_infos_ordered.data[i];
        LICMEngine *engine = loop_info->aid_engine;
        for (ListPtrNode *it = engine->motions.head; it; it = it->next) {
            IRStmtBase *motion = it->data;
            motion->is_dead = false;
            CALL(LoopInfo, *loop_info, ensure_preheader, /);
            IRBasicBlock *preheader = loop_info->preheader;
            ASSERT(preheader);
            CALL(IRBasicBlock, *preheader, add_stmt, /, motion);
        }
    }

    return updated;
}

static void MTD(LoopOpt, drop_licm_engine, /) {
    for (usize i = 0; i < self->loop_infos_ordered.size; i++) {
        LoopInfo *loop_info = self->loop_infos_ordered.data[i];
        LICMEngine *engine = loop_info->aid_engine;
        DROPOBJHEAP(LICMEngine, engine);
        loop_info->aid_engine = NULL;
    }
}

bool MTD(LoopOpt, invariant_compute_motion, /) {
    CALL(LoopOpt, *self, init_licm_engine, /);
    CALL(LoopOpt, *self, collect_motions, /);
    bool updated = CALL(LoopOpt, *self, apply_motions, /);
    CALL(LoopOpt, *self, fix_preheader, /);
    CALL(LoopOpt, *self, drop_licm_engine, /);
    return updated;
}

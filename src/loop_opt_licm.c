#include "da_reachdef.h"
#include "general_container.h"
#include "ir_basic_block.h"
#include "ir_stmt.h"
#include "loop_opt.h"

typedef struct LICMEngine {
    LoopOpt *loop_opt;
    LoopInfo *loop_info;

    // temps used to find invariants
    // invariant transition graph:
    // (a -> b): a is invaraint => b (possibly) is invariant
    MapPtrToVecPtr itg_succ;
    MapPtrUSize itg_indeg;
    VecPtr itg_worklist;

    VecPtr invariants; //  of IRStmtBase *

    // in-loop information
    MapUSizeToSetPtr var_to_defs;
    MapUSizeToSetPtr var_to_uses;
} LICMEngine;

static void MTD(LICMEngine, init, /, LoopOpt *loop_opt, LoopInfo *loop_info) {
    self->loop_opt = loop_opt;
    self->loop_info = loop_info;
    CALL(MapPtrToVecPtr, self->itg_succ, init, /);
    CALL(MapPtrUSize, self->itg_indeg, init, /);
    CALL(VecPtr, self->itg_worklist, init, /);
    CALL(VecPtr, self->invariants, init, /);
    CALL(MapUSizeToSetPtr, self->var_to_defs, init, /);
    CALL(MapUSizeToSetPtr, self->var_to_uses, init, /);
}

static void MTD(LICMEngine, drop, /) {
    DROPOBJ(MapUSizeToSetPtr, self->var_to_uses);
    DROPOBJ(MapUSizeToSetPtr, self->var_to_defs);
    DROPOBJ(VecPtr, self->invariants);
    DROPOBJ(VecPtr, self->itg_worklist);
    DROPOBJ(MapPtrUSize, self->itg_indeg);
    DROPOBJ(MapPtrToVecPtr, self->itg_succ);
}

static void VMTD(ReachDefDA, find_invariant_callback, /,
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

static void MTD(LICMEngine, prepare_invariants, /) {
    ReachDefDA *reach_def_da = &self->loop_opt->reach_def_da;
    LoopInfo *loop_info = self->loop_info;

    // find invariants
    for (SetPtrIterator it = CALL(SetPtr, loop_info->nodes, begin, /); it;
         it = CALL(SetPtr, loop_info->nodes, next, /, it)) {
        IRBasicBlock *bb = it->key;
        CALL(DataflowAnalysisBase, *TOBASE(reach_def_da), iter_bb, /, bb,
             MTDNAME(ReachDefDA, find_invariant_callback), self);
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
    // // debug output:
    // printf("LICM invariants:\n");
    // for (usize i = 0; i < self->invariants.size; i++) {
    //     IRStmtBase *stmt = self->invariants.data[i];
    //     String s = CREOBJ(String, /);
    //     VCALL(IRStmtBase, *stmt, build_str, /, &s);
    //     const char *str = STRING_C_STR(s);
    //     printf("%s", str);
    //     DROPOBJ(String, s);
    // }
    // printf("\n");
}

static void MTD(LICMEngine, prepare, /) {
    CALL(LICMEngine, *self, prepare_invariants, /);
}

static bool MTD(LICMEngine, optimize, /) { return false; }

bool MTD(LoopOpt, invariant_compute_motion, /) {
    bool updated = false;
    for (MapHeaderToLoopInfoIterator it =
             CALL(MapHeaderToLoopInfo, self->loop_infos, begin, /);
         it; it = CALL(MapHeaderToLoopInfo, self->loop_infos, next, /, it)) {
        LoopInfo *loop_info = &it->value;
        LICMEngine engine = CREOBJ(LICMEngine, /, self, loop_info);
        CALL(LICMEngine, engine, prepare, /);
        updated = CALL(LICMEngine, engine, optimize, /) || updated;
        DROPOBJ(LICMEngine, engine);
    }
    return updated;
}

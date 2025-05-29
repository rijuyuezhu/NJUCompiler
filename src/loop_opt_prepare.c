#include "da_solver.h"
#include "loop_opt.h"

static bool MTD(IRFunction, prepare_stmt_to_bb_callback, /, IRBasicBlock *bb,
                ListDynIRStmtNode *iter, void *extra_args) {
    MapPtrPtr *stmt_to_bb = extra_args;
    IRStmtBase *stmt = iter->data;
    CALL(MapPtrPtr, *stmt_to_bb, insert, /, stmt, bb);
    return false;
}

static void MTD(LoopOpt, prepare_stmt_to_bb, /) {
    CALL(IRFunction, *self->func, iter_stmt, /,
         MTDNAME(IRFunction, prepare_stmt_to_bb_callback), &self->stmt_to_bb);
}

static void MTD(LoopOpt, prepare_param_to_stmt, /) {
    for (usize i = 0; i < self->func->params.size; i++) {
        usize param = self->func->params.data[i];
        VecIRValue empty_args = CREOBJ(VecIRValue, /);
        String func_name = NSCALL(String, from_f, /, "param[%zu]", i);
        IRStmtBase *stmt = (IRStmtBase *)CREOBJHEAP(IRStmtCall, /, param,
                                                    func_name, empty_args);
        CALL(MapUSizeToDynIRStmt, self->param_to_stmt, insert, /, param, stmt);
    }
}

static void MTD(LoopOpt, prepare_loop_infos_give_dfn_dfs, /, IRBasicBlock *bb,
                usize *dfn_acc) {
    bb->tag = (usize)-2; // visited
    ListPtr *succ = CALL(IRFunction, *self->func, get_succ, /, bb);
    for (ListPtrNode *it = succ->head; it; it = it->next) {
        IRBasicBlock *succ_bb = it->data;
        if (succ_bb->tag == (usize)-1) { // unvisited
            CALL(LoopOpt, *self, prepare_loop_infos_give_dfn_dfs, /, succ_bb,
                 dfn_acc);
        }
    }
    bb->tag = --*dfn_acc; // give dfn
}

static void MTD(LoopOpt, loop_infos_try_find_backedge, /, IRBasicBlock *fr,
                IRBasicBlock *to) {
    if (!(fr->tag >= to->tag)) {
        // not a retreating edge
        return;
    }
    if (!CALL(LoopOpt, *self, is_dom_bb, /, to, fr)) {
        // not a backedge since not dom
        return;
    }
    // now  fr -> to is a backedge
    MapHeaderToLoopInfoIterator it =
        CALL(MapHeaderToLoopInfo, self->loop_infos, find_owned, /, to);
    if (!it) {
        LoopInfo info = CREOBJ(LoopInfo, /, self->func, to);
        MapHeaderToLoopInfoInsertResult res =
            CALL(MapHeaderToLoopInfo, self->loop_infos, insert, /, to, info);
        ASSERT(res.inserted);
        it = res.node;
    }
    LoopInfo *loop_info = &it->value;
    CALL(VecPtr, loop_info->backedge_starts, push_back, /, fr);
    printf("be [%p -> %p]\n", fr, to);
}

static void MTD(LoopOpt, prepare_find_loop_nodes, /, IRBasicBlock *header,
                LoopInfo *loop_info, usize col) {
    CALL(VecPtr, loop_info->nodes, push_back, /, header);
    header->tag = col;

    VecPtr work_stack = CREOBJ(VecPtr, /);
    for (usize i = 0; i < loop_info->backedge_starts.size; i++) {
        IRBasicBlock *start = loop_info->backedge_starts.data[i];
        CALL(VecPtr, work_stack, push_back, /, start);
    }

    while (work_stack.size > 0) {
        IRBasicBlock *now = *CALL(VecPtr, work_stack, back, /);
        CALL(VecPtr, work_stack, pop_back, /);
        if (now->tag == col) {
            // already visited
            continue;
        }
        now->tag = col;
        CALL(VecPtr, loop_info->nodes, push_back, /, now);
        ListPtr *pred = CALL(IRFunction, *self->func, get_pred, /, now);
        for (ListPtrNode *it = pred->head; it; it = it->next) {
            IRBasicBlock *pred_bb = it->data;
            if (pred_bb->tag == col) {
                // already visited
                continue;
            }
            CALL(VecPtr, work_stack, push_back, /, pred_bb);
        }
    }

    // find exits by the way
    for (usize i = 0; i < loop_info->nodes.size; i++) {
        IRBasicBlock *bb = loop_info->nodes.data[i];
        ListPtr *succ = CALL(IRFunction, *self->func, get_succ, /, bb);
        for (ListPtrNode *it = succ->head; it; it = it->next) {
            IRBasicBlock *succ_bb = it->data;
            if (succ_bb->tag != col) {
                // this is an exit
                CALL(VecPtr, loop_info->exits, push_back, /, succ_bb);
            }
        }
    }

    // (debug)
    printf("Loop [%p]:\n", header);
    printf("  Body: ");
    for (usize i = 0; i < loop_info->nodes.size; i++) {
        IRBasicBlock *bb = loop_info->nodes.data[i];
        printf("%p ", bb);
    }
    printf("\n");
    printf("  Exits: ");
    for (usize i = 0; i < loop_info->exits.size; i++) {
        IRBasicBlock *bb = loop_info->exits.data[i];
        printf("%p ", bb);
    }
    printf("\n");
}

static void MTD(LoopOpt, prepare_loop_infos, /) {
    // first give every block dfn (the DFN is the reverse postorder of DFS)
    IRFunction *func = self->func;
    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        bb->tag = (usize)-1; // unvisited
    }
    usize dfn_acc = func->basic_blocks.size;
    CALL(LoopOpt, *self, prepare_loop_infos_give_dfn_dfs, /, func->entry,
         &dfn_acc);

    // then find backedges
    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        if (bb->tag == (usize)-1) {
            continue;
        }
        ListPtr *succ = CALL(IRFunction, *func, get_succ, /, bb);
        for (ListPtrNode *s_it = succ->head; s_it; s_it = s_it->next) {
            IRBasicBlock *succ_bb = s_it->data;
            ASSERT(succ_bb->tag != (usize)-1); // this block can be arrived by
                                               // entry -> bb -> succ_bb
            CALL(LoopOpt, *self, loop_infos_try_find_backedge, /, bb, succ_bb);
        }
    }

    // then find all nodes in a loop

    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        bb->tag = (usize)-1; // no color
    }
    usize col = 0;
    for (MapHeaderToLoopInfoIterator it =
             CALL(MapHeaderToLoopInfo, self->loop_infos, begin, /);
         it; it = CALL(MapHeaderToLoopInfo, self->loop_infos, next, /, it)) {
        LoopInfo *loop_info = &it->value;
        IRBasicBlock *header = loop_info->header;
        CALL(LoopOpt, *self, prepare_find_loop_nodes, /, header, loop_info,
             col);
        col++;
    }
}

void MTD(LoopOpt, prepare, /) {
    // data structures
    CALL(LoopOpt, *self, prepare_stmt_to_bb, /);
    CALL(LoopOpt, *self, prepare_param_to_stmt, /);

    // analysis
    NSCALL(DAWorkListSolver, solve, /, TOBASE(&self->dom_da), self->func);
    NSCALL(DAWorkListSolver, solve, /, TOBASE(&self->reach_def_da), self->func);
    CALL(ReachDefDA, self->reach_def_da, debug_print, /, self->func);

    // find loops
    CALL(LoopOpt, *self, prepare_loop_infos, /);
}

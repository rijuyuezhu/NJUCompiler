#include "da_livevar.h"
#include "da_solver.h"
#include "ir_basic_block.h"
#include "ir_optimizer.h"

static void dce_const_eval_add_worklist(ListPtr *work_list, IRFunction *func,
                                        IRBasicBlock *bb) {
    usize choose_label = (usize)-1;
    if (bb->stmts.tail) {
        IRStmtBase *last_stmt = bb->stmts.tail->data;
        if (last_stmt->kind == IRStmtKindIf) {
            IRStmtIf *stmt_if = (IRStmtIf *)last_stmt;
            IRStmtIfConstEval const_eval =
                CALL(IRStmtIf, *stmt_if, const_eval, /);
            if (const_eval == IRStmtIfConstEvalAlwaysTrue) {
                choose_label = stmt_if->true_label;
            } else if (const_eval == IRStmtIfConstEvalAlwaysFalse) {
                choose_label = stmt_if->false_label;
            }
            if (choose_label != (usize)-1) {
                IRStmtBase *replace =
                    (IRStmtBase *)CREOBJHEAP(IRStmtGoto, /, choose_label);
                CALL(ListDynIRStmt, bb->stmts, remove_back, /);
                CALL(ListDynIRStmt, bb->stmts, push_back, /, replace);
            }
        }
    }

    IRBasicBlock *choose_bb = NULL;
    if (choose_label != (usize)-1) {
        choose_bb = CALL(IRFunction, *func, label_to_bb, /, choose_label);
    }
    ListPtr *succs = CALL(IRFunction, *func, get_succ, /, bb);
    for (ListPtrNode *it = succs->head; it; it = it->next) {
        IRBasicBlock *succ = it->data;
        if (!succ->is_dead) {
            continue;
        }
        if (choose_bb && succ != choose_bb) {
            continue;
        }
        CALL(ListPtr, *work_list, push_back, /, succ);
    }
}
static void run_dce(LiveVarDA *live_var, struct IRFunction *func) {
    for (ListBoxBBNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = it->data;
        bb->is_dead = true;
    }
    ListPtr work_list = CREOBJ(ListPtr, /);
    CALL(ListPtr, work_list, push_back, /, func->entry);
    while (work_list.head) {
        IRBasicBlock *bb = CALL(ListPtr, work_list, pop_front, /);
        if (!bb->is_dead) {
            continue;
        }
        bb->is_dead = false;
        CALL(LiveVarDA, *live_var, dead_code_eliminate_bb, /, bb);
        dce_const_eval_add_worklist(&work_list, func, bb);
    }
    func->entry->is_dead = false;
    func->exit->is_dead = false;
}

bool MTD(IROptimizer, optimize_func_dead_code_eliminate, /,
         struct IRFunction *func) {
    LiveVarDA live_var = CREOBJ(LiveVarDA, /);
    NSCALL(DAWorkListSolver, solve, /, TOBASE(&live_var), func);
    run_dce(&live_var, func);
    DROPOBJ(LiveVarDA, live_var);
    bool updated = CALL(IRFunction, *func, remove_dead_bb, /);
    return CALL(IRFunction, *func, remove_dead_stmt, /) || updated;
}

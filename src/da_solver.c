#include "da_solver.h"
#include "dataflow_analysis.h"
#include "ir_basic_block.h"
#include "ir_function.h"

static void initialize_forward(DataflowAnalysisBase *analysis,
                               IRFunction *func) {
    for (ListBasicBlockNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = &it->data;
        Any in_fact =
            VCALL(DataflowAnalysisBase, *analysis, new_initial_fact, /);
        VCALL(DataflowAnalysisBase, *analysis, set_in_fact, /, bb, in_fact);

        Any out_fact;
        if (bb == func->entry) {
            out_fact = VCALL(DataflowAnalysisBase, *analysis, new_boundary_fact,
                             /, func);
        } else {
            out_fact =
                VCALL(DataflowAnalysisBase, *analysis, new_initial_fact, /);
        }
        VCALL(DataflowAnalysisBase, *analysis, set_out_fact, /, bb, out_fact);
    }
}

static void solve_forward(DataflowAnalysisBase *analysis, IRFunction *func) {
    ListPtr work_list = CREOBJ(ListPtr, /);
    SetPtr in_que = CREOBJ(SetPtr, /);
    for (ListBasicBlockNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = &it->data;
        if (bb != func->entry) {
            CALL(ListPtr, work_list, push_back, /, bb);
            CALL(SetPtr, in_que, insert, /, bb, ZERO_SIZE);
        }
    }
    while (work_list.head) {
        IRBasicBlock *bb = CALL(ListPtr, work_list, pop_front, /);
        SetPtrIterator it = CALL(SetPtr, in_que, find_owned, /, bb);
        ASSERT(it);
        CALL(SetPtr, in_que, erase, /, it);

        ListPtr *pred = CALL(IRFunction, *func, get_pred, /, bb);
        ListPtr *succ = CALL(IRFunction, *func, get_succ, /, bb);
        Any in_fact =
            VCALL(DataflowAnalysisBase, *analysis, get_in_fact, /, bb);
        Any out_fact =
            VCALL(DataflowAnalysisBase, *analysis, get_out_fact, /, bb);
        for (ListPtrNode *listit = pred->head; listit; listit = listit->next) {
            IRBasicBlock *pred_bb = listit->data;
            Any pred_out_fact = VCALL(DataflowAnalysisBase, *analysis,
                                      get_out_fact, /, pred_bb);
            VCALL(DataflowAnalysisBase, *analysis, meet_into, /, pred_out_fact,
                  in_fact);
        }
        bool updated = CALL(DataflowAnalysisBase, *analysis, transfer_bb, /, bb,
                            in_fact, out_fact);
        if (updated) {
            for (ListPtrNode *listit = succ->head; listit;
                 listit = listit->next) {
                IRBasicBlock *succ_bb = listit->data;
                if (succ_bb == func->entry) {
                    continue;
                }
                SetPtrIterator setit =
                    CALL(SetPtr, in_que, find_owned, /, succ_bb);
                if (!setit) {
                    CALL(ListPtr, work_list, push_back, /, succ_bb);
                    CALL(SetPtr, in_que, insert, /, succ_bb, ZERO_SIZE);
                }
            }
        }
    }
    DROPOBJ(SetPtr, in_que);
    DROPOBJ(ListPtr, work_list);
}

static void initialize_backward(DataflowAnalysisBase *analysis,
                                IRFunction *func) {
    for (ListBasicBlockNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = &it->data;
        Any out_fact =
            VCALL(DataflowAnalysisBase, *analysis, new_initial_fact, /);
        VCALL(DataflowAnalysisBase, *analysis, set_out_fact, /, bb, out_fact);

        Any in_fact;
        if (bb == func->exit) {
            in_fact = VCALL(DataflowAnalysisBase, *analysis, new_boundary_fact,
                            /, func);
        } else {
            in_fact =
                VCALL(DataflowAnalysisBase, *analysis, new_initial_fact, /);
        }
        VCALL(DataflowAnalysisBase, *analysis, set_in_fact, /, bb, in_fact);
    }
}

static void solve_backward(DataflowAnalysisBase *analysis, IRFunction *func) {
    ListPtr work_list = CREOBJ(ListPtr, /);
    SetPtr in_que = CREOBJ(SetPtr, /);
    for (ListBasicBlockNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = &it->data;
        if (bb != func->exit) {
            CALL(ListPtr, work_list, push_back, /, bb);
            CALL(SetPtr, in_que, insert, /, bb, ZERO_SIZE);
        }
    }
    while (work_list.head) {
        IRBasicBlock *bb = CALL(ListPtr, work_list, pop_front, /);
        SetPtrIterator it = CALL(SetPtr, in_que, find_owned, /, bb);
        ASSERT(it);
        CALL(SetPtr, in_que, erase, /, it);

        ListPtr *pred = CALL(IRFunction, *func, get_pred, /, bb);
        ListPtr *succ = CALL(IRFunction, *func, get_succ, /, bb);
        Any in_fact =
            VCALL(DataflowAnalysisBase, *analysis, get_in_fact, /, bb);
        Any out_fact =
            VCALL(DataflowAnalysisBase, *analysis, get_out_fact, /, bb);
        for (ListPtrNode *listit = succ->head; listit; listit = listit->next) {
            IRBasicBlock *succ_bb = listit->data;
            Any succ_in_fact =
                VCALL(DataflowAnalysisBase, *analysis, get_in_fact, /, succ_bb);
            VCALL(DataflowAnalysisBase, *analysis, meet_into, /, succ_in_fact,
                  out_fact);
        }
        bool updated = CALL(DataflowAnalysisBase, *analysis, transfer_bb, /, bb,
                            in_fact, out_fact);
        if (updated) {
            for (ListPtrNode *listit = pred->head; listit;
                 listit = listit->next) {
                IRBasicBlock *pred_bb = listit->data;
                if (pred_bb == func->exit) {
                    continue;
                }
                SetPtrIterator setit =
                    CALL(SetPtr, in_que, find_owned, /, pred_bb);
                if (!setit) {
                    CALL(ListPtr, work_list, push_back, /, pred_bb);
                    CALL(SetPtr, in_que, insert, /, pred_bb, ZERO_SIZE);
                }
            }
        }
    }
    DROPOBJ(SetPtr, in_que);
    DROPOBJ(ListPtr, work_list);
}

void NSCALL(DAWorkListSolver, solve, /, DataflowAnalysisBase *analysis,
            IRFunction *func) {
    bool is_forward = VCALL(DataflowAnalysisBase, *analysis, is_forward, /);
    if (is_forward) {
        initialize_forward(analysis, func);
        solve_forward(analysis, func);
    } else {
        initialize_backward(analysis, func);
        solve_backward(analysis, func);
    }
    VCALL(DataflowAnalysisBase, *analysis, debug_print, /, func);
    printf("\n\n");
}

#include "dataflow_analysis.h"

bool MTD(DataflowAnalysisBase, transfer_bb, /, IRBasicBlock *bb, Any in_fact,
         Any out_fact) {
    bool is_forward = VCALL(DataflowAnalysisBase, *self, is_forward, /);
    if (is_forward) {
        Any new_fact = VCALL(DataflowAnalysisBase, *self, new_initial_fact, /);
        VCALL(DataflowAnalysisBase, *self, meet_into, /, in_fact, new_fact);
        for (ListDynIRStmtNode *it = bb->stmts.head; it; it = it->next) {
            IRStmtBase *stmt = it->data;
            VCALL(DataflowAnalysisBase, *self, transfer_stmt, /, stmt,
                  new_fact);
        }
        bool updated = VCALL(DataflowAnalysisBase, *self, meet_into, /,
                             new_fact, out_fact);
        VCALL(DataflowAnalysisBase, *self, drop_fact, /, new_fact);
        return updated;
    } else {
        Any new_fact = VCALL(DataflowAnalysisBase, *self, new_initial_fact, /);
        VCALL(DataflowAnalysisBase, *self, meet_into, /, out_fact, new_fact);
        for (ListDynIRStmtNode *it = bb->stmts.tail; it; it = it->prev) {
            IRStmtBase *stmt = it->data;
            VCALL(DataflowAnalysisBase, *self, transfer_stmt, /, stmt,
                  new_fact);
        }
        bool updated =
            VCALL(DataflowAnalysisBase, *self, meet_into, /, new_fact, in_fact);
        VCALL(DataflowAnalysisBase, *self, drop_fact, /, new_fact);
        return updated;
    }
}

void MTD(DataflowAnalysisBase, iter_bb, /, IRBasicBlock *bb,
         DAIterCallback callback) {
    bool is_forward = VCALL(DataflowAnalysisBase, *self, is_forward, /);
    Any in_fact = VCALL(DataflowAnalysisBase, *self, get_in_fact, /, bb);
    Any out_fact = VCALL(DataflowAnalysisBase, *self, get_out_fact, /, bb);
    if (is_forward) {
        Any new_fact = VCALL(DataflowAnalysisBase, *self, new_initial_fact, /);
        VCALL(DataflowAnalysisBase, *self, meet_into, /, in_fact, new_fact);
        for (ListDynIRStmtNode *it = bb->stmts.head; it; it = it->next) {
            callback(self, it, new_fact);
            IRStmtBase *stmt = it->data;
            VCALL(DataflowAnalysisBase, *self, transfer_stmt, /, stmt,
                  new_fact);
        }
        VCALL(DataflowAnalysisBase, *self, drop_fact, /, new_fact);
    } else {
        Any new_fact = VCALL(DataflowAnalysisBase, *self, new_initial_fact, /);
        VCALL(DataflowAnalysisBase, *self, meet_into, /, out_fact, new_fact);
        for (ListDynIRStmtNode *it = bb->stmts.tail; it; it = it->prev) {
            callback(self, it, new_fact);
            IRStmtBase *stmt = it->data;
            VCALL(DataflowAnalysisBase, *self, transfer_stmt, /, stmt,
                  new_fact);
        }
        VCALL(DataflowAnalysisBase, *self, drop_fact, /, new_fact);
    }
}
void MTD(DataflowAnalysisBase, iter_func, /, IRFunction *func,
         DAIterCallback callback) {
    for (ListBasicBlockNode *it = func->basic_blocks.head; it; it = it->next) {
        IRBasicBlock *bb = &it->data;
        CALL(DataflowAnalysisBase, *self, iter_bb, /, bb, callback);
    }
}

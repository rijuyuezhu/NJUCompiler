#include "loop_opt.h"
#include "da_solver.h"

void MTD(LoopOpt, init, /, IRFunction *func) {
    self->func = func;
    CALL(MapUSizeToDynIRStmt, self->param_to_stmt, init, /);
    CALL(MapPtrPtr, self->stmt_to_bb, init, /);
    CALL(DominatorDA, self->dom_da, init, /, &self->stmt_to_bb);
    CALL(ReachDefDA, self->reach_def_da, init, /, &self->param_to_stmt);
}
void MTD(LoopOpt, drop, /) {
    DROPOBJ(ReachDefDA, self->reach_def_da);
    DROPOBJ(DominatorDA, self->dom_da);
    DROPOBJ(MapPtrPtr, self->stmt_to_bb);
    DROPOBJ(MapUSizeToDynIRStmt, self->param_to_stmt);
}

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

void MTD(LoopOpt, prepare, /) {
    // data structures
    CALL(LoopOpt, *self, prepare_stmt_to_bb, /);
    CALL(LoopOpt, *self, prepare_param_to_stmt, /);
    // analysis
    NSCALL(DAWorkListSolver, solve, /, TOBASE(&self->dom_da), self->func);
    // CALL(DominatorDA, self->dom_da, debug_print, /, self->func);
    NSCALL(DAWorkListSolver, solve, /, TOBASE(&self->reach_def_da), self->func);
    // CALL(ReachDefDA, self->reach_def_da, debug_print, /, self->func);
}
bool MTD(LoopOpt, invariant_compute_motion, /) { return false; }
bool MTD(LoopOpt, induction_var_optimize, /) { return false; }

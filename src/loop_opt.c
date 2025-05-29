#include "loop_opt.h"

void MTD(LoopInfo, init, /, IRFunction *func, IRBasicBlock *header) {
    self->func = func;
    self->header = header;
    CALL(VecPtr, self->backedge_starts, init, /);
    CALL(SetPtr, self->nodes, init, /);
    CALL(VecPtr, self->exits, init, /);
}
void MTD(LoopInfo, drop, /) {
    DROPOBJ(VecPtr, self->exits);
    DROPOBJ(SetPtr, self->nodes);
    DROPOBJ(VecPtr, self->backedge_starts);
}

void MTD(LoopOpt, init, /, IRFunction *func) {
    self->func = func;
    CALL(MapUSizeToDynIRStmt, self->param_to_stmt, init, /);
    CALL(MapPtrPtr, self->stmt_to_bb, init, /);
    CALL(DominatorDA, self->dom_da, init, /, &self->stmt_to_bb);
    CALL(ReachDefDA, self->reach_def_da, init, /, &self->param_to_stmt);
    CALL(MapHeaderToLoopInfo, self->loop_infos, init, /);
}
void MTD(LoopOpt, drop, /) {
    DROPOBJ(MapHeaderToLoopInfo, self->loop_infos);
    DROPOBJ(ReachDefDA, self->reach_def_da);
    DROPOBJ(DominatorDA, self->dom_da);
    DROPOBJ(MapPtrPtr, self->stmt_to_bb);
    DROPOBJ(MapUSizeToDynIRStmt, self->param_to_stmt);
}

bool MTD(LoopOpt, induction_var_optimize, /) { return false; }

DEFINE_MAPPING(MapHeaderToLoopInfo, IRBasicBlock *, LoopInfo, FUNC_EXTERN);

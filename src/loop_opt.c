#include "loop_opt.h"
#include "idx_allocator.h"
#include "ir_function.h"
#include "ir_program.h"

void MTD(LoopInfo, init, /, IRFunction *func, IRBasicBlock *header) {
    self->func = func;
    self->header = header;
    self->preheader = NULL;
    CALL(VecPtr, self->backedge_starts, init, /);
    CALL(SetPtr, self->nodes, init, /);
    CALL(VecPtr, self->exits, init, /);
    self->aid_engine = NULL;
}
void MTD(LoopInfo, drop, /) {
    DROPOBJ(VecPtr, self->exits);
    DROPOBJ(SetPtr, self->nodes);
    DROPOBJ(VecPtr, self->backedge_starts);
}

void MTD(LoopInfo, ensure_preheader, /) {
    if (self->preheader) {
        return;
    }
    IRProgram *program = self->func->program;
    IdxAllocator *label_idx_allocator = &program->label_idx_allocator;

    usize preheader_label =
        CALL(IdxAllocator, *label_idx_allocator, allocate, /);
    self->preheader = CREOBJHEAP(
        IRBasicBlock, /, preheader_label); // this is owned at this time!
}

void MTD(LoopOpt, init, /, IRFunction *func) {
    self->func = func;
    self->bb_info_acc = 0;
    CALL(MapUSizeToDynIRStmt, self->param_to_stmt, init, /);
    CALL(MapStmtToBBInfo, self->stmt_to_bb_info, init, /);
    CALL(DominatorDA, self->dom_da, init, /, &self->stmt_to_bb_info);
    CALL(ReachDefDA, self->reach_def_da, init, /, &self->param_to_stmt);
    CALL(MapHeaderToLoopInfo, self->loop_infos, init, /);
    CALL(VecPtr, self->loop_infos_ordered, init, /);
}
void MTD(LoopOpt, drop, /) {
    DROPOBJ(VecPtr, self->loop_infos_ordered);
    DROPOBJ(MapHeaderToLoopInfo, self->loop_infos);
    DROPOBJ(ReachDefDA, self->reach_def_da);
    DROPOBJ(DominatorDA, self->dom_da);
    DROPOBJ(MapStmtToBBInfo, self->stmt_to_bb_info);
    DROPOBJ(MapUSizeToDynIRStmt, self->param_to_stmt);
}

DEFINE_MAPPING(MapHeaderToLoopInfo, IRBasicBlock *, LoopInfo, FUNC_EXTERN);

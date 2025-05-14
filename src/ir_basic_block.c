#include "ir_basic_block.h"

void MTD(IRBasicBlock, init, /, usize label) {
    CALL(ListPtr, self->stmts, init, /);
    self->label = label;
    self->is_dead = false;
}

void MTD(IRBasicBlock, add_stmt, /, IRStmtBase *stmt) {
    CALL(ListPtr, self->stmts, push_back, /, stmt);
}

void MTD(IRBasicBlock, drop, /) {
    for (ListPtrNode *it = self->stmts.head; it != NULL; it = it->next) {
        IRStmtBase *stmt = (IRStmtBase *)it->data;
        VCALL(IRStmtBase, *stmt, drop, /);
    }
    DROPOBJ(ListPtr, self->stmts);
}

DEFINE_LIST(ListBasicBlock, IRBasicBlock, FUNC_EXTERN);
DEFINE_MAPPING(MapLabelBB, usize, IRBasicBlock *, FUNC_EXTERN);
DEFINE_MAPPING(MapBBToListBB, IRBasicBlock *, ListPtr, FUNC_EXTERN);

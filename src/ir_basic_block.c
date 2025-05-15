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
    for (ListPtrNode *it = self->stmts.head; it; it = it->next) {
        IRStmtBase *stmt = (IRStmtBase *)it->data;
        VDROPOBJHEAP(IRStmtBase, stmt);
    }
    DROPOBJ(ListPtr, self->stmts);
}

void MTD(IRBasicBlock, build_str, /, String *builder) {
    if (self->label != (usize)-1) {
        CALL(String, *builder, pushf, /, "LABEL l%zu :\n", self->label);
    }
    for (ListPtrNode *it = self->stmts.head; it; it = it->next) {
        IRStmtBase *stmt = (IRStmtBase *)it->data;
        VCALL(IRStmtBase, *stmt, build_str, /, builder);
    }
}

DEFINE_LIST(ListBasicBlock, IRBasicBlock, FUNC_EXTERN);
DEFINE_MAPPING(MapLabelBB, usize, IRBasicBlock *, FUNC_EXTERN);
DEFINE_MAPPING(MapBBToListBB, IRBasicBlock *, ListPtr, FUNC_EXTERN);

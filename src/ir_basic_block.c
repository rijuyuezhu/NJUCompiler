#include "ir_basic_block.h"

void MTD(IRBasicBlock, init, /, usize label) {
    CALL(ListDynIRStmt, self->stmts, init, /);
    self->label = label;
    self->is_dead = false;
}

void MTD(IRBasicBlock, add_stmt, /, IRStmtBase *stmt) {
    CALL(ListDynIRStmt, self->stmts, push_back, /, stmt);
}

void MTD(IRBasicBlock, drop, /) { DROPOBJ(ListDynIRStmt, self->stmts); }

void MTD(IRBasicBlock, build_str, /, String *builder) {
    if (self->label != (usize)-1) {
        CALL(String, *builder, pushf, /, "LABEL l%zu :\n", self->label);
    }
    for (ListDynIRStmtNode *it = self->stmts.head; it; it = it->next) {
        IRStmtBase *stmt = (IRStmtBase *)it->data;
        VCALL(IRStmtBase, *stmt, build_str, /, builder);
    }
}
void MTD(IRBasicBlock, debug_print, /) {
    String builder = CREOBJ(String, /);
    CALL(IRBasicBlock, *self, build_str, /, &builder);
    if (builder.size != 0) {
        printf("     ");
    }
    for (usize i = 0; i < builder.size; i++) {
        char ch = builder.data[i];
        if (ch == '\n' && i + 1 != builder.size) {
            printf("\n     ");
        } else {
            putchar(ch);
        }
    }
    DROPOBJ(String, builder);
}

DEFINE_LIST(ListBasicBlock, IRBasicBlock, FUNC_EXTERN);
DEFINE_MAPPING(MapLabelBB, usize, IRBasicBlock *, FUNC_EXTERN);
DEFINE_MAPPING(MapBBToListBB, IRBasicBlock *, ListPtr, FUNC_EXTERN);

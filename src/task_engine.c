#include "task_engine.h"
#include "ast.h"
#include "utils.h"

void MTD(TaskEngine, init, /, const char *src_file, const char *ir_file) {
    self->input_file = src_file;
    self->ir_file = ir_file;
    self->ast_root = NULL;
    self->ast_error = false;

    CALL(TypeManager, self->type_manager, init, /);
    CALL(SymbolManager, self->symbol_manager, init, /);
    self->semantic_error = false;

    CALL(IRManager, self->ir_manager, init, /);
    self->gen_ir_error = false;
}

void MTD(TaskEngine, drop, /) {
    DROPOBJ(IRManager, self->ir_manager);
    DROPOBJ(SymbolManager, self->symbol_manager);
    DROPOBJ(TypeManager, self->type_manager);
    if (self->ast_root) {
        DROPOBJHEAP(AstNode, self->ast_root);
        self->ast_root = NULL;
    }
}

#include "task_engine.h"
#include "ast.h"

void MTD(TaskEngine, init, /, const char *file) {
    self->input_file = file;
    self->ast_root = NULL;
    self->ast_error = false;
    CALL(TypeManager, self->type_manager, init, /);
    CALL(SymbolManager, self->symbol_manager, init, /);
}

void MTD(TaskEngine, drop, /) {
    DROPOBJ(SymbolManager, self->symbol_manager);
    DROPOBJ(TypeManager, self->type_manager);
    if (self->ast_root) {
        DROPOBJHEAP(AstNode, self->ast_root);
        self->ast_root = NULL;
    }
}

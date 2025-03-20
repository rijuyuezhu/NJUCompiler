#include "task_engine.h"
#include "ast.h"

void MTD(TaskEngine, init, /, const char *file) {
    self->input_file = file;
    self->ast_root = NULL;
    self->ast_error = false;
}

void MTD(TaskEngine, drop, /) {
    if (self->ast_root) {
        DROPOBJHEAP(AstNode, self->ast_root);
        self->ast_root = NULL;
    }
}

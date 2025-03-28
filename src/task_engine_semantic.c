#include "semantic.h"
#include "task_engine.h"

void MTD(TaskEngine, analyze_semantic, /, bool add_builtin) {
    ASSERT(self->ast_root != NULL && !self->ast_error,
           "Syntax analysis failed");
    SemResolver resolver =
        CREOBJ(SemResolver, /, &self->type_manager, &self->symbol_manager);
    CALL(SemResolver, resolver, resolve, /, self->ast_root, add_builtin);
    self->semantic_error = resolver.sem_error;
    DROPOBJ(SemResolver, resolver);
}

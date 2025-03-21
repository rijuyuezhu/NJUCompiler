#include "semantic.h"
#include "task_engine.h"

void MTD(TaskEngine, analyze_semantic, /) {
    SemResolver resolver =
        CREOBJ(SemResolver, /, &self->type_manager, &self->symbol_manager);
    CALL(SemResolver, resolver, resolve, /, self->ast_root);
    self->semantic_error = resolver.sem_error;
    DROPOBJ(SemResolver, resolver);
}

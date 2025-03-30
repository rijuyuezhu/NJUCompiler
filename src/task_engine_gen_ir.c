#include "gen_ir.h"
#include "task_engine.h"

void MTD(TaskEngine, gen_ir, /) {
    IRGenerator generator = CREOBJ(IRGenerator, /, &self->ir_manager);
    DROPOBJ(IRGenerator, generator);
}

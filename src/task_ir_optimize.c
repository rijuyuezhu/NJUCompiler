#include "ir_optimizer.h"
#include "task_engine.h"

void MTD(TaskEngine, ir_optimize, /) {
    IROptimizer optimizer = CREOBJ(IROptimizer, /, &self->ir_program);
    CALL(IROptimizer, optimizer, optimize, /);
    DROPOBJ(IROptimizer, optimizer);
}

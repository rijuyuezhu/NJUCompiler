#include "debug.h"
#include "task_engine.h"

static void run_task(TaskEngine *engine) {
    CALL(TaskEngine, *engine, ir_parse, /);
    if (engine->parse_err) {
        return;
    }
    CALL(TaskEngine, *engine, ir_optimize, /);
    CALL(TaskEngine, *engine, ir_gen_str, /);
    CALL(TaskEngine, *engine, ir_save, /);
}

int main(int argc, char *argv[]) {
    ASSERT(argc == 3, "Usage: %s <input ir file> <output ir file>", argv[0]);
    TaskEngine *engine = CREOBJHEAP(TaskEngine, /, argv[1], argv[2]);
    run_task(engine);
    DROPOBJHEAP(TaskEngine, engine);
    return 0;
}

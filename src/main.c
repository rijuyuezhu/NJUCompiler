#include "debug.h"
#include "task_engine.h"

#define ASSIGNMENT 2

static void run_task(TaskEngine *engine) {
    /*
     * lexical & syntax analysis
     */
    CALL(TaskEngine, *engine, parse_ast, /);
    if (!engine->ast_root || engine->ast_error) {
        return;
    }

#if ASSIGNMENT == 1
    CALL(TaskEngine, *engine, print_ast, /);
    return;
#endif

    /*
     * semantic analysis
     */
    CALL(TaskEngine, *engine, analyze_semantic, /);
    if (engine->semantic_error) {
        return;
    }

#if ASSIGNMENT == 2
    return;
#endif
}

int main(int argc, char *argv[]) {
    ASSERT(argc == 2);
    TaskEngine *engine = CREOBJHEAP(TaskEngine, /, argv[1]);
    run_task(engine);
    DROPOBJHEAP(TaskEngine, engine);
    return 0;
}

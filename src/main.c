#include "debug.h"
#include "task_engine.h"

#define ASSIGNMENT 3

#if ASSIGNMENT <= 2
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
    CALL(TaskEngine, *engine, analyze_semantic, /, false);
    if (engine->semantic_error) {
        return;
    }
}

int main(int argc, char *argv[]) {
    ASSERT(argc == 2, "Usage: %s <source file>", argv[0]);
    TaskEngine *engine = CREOBJHEAP(TaskEngine, /, argv[1], NULL);
    run_task(engine);
    DROPOBJHEAP(TaskEngine, engine);
    return 0;
}
#elif ASSIGNMENT == 3

static void run_task(TaskEngine *engine) {
    /*
     * lexical & syntax analysis
     */
    CALL(TaskEngine, *engine, parse_ast, /);
    if (!engine->ast_root || engine->ast_error) {
        return;
    }
    /*
     * semantic analysis
     */
    CALL(TaskEngine, *engine, analyze_semantic, /, true);
    if (engine->semantic_error) {
        return;
    }
    /*
     * gen ir
     */
    CALL(TaskEngine, *engine, gen_ir, /);
    if (engine->gen_ir_error) {
        return;
    }
    CALL(TaskEngine, *engine, save_ir_to_file, /);
}

int main(int argc, char *argv[]) {
    ASSERT(argc == 3, "Usage: %s <source file> <output file>", argv[0]);
    TaskEngine *engine = CREOBJHEAP(TaskEngine, /, argv[1], argv[2]);
    run_task(engine);
    DROPOBJHEAP(TaskEngine, engine);
    return 0;
}
#endif

#include "debug.h"
#include "task_engine.h"

#define ASSIGNMENT 2

int main(int argc, char *argv[]) {
    ASSERT(argc == 2);
    TaskEngine *engine = CREOBJHEAP(TaskEngine, /, argv[1]);

    /*
     * lexical & syntax analysis
     */
    CALL(TaskEngine, *engine, parse_ast, /);
    if (!engine->ast_root || engine->ast_error) {
        goto exit;
    }

#if ASSIGNMENT == 1
    CALL(TaskEngine, engine, print_ast, /);
    goto exit;
#endif

    /*
     * semantic analysis
     */
    CALL(TaskEngine, *engine, analyze_semantic, /);
    if (engine->semantic_error) {
        goto exit;
    }

#if ASSIGNMENT == 2
    goto exit;
#endif

exit:
    DROPOBJHEAP(TaskEngine, engine);
    return 0;
}

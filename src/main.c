#include "debug.h"
#include "task_engine.h"

int main(int argc, char *argv[]) {
    ASSERT(argc == 2);
    TaskEngine *engine = CREOBJHEAP(TaskEngine, /, argv[1]);

    // lexical & syntax analysis
    CALL(TaskEngine, *engine, parse_ast, /);
    if (engine->ast_root && !engine->ast_error) { // No error in parsing
        // // A1-specific task
        // CALL(TaskEngine, engine, print_ast, /);
    } else {
        goto exit;
    }

    // semantic analysis
    CALL(TaskEngine, *engine, analyze_semantic, /);
exit:
    DROPOBJHEAP(TaskEngine, engine);
    return 0;
}

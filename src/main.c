#include "debug.h"
#include "task_engine.h"

int main(int argc, char *argv[]) {
    ASSERT(argc == 2);
    TaskEngine engine = CREOBJ(TaskEngine, /, argv[1]);

    // syntax_analysis
    CALL(TaskEngine, engine, parse_ast, /);
    if (engine.ast_root && !engine.ast_error) { // No error in parsing
        // A1 task
        CALL(TaskEngine, engine, print_ast, /);
    } else {
        goto exit;
    }

    // TODO: semantic_analysis
exit:
    DROPOBJ(TaskEngine, engine);
    return 0;
}

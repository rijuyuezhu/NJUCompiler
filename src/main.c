#include "debug.h"
#include "task_engine.h"
int main(int argc, char *argv[]) {
    ASSERT(argc == 2);
    TaskEngine engine = CREOBJ(TaskEngine, /, argv[1]);
    CALL(TaskEngine, engine, analyze_ast, /);
    if (engine.ast_root != NULL) { // No error in parsing
        CALL(TaskEngine, engine, output_ast, /);
    }
    DROPOBJ(TaskEngine, engine);
}

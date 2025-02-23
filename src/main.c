#include "debug.h"
#include "task_engine.h"

int main(int argc, char *argv[]) {
    ASSERT(argc == 2);
    TaskEngine engine = CREOBJ(TaskEngine, /, argv[1]);
    CALL(TaskEngine, engine, parse_ast, /);
    if (engine.ast_root && !engine.ast_error) { // No error in parsing
        CALL(TaskEngine, engine, print_ast, /);
    }
    DROPOBJ(TaskEngine, engine);
}

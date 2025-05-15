#include "syntax.tab.h"
#include "task_engine.h"

extern void yyrestart(FILE *input_file);
extern void yylex_destroy();

void MTD(TaskEngine, ir_parse, /) {

    FILE *fp = fopen(self->input_ir_file, "r");
    if (!fp) {
        perror(self->input_ir_file);
        exit(EXIT_FAILURE);
    }
    yyrestart(fp);
    yyparse(self);
    yylex_destroy();
    fclose(fp);
    if (self->parse_err) {
        return;
    }
    CALL(IRProgram, self->ir_program, establish, /, self);
}

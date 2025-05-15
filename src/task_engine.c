#include "task_engine.h"
#include "utils.h"

void MTD(TaskEngine, init, /, const char *input_ir_file,
         const char *output_ir_file) {
    self->input_ir_file = input_ir_file;
    self->output_ir_file = output_ir_file;
    CALL(IRProgram, self->ir_program, init, /);
    CALL(ParseHelper, self->parse_helper, init, /);
    self->parse_err = false;
    CALL(String, self->ir_program_str, init, /);
}

void MTD(TaskEngine, drop, /) {
    DROPOBJ(String, self->ir_program_str);
    DROPOBJ(ParseHelper, self->parse_helper);
    DROPOBJ(IRProgram, self->ir_program);
}

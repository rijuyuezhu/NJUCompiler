#include "task_engine.h"
#include "utils.h"

void MTD(TaskEngine, init, /, const char *input_ir_file, const char *output_ir_file) {
    self->input_ir_file = input_ir_file;
    self->output_ir_file = output_ir_file;
}

void MTD(TaskEngine, drop, /) {
}

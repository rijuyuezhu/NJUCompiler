#include "task_engine.h"

void MTD(TaskEngine, ir_gen_str, /) {
    CALL(String, self->ir_program_str, clear, /);
    CALL(IRProgram, self->ir_program, build_str, /, &self->ir_program_str);
}
void MTD(TaskEngine, ir_save, /) {
    if (self->output_ir_file == NULL) {
        fprintf(stderr, "Error: <output ir file> is NULL\n");
        return;
    }
    FILE *fp = fopen(self->output_ir_file, "w");
    if (fp == NULL) {
        perror(self->output_ir_file);
        exit(EXIT_FAILURE);
    }
    String *result = &self->ir_program_str;
    fwrite(result->data, 1, result->size, fp);
    fclose(fp);
}

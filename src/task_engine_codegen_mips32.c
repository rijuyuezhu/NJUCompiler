#include "codegen_mips32.h"
#include "task_engine.h"
#include "utils.h"

void MTD(TaskEngine, codegen_mips32, /) {
    CALL(CodegenMips32, self->codegen_mips32, generate, /);
}

void MTD(TaskEngine, codegen_mips32_dump_to_file, /) {
    if (self->asm_file == NULL) {
        fprintf(stderr, "Error: asm_file is NULL\n");
        return;
    }
    FILE *fp = fopen(self->asm_file, "w");
    if (fp == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    String *result = &self->codegen_mips32.result;
    fwrite(result->data, 1, result->size, fp);
    fclose(fp);
}

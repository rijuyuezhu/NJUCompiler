#include <stdio.h>

#include "gen_ir.h"
#include "task_engine.h"

void MTD(TaskEngine, gen_ir, /) {
    IRGenerator generator = CREOBJ(IRGenerator, /, &self->ir_manager,
                                   &self->type_manager, &self->symbol_manager);
    CALL(IRGenerator, generator, gen_ir, /, self->ast_root);
    self->gen_ir_error = generator.gen_ir_error;
    DROPOBJ(IRGenerator, generator);
}

void MTD(TaskEngine, save_ir_to_file, /) {
    ASSERT(!self->gen_ir_error, "IR generation error occurred");

    String ir_str = CALL(IRManager, self->ir_manager, get_ir_str, /);
    ASSERT(self->ir_file);
    FILE *fp = fopen(self->ir_file, "w");
    if (!fp) {
        perror(self->ir_file);
        exit(EXIT_FAILURE);
    }
    fprintf(fp, "%s", STRING_C_STR(ir_str));
    fclose(fp);
    DROPOBJ(String, ir_str);
}

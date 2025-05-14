#pragma once

#include "ir_program.h"
#include "utils.h"

typedef struct TaskEngine {
    const char *input_ir_file;
    const char *output_ir_file;

    IRProgram ir_program;
} TaskEngine;

void MTD(TaskEngine, init, /, const char *input_ir_file,
         const char *output_ir_file);
void MTD(TaskEngine, ir_opt, /);
void MTD(TaskEngine, drop, /);

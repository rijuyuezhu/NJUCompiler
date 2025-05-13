#pragma once

#include "ir.h"
#include "utils.h"

typedef struct TaskEngine {
    const char *input_ir_file;
    const char *output_ir_file;
} TaskEngine;

void MTD(TaskEngine, init, /, const char *input_ir_file, const char *output_ir_file);
void MTD(TaskEngine, drop, /);

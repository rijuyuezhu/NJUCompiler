#pragma once

#include "ir_program.h"
#include "parse_helper.h"
#include "utils.h"

typedef struct TaskEngine {
    const char *input_ir_file;
    const char *output_ir_file;

    ParseHelper parse_helper;
    IRProgram ir_program;
    bool parse_err;

    String ir_program_str;
} TaskEngine;

void MTD(TaskEngine, init, /, const char *input_ir_file,
         const char *output_ir_file);
void MTD(TaskEngine, ir_parse, /);
void MTD(TaskEngine, ir_optimize, /);
void MTD(TaskEngine, ir_gen_str, /);
void MTD(TaskEngine, ir_save, /);
void MTD(TaskEngine, drop, /);

#pragma once

#include "ir_program.h"
#include "parse_helper.h"
#include "utils.h"

typedef struct TaskEngine {
    const char *input_ir_file;
    const char *output_ir_file;

    IRProgram ir_program;
    ParseHelper parse_helper;
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

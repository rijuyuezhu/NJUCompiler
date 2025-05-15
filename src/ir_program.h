#pragma once

#include "idx_allocator.h"
#include "ir_function.h"

typedef struct IRProgram {
    VecIRFunction functions;
    IdxAllocator var_idx_allocator;
    IdxAllocator label_idx_allocator;
} IRProgram;

void MTD(IRProgram, init, /);
void MTD(IRProgram, drop, /);
void MTD(IRProgram, build_str, /, String *builder);
void MTD(IRProgram, establish, /, struct TaskEngine *engine);

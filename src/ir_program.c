#include "ir_program.h"
#include "utils.h"

void MTD(IRProgram, init, /) {
    CALL(VecIRFunction, self->functions, init, /);
    CALL(IdxAllocator, self->var_idx_allocator, init, /);
    CALL(IdxAllocator, self->label_idx_allocator, init, /);
}
void MTD(IRProgram, drop, /) {
    CALL(IdxAllocator, self->label_idx_allocator, drop, /);
    CALL(IdxAllocator, self->var_idx_allocator, drop, /);
    CALL(VecIRFunction, self->functions, drop, /);
}
void MTD(IRProgram, build_str, /, String *builder) {
    for (usize i = 0; i < self->functions.size; i++) {
        IRFunction *func = &self->functions.data[i];
        CALL(IRFunction, *func, build_str, /, builder);
    }
}
void MTD(IRProgram, establish, /, struct TaskEngine *engine) {
    for (usize i = 0; i < self->functions.size; i++) {
        IRFunction *func = &self->functions.data[i];
        CALL(IRFunction, *func, establish, /, engine);
    }
}

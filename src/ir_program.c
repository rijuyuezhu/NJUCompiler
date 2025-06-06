#include "ir_program.h"
#include "renamer.h"

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

void MTD(IRProgram, rename_var_labels, /) {
    CALL(IdxAllocator, self->var_idx_allocator, clear, /);
    CALL(IdxAllocator, self->label_idx_allocator, clear, /);
    Renamer var_renamer = CREOBJ(Renamer, /, &self->var_idx_allocator);
    Renamer label_renamer = CREOBJ(Renamer, /, &self->label_idx_allocator);
    for (usize i = 0; i < self->functions.size; i++) {
        IRFunction *func = &self->functions.data[i];
        CALL(IRFunction, *func, rename, /, &var_renamer, &label_renamer);
    }
    DROPOBJ(Renamer, label_renamer);
    DROPOBJ(Renamer, var_renamer);
}

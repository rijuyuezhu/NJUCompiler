#include "ir_value.h"
#include "str.h"
#include "utils.h"

IRValue NSMTD(IRValue, from_const, /, int const_val) {
    return (IRValue){
        .is_const = true,
        .const_val = const_val,
    };
}

IRValue NSMTD(IRValue, from_var, /, usize var) {
    return (IRValue){
        .is_const = false,
        .var = var,
    };
}

void MTD(IRValue, build_str, /, String *builder) {
    if (self->is_const) {
        CALL(String, *builder, pushf, /, "#%d", self->const_val);
    } else {
        CALL(String, *builder, pushf, /, "v%zu", self->var);
    }
}

int NSMTD(IRValue, compare, /, const IRValue *a, const IRValue *b) {
    if (a->is_const) {
        if (b->is_const) {
            return NORMALCMP(a->const_val, b->const_val);
        } else {
            return 1;
        }
    } else {
        if (b->is_const) {
            return -1;
        } else {
            return NORMALCMP(a->var, b->var);
        }
    }
}

void MTD(IRValue, rename, /, Renamer *renamer) {
    if (!self->is_const) {
        CALL(Renamer, *renamer, rename, /, &self->var);
    }
}

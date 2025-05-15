#include "ir_value.h"
#include "str.h"

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

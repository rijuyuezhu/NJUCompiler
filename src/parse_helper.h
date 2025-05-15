#pragma once
#include "ir_function.h"
#include "ir_value.h"
#include "utils.h"

typedef struct ParseHelper {
    IRFunction *now_func;
    VecIRValue arglist;
} ParseHelper;

FUNC_STATIC void MTD(ParseHelper, init, /) {
    CALL(VecIRValue, self->arglist, init, /);
}
FUNC_STATIC void MTD(ParseHelper, drop, /) {
    DROPOBJ(VecIRValue, self->arglist);
}

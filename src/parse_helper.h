#pragma once

#include "ir_value.h"

typedef struct ParseHelper {
    struct IRFunction *now_func;
    VecIRValue arglist;
} ParseHelper;

FUNC_STATIC void MTD(ParseHelper, init, /) {
    CALL(VecIRValue, self->arglist, init, /);
}
FUNC_STATIC void MTD(ParseHelper, drop, /) {
    DROPOBJ(VecIRValue, self->arglist);
}

#pragma once

#include "tem_vec.h"

struct String;
struct Renamer;
typedef struct IRValue {
    bool is_const;
    union {
        usize var;
        int const_val;
    };
} IRValue;

IRValue NSMTD(IRValue, from_const, /, int const_val);
IRValue NSMTD(IRValue, from_var, /, usize var);
void MTD(IRValue, build_str, /, struct String *builder);
int NSMTD(IRValue, compare, /, const IRValue *a, const IRValue *b);
void MTD(IRValue, rename, /, struct Renamer *renamer);

DECLARE_PLAIN_VEC(VecIRValue, IRValue, FUNC_EXTERN)

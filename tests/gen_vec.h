#pragma once
#include "str.h"
#include "tem_vec.h"

DECLARE_PLAIN_VEC(VecCString, const char *, FUNC_EXTERN);
DECLARE_PLAIN_VEC(VecI32, i32, FUNC_EXTERN);
DECLARE_CLASS_VEC(VecStr, String, FUNC_EXTERN);

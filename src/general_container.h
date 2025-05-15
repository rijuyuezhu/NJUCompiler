#pragma once

#include "str.h"
#include "tem_list.h"
#include "tem_map.h"
#include "tem_vec.h"
#include "utils.h"

DECLARE_PLAIN_VEC(VecPtr, void *, FUNC_EXTERN);
DECLARE_PLAIN_VEC(VecUSize, usize, FUNC_EXTERN);
DECLARE_LIST(ListPtr, void *, FUNC_EXTERN, GENERATOR_PLAIN_VALUE);
DECLARE_MAPPING(MapStrUSize, HString, usize, FUNC_EXTERN, GENERATOR_CLASS_KEY,
                GENERATOR_PLAIN_VALUE, GENERATOR_CLASS_COMPARATOR);
DECLARE_MAPPING(SetPtr, void *, ZERO_SIZE_TYPE, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_PLAIN_VALUE,
                GENERATOR_PLAIN_COMPARATOR);

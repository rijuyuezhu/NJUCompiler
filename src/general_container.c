#include "general_container.h"

DEFINE_PLAIN_VEC(VecPtr, void *, FUNC_EXTERN);
DEFINE_PLAIN_VEC(VecUSize, usize, FUNC_EXTERN);
DEFINE_LIST(ListPtr, void *, FUNC_EXTERN);
DEFINE_MAPPING(MapStrUSize, HString, usize, FUNC_EXTERN);
DEFINE_MAPPING(SetPtr, void *, ZERO_SIZE_TYPE, FUNC_EXTERN);

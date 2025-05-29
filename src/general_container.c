#include "general_container.h"

DEFINE_PLAIN_VEC(VecPtr, void *, FUNC_EXTERN);
DEFINE_PLAIN_VEC(VecUSize, usize, FUNC_EXTERN);
DEFINE_LIST(ListPtr, void *, FUNC_EXTERN);
DEFINE_MAPPING(MapStrUSize, HString, usize, FUNC_EXTERN);
DEFINE_MAPPING(MapUSizeUSize, usize, usize, FUNC_EXTERN);
DEFINE_MAPPING(MapPtrPtr, void *, void *, FUNC_EXTERN);
DEFINE_MAPPING(MapPtrUSize, void *, usize, FUNC_EXTERN);
DEFINE_MAPPING(SetPtr, void *, ZERO_SIZE_TYPE, FUNC_EXTERN);
DEFINE_MAPPING(SetUSize, usize, ZERO_SIZE_TYPE, FUNC_EXTERN);
DEFINE_MAPPING(MapUSizeToVecUSize, usize, VecUSize, FUNC_EXTERN);
DEFINE_MAPPING(MapPtrToVecPtr, void *, VecPtr, FUNC_EXTERN);
DEFINE_MAPPING(MapUSizeToSetUSize, usize, SetUSize, FUNC_EXTERN);
DEFINE_MAPPING(MapUSizeToSetPtr, usize, SetPtr, FUNC_EXTERN);

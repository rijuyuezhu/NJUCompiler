#pragma once

#include "general_vec.h"
#include "tem_vec.h"
#include "utils.h"

/* Type */

typedef enum TypeKind {
    TypeKindInvalid,
    TypeKindInt,
    TypeKindFloat,
    TypeKindArray,
    TypeKindStruct,
} TypeKind;

typedef struct Type {
    TypeKind kind;
    union {
        struct {
            usize size;
            usize dim;
            usize subtype_idx;
        } as_array;
        struct {
            VecUSize field_idxes;
        } as_struct;
    };
} Type;

DELETED_CLONER(Type, FUNC_STATIC);
DECLARE_CLASS_VEC(VecType, Type, FUNC_EXTERN);

struct TypeManager;

void MTD(Type, drop, /);
Type NSMTD(Type, make_array, /, struct TypeManager *manager, usize size,
           usize subtype_idx);
Type NSMTD(Type, make_struct, /);
void MTD(Type, add_struct_field, /, usize field_idx);

/* TypeManager */

typedef struct TypeManager {
    VecType types;
    usize int_type_idx;
    usize float_type_idx;
} TypeManager;

void MTD(TypeManager, init, /);
void MTD(TypeManager, drop, /);
usize MTD(TypeManager, add_type, /, Type type);

FUNC_STATIC Type MTD(TypeManager, make_array, /, usize size,
                     usize subtype_idx) {
    return NSCALL(Type, make_array, /, self, size, subtype_idx);
}

FUNC_STATIC Type MTD(TypeManager, make_struct, /) {
    return NSCALL(Type, make_struct, /);
}

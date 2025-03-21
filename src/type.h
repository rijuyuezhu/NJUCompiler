#pragma once

#include "general_vec.h"
#include "tem_vec.h"
#include "utils.h"

/* Type */

typedef enum TypeKind {
    TypeKindInvalid,
    TypeKindInt,
    TypeKindFloat,
    TypeKindVoid,
    TypeKindArray,
    TypeKindStruct,
    TypeKindFun,
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
            usize symtab_idx;
        } as_struct;
        struct {
            VecUSize ret_par_idxes; // the 0th element is the return type
        } as_fun;
    };
} Type;

DELETED_CLONER(Type, FUNC_STATIC);
DECLARE_CLASS_VEC(VecType, Type, FUNC_EXTERN);

struct TypeManager;

void MTD(Type, drop, /);
Type NSMTD(Type, make_array, /, struct TypeManager *manager, usize size,
           usize subtype_idx);
Type NSMTD(Type, make_struct, /, usize symtab_idx);
void MTD(Type, add_struct_field, /, usize field_idx);
Type NSMTD(Type, make_fun, /);
void MTD(Type, add_fun_ret_par, /, usize ret_par_idx);

/* TypeManager */

typedef struct TypeManager {
    VecType types;
    usize int_type_idx;
    usize float_type_idx;
    usize void_type_idx;
} TypeManager;

void MTD(TypeManager, init, /);
void MTD(TypeManager, drop, /);

usize MTD(TypeManager, add_type, /, Type type);
bool MTD(TypeManager, pop_type, /, usize last_type_idx);
usize MTD(TypeManager, make_array, /, usize size, usize subtype_idx);
usize MTD(TypeManager, make_struct, /, usize symtab_idx);
usize MTD(TypeManager, make_fun, /);
void MTD(TypeManager, add_struct_field, /, usize type_idx, usize field_idx);
void MTD(TypeManager, add_fun_ret_par, /, usize type_idx, usize ret_par_idx);
bool MTD(TypeManager, is_type_consistency, /, usize type_idx1, usize type_idx2);
bool MTD(TypeManager, is_type_consistency_with_fun_fix, /, usize type_idx1,
         usize type_idx2);

FUNC_STATIC Type *MTD(TypeManager, get_type, /, usize type_idx) {
    ASSERT(type_idx < self->types.size, "type_idx out of range");
    return &self->types.data[type_idx];
}

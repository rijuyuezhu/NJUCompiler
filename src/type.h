#pragma once

#include "general_vec.h"
#include "tem_map.h"
#include "tem_vec.h"
#include "utils.h"

struct TypeManager;

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
    usize repr_val; // representive index; for fast equal test.
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

FUNC_STATIC DEFAULT_INITIALIZER(Type);
void MTD(Type, clone_from, /, const Type *other);
FUNC_STATIC DEFAULT_DERIVE_CLONE(Type, /);
DECLARE_CLASS_VEC(VecType, Type, FUNC_EXTERN);

void MTD(Type, drop, /);
Type NSMTD(Type, make_array, /, struct TypeManager *manager, usize size,
           usize subtype_idx);
Type NSMTD(Type, make_struct, /, usize symtab_idx);
void MTD(Type, add_struct_field, /, usize field_idx);
Type NSMTD(Type, make_fun, /);
void MTD(Type, add_fun_ret_par, /, usize ret_par_idx);
int NSMTD(Type, compare, /, const Type *type1, const Type *type2);

typedef struct HType {
    Type t;
    u64 stored_hash;
} HType;

DELETED_CLONER(HType, FUNC_STATIC);
FUNC_STATIC void MTD(HType, drop, /) { DROPOBJ(Type, self->t); }
void MTD(HType, rehash, /);
int NSMTD(HType, compare, /, const HType *ht1, const HType *ht2);
HType NSMTD(HType, from_inner, /, Type t);

DECLARE_MAPPING(MapHTypeUSize, HType, usize, FUNC_EXTERN, GENERATOR_CLASS_KEY,
                GENERATOR_PLAIN_VALUE, GENERATOR_CLASS_COMPARATOR);

/* TypeManager */

typedef struct TypeManager {
    VecType types;
    MapHTypeUSize repr_map;
    usize repr_cnt;
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
void MTD(TypeManager, fill_in_repr, /, usize type_idx);
Type *MTD(TypeManager, get_type, /, usize type_idx);

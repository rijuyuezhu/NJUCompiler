#include "type.h"
#include "symbol.h"
#include "tem_vec.h"

void MTD(Type, drop, /) {
    if (self->kind == TypeKindStruct) {
        DROPOBJ(VecUSize, self->as_struct.field_idxes);
    } else if (self->kind == TypeKindFun) {
        DROPOBJ(VecUSize, self->as_fun.ret_par_idxes);
    }
}

Type NSMTD(Type, make_array, /, TypeManager *manager, usize size,
           usize subtype_idx) {
    Type result = (Type){
        .kind = TypeKindArray,
        .as_array = {.size = size, .subtype_idx = subtype_idx},
    };
    Type *subtype = &manager->types.data[subtype_idx];
    if (subtype->kind == TypeKindArray) {
        result.as_array.dim = subtype->as_array.dim + 1;
    } else {
        result.as_array.dim = 1;
    }
    return result;
}

Type NSMTD(Type, make_struct, /, usize symtab_idx) {
    return (Type){
        .kind = TypeKindStruct,
        .as_struct =
            {
                .field_idxes = CREOBJ(VecUSize, /),
                .symtab_idx = symtab_idx,
            },
    };
}

void MTD(Type, add_struct_field, /, usize field_idx) {
    ASSERT(self->kind == TypeKindStruct);
    CALL(VecUSize, self->as_struct.field_idxes, push_back, /, field_idx);
}

Type NSMTD(Type, make_fun, /) {
    return (Type){
        .kind = TypeKindFun,
        .as_fun = {.ret_par_idxes = CREOBJ(VecUSize, /)},
    };
}

void MTD(Type, add_fun_ret_par, /, usize ret_par_idx) {
    ASSERT(self->kind == TypeKindFun);
    CALL(VecUSize, self->as_fun.ret_par_idxes, push_back, /, ret_par_idx);
}

void MTD(TypeManager, init, /) {
    CALL(VecType, self->types, init, /);
    self->int_type_idx =
        CALL(TypeManager, *self, add_type, /, (Type){.kind = TypeKindInt});
    self->float_type_idx =
        CALL(TypeManager, *self, add_type, /, (Type){.kind = TypeKindFloat});
    self->void_type_idx =
        CALL(TypeManager, *self, add_type, /, (Type){.kind = TypeKindVoid});
}

void MTD(TypeManager, drop, /) { DROPOBJ(VecType, self->types); }

usize MTD(TypeManager, add_type, /, Type type) {
    CALL(VecType, self->types, push_back, /, type);
    return self->types.size - 1;
}

bool MTD(TypeManager, pop_type, /, usize last_type_idx) {
    if (last_type_idx + 1 == self->types.size) {
        CALL(VecType, self->types, pop_back, /);
        return true;
    } else {
        return false;
    }
}

usize MTD(TypeManager, make_array, /, usize size, usize subtype_idx) {
    Type t = NSCALL(Type, make_array, /, self, size, subtype_idx);
    return CALL(TypeManager, *self, add_type, /, t);
}

usize MTD(TypeManager, make_struct, /, usize symtab_idx) {
    Type t = NSMTD(Type, make_struct, /, symtab_idx);
    return CALL(TypeManager, *self, add_type, /, t);
}

usize MTD(TypeManager, make_fun, /) {
    Type t = NSMTD(Type, make_fun, /);
    return CALL(TypeManager, *self, add_type, /, t);
}

void MTD(TypeManager, add_struct_field, /, usize type_idx, usize field_idx) {
    CALL(Type, self->types.data[type_idx], add_struct_field, /, field_idx);
}

void MTD(TypeManager, add_fun_ret_par, /, usize type_idx, usize ret_par_idx) {
    CALL(Type, self->types.data[type_idx], add_fun_ret_par, /, ret_par_idx);
}

FUNC_STATIC bool MTD(TypeManager, is_type_array_consistency, /, Type *t1,
                     Type *t2) {
    return t1->as_array.dim == t2->as_array.dim &&
           CALL(TypeManager, *self, is_type_consistency, /,
                t1->as_array.subtype_idx, t2->as_array.subtype_idx);
}

FUNC_STATIC bool MTD(TypeManager, is_type_struct_consistency, /, Type *t1,
                     Type *t2) {
    VecUSize *f1 = &t1->as_struct.field_idxes;
    VecUSize *f2 = &t2->as_struct.field_idxes;
    if (f1->size != f2->size) {
        return false;
    }
    for (usize i = 0; i < f1->size; i++) {
        if (!CALL(TypeManager, *self, is_type_consistency, /, f1->data[i],
                  f2->data[i])) {
            return false;
        }
    }
    return true;
}
FUNC_STATIC bool MTD(TypeManager, is_type_fun_consistency, /, Type *t1,
                     Type *t2, bool fix) {
    VecUSize *r1 = &t1->as_fun.ret_par_idxes;
    VecUSize *r2 = &t2->as_fun.ret_par_idxes;
    if (r1->size != r2->size) {
        return false;
    }
    if (r1->size == 0) {
        return true;
    }

    if (!fix) {
        if (!CALL(TypeManager, *self, is_type_consistency, /, r1->data[0],
                  r2->data[0])) {
            return false;
        }
    }

    for (usize i = 1; i < r1->size; i++) {
        if (!CALL(TypeManager, *self, is_type_consistency, /, r1->data[i],
                  r2->data[i])) {
            return false;
        }
    }

    if (fix) {
        if (r1->data[0] == self->void_type_idx) {
            r1->data[0] = r2->data[0];
        } else if (r2->data[0] == self->void_type_idx) {
            r2->data[0] = r1->data[0];
        } else {
            ASSERT(r1->data[0] == r2->data[0]);
        }
    }
    return true;
}

static bool MTD(TypeManager, is_type_consistency_inner, /, usize type_idx1,
                usize type_idx2, bool fun_fix) {
    if (type_idx1 == self->void_type_idx || type_idx2 == self->void_type_idx) {
        return true;
    }
    // now types cannot be void

    Type *t1 = CALL(TypeManager, *self, get_type, /, type_idx1);
    Type *t2 = CALL(TypeManager, *self, get_type, /, type_idx2);

    if (t1->kind != t2->kind) {
        return false;
    }

    if (t1->kind == TypeKindInt || t1->kind == TypeKindFloat) {
        return true;
    }

    if (t1->kind == TypeKindArray) {
        return CALL(TypeManager, *self, is_type_array_consistency, /, t1, t2);
    } else if (t1->kind == TypeKindStruct) {
        return CALL(TypeManager, *self, is_type_struct_consistency, /, t1, t2);
    } else if (t1->kind == TypeKindFun) {
        return CALL(TypeManager, *self, is_type_fun_consistency, /, t1, t2,
                    fun_fix);
    } else {
        PANIC("Unknown kind");
    }
}

bool MTD(TypeManager, is_type_consistency, /, usize type_idx1,
         usize type_idx2) {
    return CALL(TypeManager, *self, is_type_consistency_inner, /, type_idx1,
                type_idx2, false);
}

DEFINE_CLASS_VEC(VecType, Type, FUNC_EXTERN);
bool MTD(TypeManager, is_type_consistency_with_fun_fix, /, usize type_idx1,
         usize type_idx2) {
    return CALL(TypeManager, *self, is_type_consistency_inner, /, type_idx1,
                type_idx2, true);
}

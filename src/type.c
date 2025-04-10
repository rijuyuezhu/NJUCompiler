#include "type.h"
#include "tem_vec.h"

void MTD(Type, clone_from, /, const Type *other) {
    *self = *other;
    if (self->kind == TypeKindStruct) {
        self->as_struct.field_idxes =
            CALL(VecUSize, other->as_struct.field_idxes, clone, /);
    } else if (self->kind == TypeKindFun) {
        self->as_fun.ret_param_idxes =
            CALL(VecUSize, other->as_fun.ret_param_idxes, clone, /);
    }
}

void MTD(Type, drop, /) {
    if (self->kind == TypeKindStruct) {
        DROPOBJ(VecUSize, self->as_struct.field_idxes);
    } else if (self->kind == TypeKindFun) {
        DROPOBJ(VecUSize, self->as_fun.ret_param_idxes);
    }
}

Type NSMTD(Type, make_array, /, TypeManager *manager, usize size,
           usize subtype_idx) {
    Type result = (Type){
        .kind = TypeKindArray,
        .repr_val = (usize)-1,
        .as_array =
            {
                .size = size,
                .subtype_idx = subtype_idx,
            },
    };
    Type *subtype = CALL(TypeManager, *manager, get_type, /, subtype_idx);
    if (subtype->kind == TypeKindArray) {
        result.as_array.dim = subtype->as_array.dim + 1;
    } else {
        result.as_array.dim = 1;
    }
    result.width = subtype->width * size;
    return result;
}

Type NSMTD(Type, make_struct, /, usize symtab_idx) {
    return (Type){
        .kind = TypeKindStruct,
        .repr_val = (usize)-1,
        .as_struct =
            {
                .field_idxes = CREOBJ(VecUSize, /),
                .symtab_idx = symtab_idx,
            },
        // for struct, the width is accumulated during add_struct_field
        .width = 0,
    };
}

void MTD(Type, add_struct_field, /, TypeManager *manager, usize field_idx) {
    ASSERT(self->kind == TypeKindStruct);
    CALL(VecUSize, self->as_struct.field_idxes, push_back, /, field_idx);
    Type *field = CALL(TypeManager, *manager, get_type, /, field_idx);
    self->width += field->width;
}

Type NSMTD(Type, make_fun, /) {
    return (Type){
        .kind = TypeKindFun,
        .repr_val = (usize)-1,
        .as_fun = {.ret_param_idxes = CREOBJ(VecUSize, /)},
        .width = 0,
        // width here is never used
    };
}

void MTD(Type, add_fun_ret_param, /, usize ret_param_idx) {
    ASSERT(self->kind == TypeKindFun);
    CALL(VecUSize, self->as_fun.ret_param_idxes, push_back, /, ret_param_idx);
}

int NSMTD(Type, compare, /, const Type *type1, const Type *type2) {
    if (type1->kind != type2->kind) {
        return NORMALCMP(type1->kind, type2->kind);
    }
    if (type1->kind == TypeKindInvalid || type1->kind == TypeKindInt ||
        type1->kind == TypeKindFloat || type1->kind == TypeKindVoid) {
        return 0;
    } else if (type1->kind == TypeKindArray) {
        // NOTE: the size of the array is not part of the comparison
        if (type1->as_array.dim != type2->as_array.dim) {
            return NORMALCMP(type1->as_array.dim, type2->as_array.dim);
        }
        return NORMALCMP(type1->as_array.subtype_idx,
                         type2->as_array.subtype_idx);
    } else if (type1->kind == TypeKindStruct) {
        const VecUSize *v1 = &type1->as_struct.field_idxes;
        const VecUSize *v2 = &type2->as_struct.field_idxes;
        for (usize i = 0; i < v1->size && i < v2->size; i++) {
            if (v1->data[i] != v2->data[i]) {
                return NORMALCMP(v1->data[i], v2->data[i]);
            }
        }
        return NORMALCMP(v1->size, v2->size);
    } else if (type1->kind == TypeKindFun) {
        const VecUSize *v1 = &type1->as_fun.ret_param_idxes;
        const VecUSize *v2 = &type2->as_fun.ret_param_idxes;
        for (usize i = 0; i < v1->size && i < v2->size; i++) {
            if (v1->data[i] != v2->data[i]) {
                return NORMALCMP(v1->data[i], v2->data[i]);
            }
        }
        return NORMALCMP(v1->size, v2->size);
    } else {
        PANIC("Unknown kind");
    }
}

void MTD(HType, rehash, /) {
    Type *t = &self->t;
    u64 hash = t->kind;
    const u64 BASE = 277;
    if (t->kind == TypeKindArray) {
        hash = hash * BASE + t->as_array.dim;
        hash = hash * BASE + t->as_array.subtype_idx;
    } else if (t->kind == TypeKindStruct) {
        const VecUSize *v = &t->as_struct.field_idxes;
        for (usize i = 0; i < v->size; i++) {
            hash = hash * BASE + v->data[i];
        }
    } else if (t->kind == TypeKindFun) {
        const VecUSize *v = &t->as_fun.ret_param_idxes;
        for (usize i = 0; i < v->size; i++) {
            hash = hash * BASE + v->data[i];
        }
    }
    self->stored_hash = hash;
}

int NSMTD(HType, compare, /, const HType *ht1, const HType *ht2) {
    if (ht1->stored_hash != ht2->stored_hash) {
        return NORMALCMP(ht1->stored_hash, ht2->stored_hash);
    }
    return NSCALL(Type, compare, /, &ht1->t, &ht2->t);
}

HType NSMTD(HType, from_inner, /, Type t) {
    HType ht = {.t = t};
    CALL(HType, ht, rehash, /);
    return ht;
}

void MTD(TypeManager, init, /) {
    CALL(VecType, self->types, init, /);
    CALL(MapHTypeUSize, self->repr_map, init, /);
    self->repr_cnt = 0;
    self->void_type_idx = CALL(TypeManager, *self, add_type, /,
                               (Type){
                                   .kind = TypeKindVoid,
                                   .repr_val = (usize)-1,
                               });
    self->int_type_idx = CALL(TypeManager, *self, add_type, /,
                              (Type){
                                  .kind = TypeKindInt,
                                  .repr_val = (usize)-1,
                              });
    self->float_type_idx = CALL(TypeManager, *self, add_type, /,
                                (Type){
                                    .kind = TypeKindFloat,
                                    .repr_val = (usize)-1,
                                });
    CALL(TypeManager, *self, fill_in_repr, /, self->void_type_idx);
    CALL(TypeManager, *self, fill_in_repr, /, self->int_type_idx);
    CALL(TypeManager, *self, fill_in_repr, /, self->float_type_idx);
}

void MTD(TypeManager, drop, /) {
    DROPOBJ(MapHTypeUSize, self->repr_map);
    DROPOBJ(VecType, self->types);
}

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
    CALL(Type, self->types.data[type_idx], add_struct_field, /, self,
         field_idx);
}

void MTD(TypeManager, add_fun_ret_param, /, usize type_idx,
         usize ret_param_idx) {
    CALL(Type, self->types.data[type_idx], add_fun_ret_param, /, ret_param_idx);
}

static bool MTD(TypeManager, is_type_array_consistency, /, Type *t1, Type *t2) {
    return t1->as_array.dim == t2->as_array.dim &&
           CALL(TypeManager, *self, is_type_consistency, /,
                t1->as_array.subtype_idx, t2->as_array.subtype_idx);
}

static bool MTD(TypeManager, is_type_struct_consistency, /, Type *t1,
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
static bool MTD(TypeManager, is_type_fun_consistency, /, Type *t1, Type *t2,
                bool fix) {
    VecUSize *r1 = &t1->as_fun.ret_param_idxes;
    VecUSize *r2 = &t2->as_fun.ret_param_idxes;
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

    if (t1->repr_val != (usize)-1 && t2->repr_val != (usize)-1) {
        return t1->repr_val == t2->repr_val;
    }

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

bool MTD(TypeManager, is_type_consistency_with_fun_fix, /, usize type_idx1,
         usize type_idx2) {
    return CALL(TypeManager, *self, is_type_consistency_inner, /, type_idx1,
                type_idx2, true);
}

void MTD(TypeManager, fill_in_repr, /, usize type_idx) {
    Type *t = CALL(TypeManager, *self, get_type, /, type_idx);
    if (t->repr_val != (usize)-1) {
        return;
    }

    Type repr_t = CALL(Type, *t, clone, /);

    if (t->kind == TypeKindArray) {
        Type *subtype =
            CALL(TypeManager, *self, get_type, /, t->as_array.subtype_idx);
        ASSERT(subtype->repr_val != (usize)-1);
        repr_t.as_array.subtype_idx = subtype->repr_val;
    } else if (t->kind == TypeKindStruct) {
        for (usize i = 0; i < t->as_struct.field_idxes.size; i++) {
            Type *field = CALL(TypeManager, *self, get_type, /,
                               t->as_struct.field_idxes.data[i]);
            ASSERT(field->repr_val != (usize)-1);
            repr_t.as_struct.field_idxes.data[i] = field->repr_val;
        }
    } else if (t->kind == TypeKindFun) {
        for (usize i = 0; i < t->as_fun.ret_param_idxes.size; i++) {
            Type *ret_param = CALL(TypeManager, *self, get_type, /,
                                   t->as_fun.ret_param_idxes.data[i]);
            ASSERT(ret_param->repr_val != (usize)-1);
            repr_t.as_fun.ret_param_idxes.data[i] = ret_param->repr_val;
        }
    } // else, basic type; no change

    HType repr_ht = NSCALL(HType, from_inner, /, repr_t);
    MapHTypeUSizeIterator it =
        CALL(MapHTypeUSize, self->repr_map, find, /, &repr_ht);
    if (it == NULL) {
        usize repr_val = self->repr_cnt++;
        MapHTypeUSizeInsertResult result =
            CALL(MapHTypeUSize, self->repr_map, insert, /, repr_ht, repr_val);
        ASSERT(result.inserted, "Insertion shall be successful");
        t->repr_val = repr_val;
    } else {
        t->repr_val = it->value;
        DROPOBJ(HType, repr_ht);
    }
}

Type *MTD(TypeManager, get_type, /, usize type_idx) {
    ASSERT(type_idx < self->types.size, "type_idx out of range");
    return &self->types.data[type_idx];
}

DEFINE_CLASS_VEC(VecType, Type, FUNC_EXTERN);
DEFINE_MAPPING(MapHTypeUSize, HType, usize, FUNC_EXTERN);

#include "type.h"
#include "tem_vec.h"

void MTD(Type, drop, /) {
    if (self->kind == TypeKindStruct) {
        DROPOBJ(VecUSize, self->as_struct.field_idxes);
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

Type NSMTD(Type, make_struct, /) {
    return (Type){.kind = TypeKindStruct,
                  .as_struct = {.field_idxes = CREOBJ(VecUSize, /)}};
}

void MTD(Type, add_struct_field, /, usize field_idx) {
    ASSERT(self->kind == TypeKindStruct);
    CALL(VecUSize, self->as_struct.field_idxes, push_back, /, field_idx);
}

usize MTD(TypeManager, add_type, /, Type type) {
    CALL(VecType, self->types, push_back, /, type);
    return self->types.size - 1;
}
void MTD(TypeManager, init, /) {
    CALL(VecType, self->types, init, /);
    self->int_type_idx =
        CALL(TypeManager, *self, add_type, /, (Type){.kind = TypeKindInt});
    self->float_type_idx =
        CALL(TypeManager, *self, add_type, /, (Type){.kind = TypeKindFloat});
}
void MTD(TypeManager, drop, /) { DROPOBJ(VecType, self->types); }

DEFINE_CLASS_VEC(VecType, Type, FUNC_EXTERN);

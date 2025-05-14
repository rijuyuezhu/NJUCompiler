#include "idx_allocator.h"
#include "general_container.h"

void MTD(IdxAllocator, init, /) {
    CALL(MapStrUSize, self->str_to_idx, init, /);
    self->avali_acc = 0;
}

void MTD(IdxAllocator, drop, /) {
    CALL(MapStrUSize, self->str_to_idx, drop, /);
}

usize MTD(IdxAllocator, get, /, String str) {
    HString hs = NSCALL(HString, from_inner, /, str);
    MapStrUSizeInsertResult res =
        CALL(MapStrUSize, self->str_to_idx, insert, /, hs, self->avali_acc);
    if (res.inserted) {
        self->avali_acc++;
    }
    return res.node->value;
}

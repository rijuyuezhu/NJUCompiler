#include "renamer.h"
#include "idx_allocator.h"

void MTD(Renamer, init, /, IdxAllocator *allocator) {
    CALL(MapUSizeUSize, self->rename_map, init, /);
    self->allocator = allocator;
}
void MTD(Renamer, drop, /) { CALL(MapUSizeUSize, self->rename_map, drop, /); }

void MTD(Renamer, rename, /, usize *obj) {
    if (*obj == (usize)-1) {
        return;
    }
    MapUSizeUSizeIterator it =
        CALL(MapUSizeUSize, self->rename_map, find_owned, /, *obj);
    if (it == NULL) {
        usize new_id = CALL(IdxAllocator, *self->allocator, allocate, /);
        MapUSizeUSizeInsertResult res =
            CALL(MapUSizeUSize, self->rename_map, insert, /, *obj, new_id);
        ASSERT(res.inserted);
        it = res.node;
    }
    *obj = it->value;
}

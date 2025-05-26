#include "renamer.h"

void MTD(Renamer, init, /, MapUSizeUSize *rename_map, IdxAllocator *allocator) {
    self->rename_map = rename_map;
    self->allocator = allocator;
}

void MTD(Renamer, rename, /, usize *obj) {
    if (*obj == (usize)-1) {
        return;
    }
    MapUSizeUSizeIterator it =
        CALL(MapUSizeUSize, *self->rename_map, find_owned, /, *obj);
    if (it == NULL) {
        usize new_id = CALL(IdxAllocator, *self->allocator, allocate, /);
        MapUSizeUSizeInsertResult res =
            CALL(MapUSizeUSize, *self->rename_map, insert, /, *obj, new_id);
        ASSERT(res.inserted);
        it = res.node;
    }
    *obj = it->value;
}

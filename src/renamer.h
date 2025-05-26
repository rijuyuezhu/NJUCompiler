#pragma once
#include "general_container.h"
#include "idx_allocator.h"

typedef struct Renamer {
    MapUSizeUSize *rename_map;
    IdxAllocator *allocator;
} Renamer;

void MTD(Renamer, init, /, MapUSizeUSize *rename_map, IdxAllocator *allocator);
FUNC_STATIC DEFAULT_DROPER(Renamer);
void MTD(Renamer, rename, /, usize *obj);

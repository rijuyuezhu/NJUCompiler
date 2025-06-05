#pragma once

#include "utils.h"

struct MapUSizeUSize;
struct IdxAllocator;
typedef struct Renamer {
    struct MapUSizeUSize *rename_map;
    struct IdxAllocator *allocator;
} Renamer;

void MTD(Renamer, init, /, struct MapUSizeUSize *rename_map,
         struct IdxAllocator *allocator);
FUNC_STATIC DEFAULT_DROPER(Renamer);
void MTD(Renamer, rename, /, usize *obj);

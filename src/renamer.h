#pragma once

#include "general_container.h"
#include "utils.h"

struct IdxAllocator;
typedef struct Renamer {
    MapUSizeUSize rename_map;
    struct IdxAllocator *allocator;
} Renamer;

void MTD(Renamer, init, /, struct IdxAllocator *allocator);
void MTD(Renamer, drop, /);
void MTD(Renamer, rename, /, usize *obj);

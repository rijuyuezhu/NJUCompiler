#pragma once
#include "general_container.h"

typedef struct IdxAllocator {
    MapStrUSize str_to_idx;
    usize avali_acc;
} IdxAllocator;

void MTD(IdxAllocator, init, /);
void MTD(IdxAllocator, drop, /);
usize MTD(IdxAllocator, get, /, String str);

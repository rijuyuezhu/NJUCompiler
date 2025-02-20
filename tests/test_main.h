#pragma once

#include "utils.h"

typedef struct TestEntry {
    const char *test_name;
    void (*test_fn)();
} TestEntry;

#define TESTENTRY(_name)                                                       \
    ({                                                                         \
        extern void CONCATENATE(test_, _name)();                               \
        TestEntry entry = {                                                    \
            .test_name = STRINGIFY(_name),                                     \
            .test_fn = CONCATENATE(test_, _name),                              \
        };                                                                     \
        entry;                                                                 \
    })

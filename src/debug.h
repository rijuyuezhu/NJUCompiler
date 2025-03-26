#pragma once

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

/// ASSERT always complains about the failed assertion, regardless of the NDEBUG
/// macro

#undef ASSERT
#define ASSERT(x, ...)                                                         \
    ({                                                                         \
        if (unlikely(!(x))) {                                                  \
            fprintf(stderr,                                                    \
                    "Assertion failed: `" #x                                   \
                    "`, at %s:%s:%d." VA_OPT_ONE(" %s", ##__VA_ARGS__) "\n",   \
                    __FILE__, __func__, __LINE__, ##__VA_ARGS__);              \
            abort();                                                           \
        }                                                                      \
    })

#undef PANIC
#define PANIC(...)                                                             \
    ({                                                                         \
        fprintf(stderr, "Panic: %s", __VA_ARGS__);                             \
        abort();                                                               \
    })

#undef ASSERT_EQ
#define ASSERT_EQ(x, y, fmtid, ...)                                            \
    ({                                                                         \
        typeof(x) MPROT(ASSERT_EQ_X) = (x);                                    \
        typeof(y) MPROT(ASSERT_EQ_Y) = (y);                                    \
        if (unlikely(MPROT(ASSERT_EQ_X) != MPROT(ASSERT_EQ_Y))) {              \
            printf("Assertion failed: `" #x "`=`" fmtid                        \
                   "` is not equal to `" #y "`=`" fmtid                        \
                   "`, at %s:%s:%d." VA_OPT_ONE(" %s", ##__VA_ARGS__) "\n",    \
                   MPROT(ASSERT_EQ_X), MPROT(ASSERT_EQ_Y), __FILE__, __func__, \
                   __LINE__, ##__VA_ARGS__);                                   \
            abort();                                                           \
        }                                                                      \
    })

#undef ASSERT_EQ_STR
#define ASSERT_EQ_STR(x, y, ...)                                               \
    ({                                                                         \
        const char *MPROT(ASSERT_EQ_X) = (x);                                  \
        const char *MPROT(ASSERT_EQ_Y) = (y);                                  \
        if (unlikely(strcmp(MPROT(ASSERT_EQ_X), MPROT(ASSERT_EQ_Y)) != 0)) {   \
            printf("Assertion failed: `" #x "`=`%s` is not equal to `" #y      \
                   "`=`%s`, "                                                  \
                   "at %s:%s:%d." VA_OPT_ONE(" %s", ##__VA_ARGS__) "\n",       \
                   MPROT(ASSERT_EQ_X), MPROT(ASSERT_EQ_Y), __FILE__, __func__, \
                   __LINE__, ##__VA_ARGS__);                                   \
            abort();                                                           \
        }                                                                      \
    })

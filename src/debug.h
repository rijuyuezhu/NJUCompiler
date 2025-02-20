#pragma once

#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

/// ASSERT always complains about the failed assertion, regardless of the NDEBUG
/// macro

#undef ASSERT
#define ASSERT(x)                                                              \
    ({                                                                         \
        if (!(x)) {                                                            \
            printf("Assertion failed: `" #x "`, at %s:%s:%d.\n", __FILE__,     \
                   __func__, __LINE__);                                        \
            abort();                                                           \
        }                                                                      \
    })

#undef ASSERT_EQ
#define ASSERT_EQ(x, y, fmtid)                                                 \
    ({                                                                         \
        typeof(x) MPROT(ASSERT_EQ_X) = (x);                                    \
        typeof(y) MPROT(ASSERT_EQ_Y) = (y);                                    \
        if (MPROT(ASSERT_EQ_X) != MPROT(ASSERT_EQ_Y)) {                        \
            printf("Assertion failed: `" #x "`=`" fmtid                        \
                   "` is not equal to `" #y "`=`" fmtid "`, at %s:%s:%d.\n",   \
                   MPROT(ASSERT_EQ_X), MPROT(ASSERT_EQ_Y), __FILE__, __func__, \
                   __LINE__);                                                  \
            abort();                                                           \
        }                                                                      \
    })

#undef ASSERT_EQ_STR
#define ASSERT_EQ_STR(x, y)                                                    \
    ({                                                                         \
        const char *MPROT(ASSERT_EQ_X) = (x);                                  \
        const char *MPROT(ASSERT_EQ_Y) = (y);                                  \
        if (strcmp(MPROT(ASSERT_EQ_X), MPROT(ASSERT_EQ_Y)) != 0) {             \
            printf("Assertion failed: `" #x "`=`%s` is not equal to `" #y      \
                   "`=`%s`, "                                                  \
                   "at %s:%s:%d.\n",                                           \
                   MPROT(ASSERT_EQ_X), MPROT(ASSERT_EQ_Y), __FILE__, __func__, \
                   __LINE__);                                                  \
            abort();                                                           \
        }                                                                      \
    })

/// DEBUG_ASSERT only complains about the failed assertion if the DEBUG_ENABLE
/// macro is defined
#undef DEBUG_ASSERT
#ifdef DEBUG_ENABLE
#define DEBUG_ASSERT(x, ...) ASSERT(x, ##__VA_ARGS__)
#else
#define DEBUG_ASSERT(x, ...) ((void)0)
#endif

/// LOG always prints the message.
#undef LOG
#define LOG(format, ...)                                                       \
    printf("\033[1;33mLOG:%s:%d\033[0;m " format "\n", __FILE__, __LINE__,     \
           ##__VA_ARGS__)

/// DEBUG_LOG only prints the message if the DEBUG_ENABLE macro is defined
#undef DEBUG_LOG
#ifdef DEBUG_ENABLE
#define DEBUG_LOG(format, ...)                                                 \
    printf("\033[1;33mDEBUG:%s:%d\033[0;m " format "\n", __FILE__, __LINE__,   \
           ##__VA_ARGS__)
#else
#define DEBUG_LOG(format, ...) ((void)0)
#endif

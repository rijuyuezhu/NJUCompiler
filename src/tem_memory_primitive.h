#pragma once

#include "utils.h"

#undef GENERATOR_PLAIN_COMPARATOR
#define GENERATOR_PLAIN_COMPARATOR(Container, K)                               \
    FUNC_STATIC int NSMTD(Container, comparator, /, const typeof(K) *MPROT(a), \
                          const typeof(K) *MPROT(b)) {                         \
        return *MPROT(a) < *MPROT(b) ? -1 : (*MPROT(a) > *MPROT(b) ? 1 : 0);   \
    }

#undef GENERATOR_CLASS_COMPARATOR
#define GENERATOR_CLASS_COMPARATOR(Container, K)                               \
    FUNC_STATIC int NSMTD(Container, comparator, /, const typeof(K) *MPROT(a), \
                          const typeof(K) *MPROT(b)) {                         \
        return NSCALL(K, compare, /, MPROT(a), MPROT(b));                      \
    }

#undef GENERATOR_CUSTOM_COMPARATOR
#define GENERATOR_CUSTOM_COMPARATOR(Container, K)

#undef GENERATOR_PLAIN_KEY
#define GENERATOR_PLAIN_KEY(Container, K)                                      \
    FUNC_STATIC void NSMTD(Container, drop_key, /,                             \
                           ATTR_UNUSED typeof(K) *MPROT(key)) {}               \
    FUNC_STATIC typeof(K) NSMTD(Container, clone_key, /,                       \
                                const typeof(K) *MPROT(other)) {               \
        return *MPROT(other);                                                  \
    }

#undef GENERATOR_CLASS_KEY
#define GENERATOR_CLASS_KEY(Container, K)                                      \
    FUNC_STATIC void NSMTD(Container, drop_key, /, typeof(K) *MPROT(key)) {    \
        CALL(K, *MPROT(key), drop, /);                                         \
    }                                                                          \
    FUNC_STATIC typeof(K) NSMTD(Container, clone_key, /,                       \
                                const typeof(K) *MPROT(other)) {               \
        return CALL(K, *MPROT(other), clone, /);                               \
    }

#undef GENERATOR_CUSTOM_KEY
#define GENERATOR_CUSTOM_KEY(Container, K)

#undef GENERATOR_PLAIN_VALUE
#define GENERATOR_PLAIN_VALUE(Container, V)                                    \
    FUNC_STATIC void NSMTD(Container, drop_value, /,                           \
                           ATTR_UNUSED typeof(V) *MPROT(value)) {}             \
    FUNC_STATIC typeof(V) NSMTD(Container, clone_value, /,                     \
                                const typeof(V) *MPROT(other)) {               \
        return *MPROT(other);                                                  \
    }

#undef GENERATOR_CLASS_VALUE
#define GENERATOR_CLASS_VALUE(Container, V)                                    \
    FUNC_STATIC void NSMTD(Container, drop_value, /,                           \
                           typeof(V) *MPROT(value)) {                          \
        CALL(V, *MPROT(value), drop, /);                                       \
    }                                                                          \
    FUNC_STATIC typeof(V) NSMTD(Container, clone_value, /,                     \
                                const typeof(V) *MPROT(other)) {               \
        return CALL(V, *MPROT(other), clone, /);                               \
    }

#undef GENERATOR_CUSTOM_VALUE
#define GENERATOR_CUSTOM_VALUE(Container, V)

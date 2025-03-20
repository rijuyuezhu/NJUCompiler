#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* utils */

/// Macros

#undef STRINGIFY_INNER
#define STRINGIFY_INNER(x) #x
#undef STRINGIFY
#define STRINGIFY(x) STRINGIFY_INNER(x)
#undef CONCATENATE_INNER
#define CONCATENATE_INNER(x, y) x##y
#undef CONCATENATE
#define CONCATENATE(x, y) CONCATENATE_INNER(x, y)
#undef CONCATENATE3
#define CONCATENATE3(x, y, z) CONCATENATE(CONCATENATE(x, y), z)
#undef CONCATENATE4
#define CONCATENATE4(x, y, z, w) CONCATENATE(CONCATENATE3(x, y, z), w)
#undef VA_OPT_IDENTITY
#define VA_OPT_IDENTITY(x) x
#undef VA_OPT_EMPTY
#define VA_OPT_EMPTY(x)
#undef CHOOSE_3rd
#define CHOOSE_3rd(a, b, c, ...) c
#undef VA_OPT_ONE
#define VA_OPT_ONE(s, ...)                                                     \
    CHOOSE_3rd(_0, ##__VA_ARGS__, VA_OPT_IDENTITY, VA_OPT_EMPTY)(s)

#undef typeof
#define typeof __typeof__

#undef LENGTH
#define LENGTH(array) (sizeof(array) / sizeof((array)[0]))

#undef ATTR_UNUSED
#define ATTR_UNUSED __attribute__((unused))
#undef UNREACHABLE
#define UNREACHABLE __builtin_unreachable()
#undef FUNC_STATIC
#define FUNC_STATIC ATTR_UNUSED static inline
#undef FUNC_EXTERN
#define FUNC_EXTERN extern

/// Macro protector
#undef MPROT
#define MPROT(name) CONCATENATE(RJYZ_INNER_MPROT_, name)

/// Common algorithms
#undef Max
#define Max(x, y)                                                              \
    ({                                                                         \
        typeof(x) MPROT(MAX_X) = (x);                                          \
        typeof(y) MPROT(MAX_Y) = (y);                                          \
        MPROT(MAX_X) > MPROT(MAX_Y) ? MPROT(MAX_X) : MPROT(MAX_Y);             \
    })

#undef Min
#define Min(x, y)                                                              \
    ({                                                                         \
        typeof(x) MPROT(MIN_X) = (x);                                          \
        typeof(y) MPROT(MIN_Y) = (y);                                          \
        MPROT(MIN_X) < MPROT(MIN_Y) ? MPROT(MIN_X) : MPROT(MIN_Y);             \
    })

/* method call support */

/// MTDNAME defines the name of the method
#undef MTDNAME
#define MTDNAME(ns, func) CONCATENATE4(RJYZ_METHOD__, ns, __, func)

/// NSMTD is used to functions in a namespace
#undef NSMTD
#define NSMTD(ns, func, slash, ...) MTDNAME(ns, func)(__VA_ARGS__)

/// MTD is used to define the signature of a method; MTDCONST is used to define
/// the signature of a const method (not including the retval)
#undef MTD
#define MTD(cls, method, slash, ...)                                           \
    NSMTD(cls, method, slash, ATTR_UNUSED typeof(cls) *self, ##__VA_ARGS__)

#undef MTDCONST
#define MTDCONST(cls, method, slash, ...)                                      \
    NSMTD(cls, method, slash, ATTR_UNUSED const typeof(cls) *self,             \
          ##__VA_ARGS__)

/// NSCALL is used to call a function in a namespace
#undef NSCALL
#define NSCALL(ns, func, slash, ...) MTDNAME(ns, func)(__VA_ARGS__)

/// CALL is used to call a method
#undef CALL
#define CALL(cls, self, method, slash, ...)                                    \
    NSCALL(cls, method, slash, &(self), ##__VA_ARGS__)

/// CREOBJ and DROPOBJ are used to create and drop an object; CREOBJHEAP and
/// DROPOBJ are used to create and drop an object on heap
#undef CREOBJ
#define CREOBJ(cls, slash, ...)                                                \
    ({                                                                         \
        typeof(cls) MPROT(temp);                                               \
        CALL(cls, MPROT(temp), init, /, ##__VA_ARGS__);                        \
        MPROT(temp);                                                           \
    })

#undef DROPOBJ
#define DROPOBJ(cls, expr) CALL(cls, (expr), drop, /)

#undef CREOBJRAWHEAP
#define CREOBJRAWHEAP(cls)                                                     \
    ({                                                                         \
        typeof(cls) *MPROT(temp) = (typeof(cls) *)malloc(sizeof(cls));         \
        ASSERT(MPROT(temp));                                                   \
        MPROT(temp);                                                           \
    })

#undef CREOBJHEAP
#define CREOBJHEAP(cls, slash, ...)                                            \
    ({                                                                         \
        typeof(cls) *MPROT(temp) = (typeof(cls) *)malloc(sizeof(cls));         \
        ASSERT(MPROT(temp));                                                   \
        CALL(cls, *MPROT(temp), init, /, ##__VA_ARGS__);                       \
        MPROT(temp);                                                           \
    })

#undef DROPOBJHEAP
#define DROPOBJHEAP(cls, ptr)                                                  \
    ({                                                                         \
        typeof(cls) *MPROT(temp) = (ptr);                                      \
        if (MPROT(temp)) {                                                     \
            CALL(cls, *MPROT(temp), drop, /);                                  \
            free(MPROT(temp));                                                 \
        }                                                                      \
    })

/// helper macros for defining default initializers and dropers
#undef DEFAULT_INITIALIZER
#define DEFAULT_INITIALIZER(cls)                                               \
    void MTD(cls, init, /) {}

#undef DEFAULT_DROPER
#define DEFAULT_DROPER(cls)                                                    \
    void MTD(cls, drop, /) {}

#undef DEFAULT_DERIVE_CLONE
#define DEFAULT_DERIVE_CLONE(cls, slash, ...)                                  \
    typeof(cls) MTDCONST(cls, clone, /) {                                      \
        typeof(cls) MPROT(default_derive_clone_temp) =                         \
            CREOBJ(cls, /, ##__VA_ARGS__);                                     \
        CALL(cls, MPROT(default_derive_clone_temp), clone_from, /, self);      \
        return MPROT(default_derive_clone_temp);                               \
    }

#undef DEFAULT_CLONER
#define DEFAULT_CLONER(cls)                                                    \
    void MTD(cls, clone_from, /, const typeof(cls) *MPROT(other)) {            \
        *self = *MPROT(other);                                                 \
    }

#undef DELETED_CLONER
#define DELETED_CLONER(cls, STORAGE)                                           \
    STORAGE void MTD(cls, clone_from, /,                                       \
                     ATTR_UNUSED const typeof(cls) *MPROT(other)) {            \
        PANIC("Cloning is not allowed for " #cls);                             \
        UNREACHABLE;                                                           \
    }                                                                          \
    STORAGE typeof(cls) MTDCONST(cls, clone, /) {                              \
        PANIC("Cloning is not allowed for " #cls);                             \
        UNREACHABLE;                                                           \
    }

/* type definitions */

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef ptrdiff_t isize;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;

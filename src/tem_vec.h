// clang-format off
/// vec_template.h: provides a template for implementing a vector-like data structure.
///
/// Macros:
///     DECLARE_PLAIN_VEC(Vec, T, STORAGE): declare a plain vector-like data structure.
///     DEFINE_PLAIN_VEC(Vec, T, STORAGE): define a plain vector-like data structure.
///     DECLARE_CLASS_VEC(Vec, T, STORAGE): declare a class vector-like data structure.
///     DEFINE_CLASS_VEC(Vec, T, STORAGE): define a class vector-like data structure.
///
///     Here STORAGE is either `FUNC_STATIC` or `FUNC_EXTERN`
///
/// Plain Vec Methods:
///     Vec.init(): initialize the vector.
///     Vec.drop(): drop the vector.
///     Vec.clone_from(const Vec *other): clone the vector from another vector.
///     Vec.clone() const -> Vec: clone the vector.
///     Vec.reserve(usize new_cap): reserve the capacity of the vector.
///     Vec.insert(usize to_index, T elem): insert an element at the specified index.
///     Vec.erase(usize index): erase an element at the specified index.
///     Vec.push_back(T elem): push an element to the back of the vector.
///     Vec.pop_back(): pop an element from the back of the vector.
///     Vec.resize(usize new_size): resize the vector to the specified size.
///     Vec.swap(Vec *other): swap the vector with another vector.
///     Vec.shrink_to_fit(): shrink the capacity of the vector to its size.
///     Vec.empty() -> bool: check if the vector is empty.
///     Vec.clear(): clear the vector.
///     Vec.at(usize index) -> T *: get the element at the specified index.
///     Vec.front() -> T *: get the first element of the vector.
///     Vec.back() -> T *: get the last element of the vector.
///
/// Class Vec Methods:
///    Vec.init(): initialize the vector.
///    Vec.drop(): drop the vector.
///    Vec.clone_from(const Vec *other): clone the vector from another vector.
///    Vec.clone() const -> Vec: clone the vector.
///    Vec.reserve(usize new_cap): reserve the capacity of the vector.
///    Vec.insert(usize to_index, T elem): insert an element at the specified index.
///    Vec.erase(usize index): erase an element at the specified index.
///    Vec.push_back(T elem): push an element to the back of the vector.
///    Vec.pop_back(): pop an element from the back of the vector.
///    Vec.resize(usize new_size, T padding): resize the vector to the specified size.
///    Vec.truncate(usize limit): truncate the vector to the specified limit.
///    Vec.swap(Vec *other): swap the vector with another vector.
///    Vec.shrink_to_fit(): shrink the capacity of the vector to its size.
///    Vec.empty() -> bool: check if the vector is empty.
///    Vec.clear(): clear the vector.
///    Vec.at(usize index) -> T *: get the element at the specified index.
///    Vec.front() -> T *: get the first element of the vector.
///    Vec.back() -> T *: get the last element of the vector.
// clang-format on

#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "utils.h"

/// declare at .h files
#undef DECLARE_PLAIN_VEC
#define DECLARE_PLAIN_VEC(Vec, T, STORAGE)                                     \
    DECLARE_PLAIN_VEC_INNER(Vec, typeof(T), STORAGE)

#undef DECLARE_PLAIN_VEC_INNER
#define DECLARE_PLAIN_VEC_INNER(Vec, T, STORAGE)                               \
    typedef struct Vec {                                                       \
        T *data;                                                               \
        usize size;                                                            \
        usize capacity;                                                        \
    } Vec;                                                                     \
                                                                               \
    /* NOTE: Methods to implement in .c */                                     \
                                                                               \
    /* Vec.clone_from(const Vec *other) */                                     \
    STORAGE void MTD(Vec, clone_from, /, const Vec *other);                    \
                                                                               \
    /* Vec.drop() */                                                           \
    STORAGE void MTD(Vec, drop, /);                                            \
                                                                               \
    /* Vec.reserve(usize new_cap) */                                           \
    STORAGE void MTD(Vec, reserve, /, usize new_cap);                          \
                                                                               \
    /* Vec.insert(usize to_index, T elem) */                                   \
    STORAGE void MTD(Vec, insert, /, usize to_index, T elem);                  \
                                                                               \
    /* Vec.erase(usize index) */                                               \
    STORAGE void MTD(Vec, erase, /, usize index);                              \
                                                                               \
    /* Vec.push_back(T elem) */                                                \
    STORAGE void MTD(Vec, push_back, /, T elem);                               \
                                                                               \
    /* Vec.pop_back() */                                                       \
    STORAGE void MTD(Vec, pop_back, /);                                        \
                                                                               \
    /* Vec.resize(usize new_size) */                                           \
    STORAGE void MTD(Vec, resize, /, usize new_size);                          \
                                                                               \
    /* Vec.swap(Vec *other) */                                                 \
    STORAGE void MTD(Vec, swap, /, Vec * other);                               \
                                                                               \
    /* Vec.shrink_to_fit() */                                                  \
    STORAGE void MTD(Vec, shrink_to_fit, /);                                   \
                                                                               \
    /* NOTE: static methods that are not required to implement in .c */        \
                                                                               \
    /* Vec.init() */                                                           \
    FUNC_STATIC void MTD(Vec, init, /) {                                       \
        self->data = NULL;                                                     \
        self->size = 0;                                                        \
        self->capacity = 0;                                                    \
    }                                                                          \
                                                                               \
    /* Vec.clone() const -> Vec */                                             \
    FUNC_STATIC DEFAULT_DERIVE_CLONE(Vec, /);                                  \
                                                                               \
    /* Vec.empty() -> bool */                                                  \
    FUNC_STATIC bool MTD(Vec, empty, /) { return self->size == 0; }            \
                                                                               \
    /* Vec.clear() */                                                          \
    FUNC_STATIC void MTD(Vec, clear, /) { self->size = 0; }                    \
                                                                               \
    /* Vec.at(usize index) -> T* */                                            \
    FUNC_STATIC T *MTD(Vec, at, /, usize index) {                              \
        ASSERT(index < self->size);                                            \
        return self->data + index;                                             \
    }                                                                          \
                                                                               \
    /* Vec.front() -> T * */                                                   \
    FUNC_STATIC T *MTD(Vec, front, /) { return CALL(Vec, *self, at, /, 0); }   \
                                                                               \
    /* Vec.back() -> T * */                                                    \
    FUNC_STATIC T *MTD(Vec, back, /) {                                         \
        return CALL(Vec, *self, at, /, self->size - 1);                        \
    }

/// define at .c files
#undef DEFINE_PLAIN_VEC
#define DEFINE_PLAIN_VEC(Vec, T, STORAGE)                                      \
    DEFINE_PLAIN_VEC_INNER(Vec, typeof(T), STORAGE)

#undef DEFINE_PLAIN_VEC_INNER
#define DEFINE_PLAIN_VEC_INNER(Vec, T, STORAGE)                                \
    STORAGE void MTD(Vec, clone_from, /, const Vec *MPROT(other)) {            \
        if (self == MPROT(other)) {                                            \
            return;                                                            \
        }                                                                      \
        CALL(Vec, *self, reserve, /, MPROT(other)->size);                      \
        self->size = MPROT(other)->size;                                       \
        if (MPROT(other)->size > 0) {                                          \
            memcpy(self->data, MPROT(other)->data,                             \
                   MPROT(other)->size * sizeof(T));                            \
        }                                                                      \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, drop, /) {                                           \
        free(self->data);                                                      \
        self->data = NULL;                                                     \
        self->size = 0;                                                        \
        self->capacity = 0;                                                    \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, reserve, /, usize MPROT(new_cap)) {                  \
        if (MPROT(new_cap) <= self->capacity) {                                \
            return;                                                            \
        }                                                                      \
        ASSERT(MPROT(new_cap) > 0);                                            \
        self->data = (T *)realloc(self->data, MPROT(new_cap) * sizeof(T));     \
        ASSERT(self->data);                                                    \
        memset(self->data + self->capacity, 0,                                 \
               (MPROT(new_cap) - self->capacity) * sizeof(T));                 \
        self->capacity = MPROT(new_cap);                                       \
    }                                                                          \
                                                                               \
    static usize NSMTD(Vec, num_from_size, /) {                                \
        usize MPROT(size) = sizeof(T);                                         \
        /* For tiny vectors. From Rust's alloc::raw_vec::min_non_zero_cap */   \
        if (MPROT(size) == 1) {                                                \
            return 8;                                                          \
        } else if (MPROT(size) <= 1024) {                                      \
            return 4;                                                          \
        } else {                                                               \
            return 1;                                                          \
        }                                                                      \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, check_expansion, /) {                                \
        if (self->size < self->capacity) {                                     \
            return;                                                            \
        }                                                                      \
        CALL(Vec, *self, reserve, /,                                           \
             Max(NSCALL(Vec, num_from_size, /), self->capacity * 2));          \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, insert, /, usize MPROT(to_index), T MPROT(elem)) {   \
        ASSERT(MPROT(to_index) <= self->size);                                 \
        if (MPROT(to_index) == self->size) {                                   \
            CALL(Vec, *self, push_back, /, MPROT(elem));                       \
            return;                                                            \
        }                                                                      \
        CALL(Vec, *self, check_expansion, /);                                  \
        memmove(self->data + MPROT(to_index) + 1,                              \
                self->data + MPROT(to_index),                                  \
                (self->size - MPROT(to_index)) * sizeof(T));                   \
        self->data[MPROT(to_index)] = MPROT(elem);                             \
        self->size++;                                                          \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, erase, /, usize MPROT(index)) {                      \
        ASSERT(MPROT(index) < self->size);                                     \
        if (MPROT(index) + 1 == self->size) {                                  \
            CALL(Vec, *self, pop_back, /);                                     \
            return;                                                            \
        }                                                                      \
        memmove(self->data + MPROT(index), self->data + MPROT(index) + 1,      \
                (self->size - MPROT(index) - 1) * sizeof(T));                  \
        self->size--;                                                          \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, push_back, /, T MPROT(elem)) {                       \
        CALL(Vec, *self, check_expansion, /);                                  \
        self->data[self->size++] = MPROT(elem);                                \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, pop_back, /) {                                       \
        ASSERT(self->size > 0);                                                \
        self->size--;                                                          \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, resize, /, usize MPROT(new_size)) {                  \
        CALL(Vec, *self, reserve, /, MPROT(new_size));                         \
        self->size = MPROT(new_size);                                          \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, swap, /, Vec * MPROT(other)) {                       \
        if (self == MPROT(other)) {                                            \
            return;                                                            \
        }                                                                      \
        Vec MPROT(tmp) = *self;                                                \
        *self = *MPROT(other);                                                 \
        *MPROT(other) = MPROT(tmp);                                            \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, shrink_to_fit, /) {                                  \
        if (self->size == self->capacity) {                                    \
            return;                                                            \
        }                                                                      \
        self->data = (T *)realloc(self->data, self->size * sizeof(T));         \
        ASSERT(self->data);                                                    \
        self->capacity = self->size;                                           \
    }

/// declare at .h files

#undef DECLARE_CLASS_VEC
#define DECLARE_CLASS_VEC(Vec, T, STORAGE)                                     \
    typedef struct Vec {                                                       \
        T *data;                                                               \
        usize size;                                                            \
        usize capacity;                                                        \
    } Vec;                                                                     \
                                                                               \
    /* NOTE: Methods to implement in .c */                                     \
                                                                               \
    /* Vec.clone_from(const Vec *other) */                                     \
    STORAGE void MTD(Vec, clone_from, /, const Vec *other);                    \
                                                                               \
    /* Vec.drop() */                                                           \
    STORAGE void MTD(Vec, drop, /);                                            \
                                                                               \
    /* Vec.reserve(usize new_cap) */                                           \
    STORAGE void MTD(Vec, reserve, /, usize new_cap);                          \
                                                                               \
    /* Vec.clear() */                                                          \
    STORAGE void MTD(Vec, clear, /);                                           \
                                                                               \
    /* Vec.insert(usize to_index, T elem) */                                   \
    STORAGE void MTD(Vec, insert, /, usize to_index, T elem);                  \
                                                                               \
    /* Vec.erase(usize index) */                                               \
    STORAGE void MTD(Vec, erase, /, usize index);                              \
                                                                               \
    /* Vec.push_back(T elem) */                                                \
    STORAGE void MTD(Vec, push_back, /, T elem);                               \
                                                                               \
    /* Vec.pop_back() */                                                       \
    STORAGE void MTD(Vec, pop_back, /);                                        \
                                                                               \
    /* Vec.resize(usize new_size, T padding) */                                \
    STORAGE void MTD(Vec, resize, /, usize new_size, T padding);               \
                                                                               \
    /* Vec.truncate(usize limit) */                                            \
    STORAGE void MTD(Vec, truncate, /, usize limit);                           \
                                                                               \
    /* Vec.swap(Vec *other) */                                                 \
    STORAGE void MTD(Vec, swap, /, Vec * other);                               \
                                                                               \
    /* Vec.shrink_to_fit() */                                                  \
    STORAGE void MTD(Vec, shrink_to_fit, /);                                   \
                                                                               \
    /* NOTE: static methods that are not required to implement in .c */        \
                                                                               \
    /* Vec.init() */                                                           \
    FUNC_STATIC void MTD(Vec, init, /) {                                       \
        self->data = NULL;                                                     \
        self->size = 0;                                                        \
        self->capacity = 0;                                                    \
    }                                                                          \
                                                                               \
    /* Vec.clone() const -> Vec */                                             \
    FUNC_STATIC DEFAULT_DERIVE_CLONE(Vec, /);                                  \
                                                                               \
    /* Vec.empty() -> bool */                                                  \
    FUNC_STATIC bool MTD(Vec, empty, /) { return self->size == 0; }            \
                                                                               \
    /* Vec.at(usize index) -> T * */                                           \
    FUNC_STATIC T *MTD(Vec, at, /, usize index) {                              \
        ASSERT(index < self->size);                                            \
        return self->data + index;                                             \
    }                                                                          \
                                                                               \
    /* Vec.front() -> T * */                                                   \
    FUNC_STATIC T *MTD(Vec, front, /) { return CALL(Vec, *self, at, /, 0); }   \
                                                                               \
    /* Vec.back() -> T * */                                                    \
    FUNC_STATIC T *MTD(Vec, back, /) {                                         \
        return CALL(Vec, *self, at, /, self->size - 1);                        \
    }

/// define at .c files
#undef DEFINE_CLASS_VEC
#define DEFINE_CLASS_VEC(Vec, T, STORAGE)                                      \
    STORAGE void MTD(Vec, clone_from, /, const Vec *MPROT(other)) {            \
        if (self == MPROT(other)) {                                            \
            return;                                                            \
        }                                                                      \
        for (usize i = 0; i < self->size; i++) {                               \
            CALL(T, self->data[i], drop, /);                                   \
        }                                                                      \
        CALL(Vec, *self, reserve, /, MPROT(other)->size);                      \
        self->size = MPROT(other)->size;                                       \
        for (usize i = 0; i < self->size; i++) {                               \
            self->data[i] = CALL(T, MPROT(other)->data[i], clone, /);          \
        }                                                                      \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, drop, /) {                                           \
        for (usize i = 0; i < self->size; i++) {                               \
            CALL(T, self->data[i], drop, /);                                   \
        }                                                                      \
        free(self->data);                                                      \
        self->data = NULL;                                                     \
        self->size = 0;                                                        \
        self->capacity = 0;                                                    \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, reserve, /, usize MPROT(new_cap)) {                  \
        if (MPROT(new_cap) <= self->capacity) {                                \
            return;                                                            \
        }                                                                      \
        self->data = (T *)realloc(self->data, MPROT(new_cap) * sizeof(T));     \
        ASSERT(self->data);                                                    \
        memset(self->data + self->capacity, 0,                                 \
               (MPROT(new_cap) - self->capacity) * sizeof(T));                 \
        self->capacity = MPROT(new_cap);                                       \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, clear, /) {                                          \
        for (usize i = 0; i < self->size; i++) {                               \
            CALL(T, self->data[i], drop, /);                                   \
        }                                                                      \
        self->size = 0;                                                        \
    }                                                                          \
                                                                               \
    STORAGE usize NSMTD(Vec, num_from_size, /) {                               \
        usize MPROT(size) = sizeof(T);                                         \
        /* For tiny vectors. From Rust's alloc::raw_vec::min_non_zero_cap*/    \
        if (MPROT(size) == 1) {                                                \
            return 8;                                                          \
        } else if (MPROT(size) <= 1024) {                                      \
            return 4;                                                          \
        } else {                                                               \
            return 1;                                                          \
        }                                                                      \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, check_expansion, /) {                                \
        if (self->size < self->capacity) {                                     \
            return;                                                            \
        }                                                                      \
        CALL(Vec, *self, reserve, /,                                           \
             Max(NSCALL(Vec, num_from_size, /), self->capacity * 2));          \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, insert, /, usize MPROT(to_index), T MPROT(elem)) {   \
        ASSERT(MPROT(to_index) <= self->size);                                 \
        if (MPROT(to_index) == self->size) {                                   \
            CALL(Vec, *self, push_back, /, MPROT(elem));                       \
            return;                                                            \
        }                                                                      \
        CALL(Vec, *self, check_expansion, /);                                  \
        memmove(self->data + MPROT(to_index) + 1,                              \
                self->data + MPROT(to_index),                                  \
                (self->size - MPROT(to_index)) * sizeof(T));                   \
        self->data[MPROT(to_index)] = MPROT(elem);                             \
        self->size++;                                                          \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, erase, /, usize MPROT(index)) {                      \
        ASSERT(MPROT(index) < self->size);                                     \
        if (MPROT(index) + 1 == self->size) {                                  \
            CALL(Vec, *self, pop_back, /);                                     \
            return;                                                            \
        }                                                                      \
        CALL(T, self->data[MPROT(index)], drop, /);                            \
        memmove(self->data + MPROT(index), self->data + MPROT(index) + 1,      \
                (self->size - MPROT(index) - 1) * sizeof(T));                  \
        self->size--;                                                          \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, push_back, /, T MPROT(elem)) {                       \
        CALL(Vec, *self, check_expansion, /);                                  \
        self->data[self->size++] = MPROT(elem);                                \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, pop_back, /) {                                       \
        ASSERT(self->size > 0);                                                \
        self->size--;                                                          \
        CALL(T, self->data[self->size], drop, /);                              \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, resize, /, usize MPROT(new_size),                    \
                     T MPROT(padding)) {                                       \
        CALL(Vec, *self, reserve, /, MPROT(new_size));                         \
        if (MPROT(new_size) < self->size) {                                    \
            for (usize i = MPROT(new_size); i < self->size; i++) {             \
                CALL(T, self->data[i], drop, /);                               \
            }                                                                  \
        } else if (MPROT(new_size) > self->size) {                             \
            for (usize i = self->size; i + 1 < MPROT(new_size); i++) {         \
                self->data[i] = CALL(T, MPROT(padding), clone, /);             \
            }                                                                  \
            self->data[MPROT(new_size) - 1] = MPROT(padding);                  \
        }                                                                      \
        self->size = MPROT(new_size);                                          \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, truncate, /, usize MPROT(limit)) {                   \
        if (MPROT(limit) >= self->size) {                                      \
            return;                                                            \
        }                                                                      \
        for (usize i = MPROT(limit); i < self->size; i++) {                    \
            CALL(T, self->data[i], drop, /);                                   \
        }                                                                      \
        self->size = MPROT(limit);                                             \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, swap, /, Vec * MPROT(other)) {                       \
        if (self == MPROT(other)) {                                            \
            return;                                                            \
        }                                                                      \
        Vec MPROT(tmp) = *self;                                                \
        *self = *MPROT(other);                                                 \
        *MPROT(other) = MPROT(tmp);                                            \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Vec, shrink_to_fit, /) {                                  \
        if (self->size == self->capacity) {                                    \
            return;                                                            \
        }                                                                      \
        self->data = (T *)realloc(self->data, self->size * sizeof(T));         \
        ASSERT(self->data);                                                    \
        self->capacity = self->size;                                           \
    }

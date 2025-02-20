// clang-format off
/// mapping.h: provides a template for implementing a mapping-like data structure.
///
/// Macros:
///     DECLARE_MAPPING(Mapping, K, V, STORAGE, key_gen, value_gen, com_gen): declare a mapping.
///         key_gen: define the key generator.
///         - GENERATOR_PLAIN_KEY: define a plain key generator.
///         - GENERATOR_CLASS_KEY: define a class key generator.
///         value_gen: define the value generator.
///         - GENERATOR_PLAIN_VALUE: define a plain value generator.
///         - GENERATOR_CLASS_VALUE: define a class value generator.
///         com_gen: define the comparator generator.
///         - GENERATOR_PLAIN_COMPARATOR: define a plain comparator generator.
///         - GENERATOR_CLASS_COMPARATOR: define a class comparator generator.
///         - GENERATOR_CUSTOM_COMPARATOR: define a custom comparator generator.
///     DEFINE_MAPPING(Mapping, K, V, STORAGE): define a mapping.
///
///     Here STORAGE is either `FUNC_STATIC` or `FUNC_EXTERN`
///
/// Mapping Methods:
///     Mapping.init(): initialize the mapping.
///     Mapping.drop(): drop the mapping.
///     Mapping.clone_from(const Mapping *other): clone the mapping from another mapping.
///     Mapping.clone() const -> Mapping: clone the mapping.
///     Mapping.insert(K key, V value) -> MappingInsertResult: insert a key-value pair.
///     Mapping.insert_or_assign(K key, V value) -> MappingInsertResult: insert or assign a key-value pair.
///     Mapping.find(const K *key) -> MappingIterator: find a key in the mapping.
///     Mapping.find_owned(K key) -> MappingIterator: find a key in the mapping.
///     Mapping.find_or_insert(K key, V or_insert_value) -> MappingIterator: find or insert a key-value pair.
///     Mapping.erase(MappingIterator node): erase a key-value pair from the mapping.
///     Mapping.swap(Mapping *other): swap the mapping with another mapping.
///     Mapping.empty() -> bool: check if the mapping is empty.
///     Mapping.clear(): clear the mapping.
///     Mapping.begin() -> MappingIterator: get the begin iterator of the mapping.
///     Mapping.next(MappingIterator node) -> MappingIterator: get the next iterator of the mapping.
///     Mapping.prev(MappingIterator node) -> MappingIterator: get the previous iterator of the mapping.
///     Mapping.lower_bound(const K *key) -> MappingIterator: get the lower bound iterator of the mapping.
///     Mapping.upper_bound(const K *key) -> MappingIterator: get the upper bound iterator of the mapping.
// clang-format on

#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "debug.h"
#include "utils.h"

#undef DECLARE_MAPPING
#define DECLARE_MAPPING(Mapping, K, V, STORAGE, key_gen, value_gen, com_gen)   \
    DECLARE_MAPPING_INNER(Mapping, CONCATENATE(Mapping, Node),                 \
                          CONCATENATE(Mapping, InsertResult),                  \
                          CONCATENATE(Mapping, Iterator), typeof(K),           \
                          typeof(V), STORAGE);                                 \
    key_gen(Mapping, K);                                                       \
    value_gen(Mapping, V);                                                     \
    com_gen(Mapping, K);

#undef DEFINE_MAPPING
#define DEFINE_MAPPING(Mapping, K, V, STORAGE)                                 \
    DEFINE_MAPPING_INNER(Mapping, CONCATENATE(Mapping, Node),                  \
                         CONCATENATE(Mapping, InsertResult),                   \
                         CONCATENATE(Mapping, Iterator), typeof(K), typeof(V), \
                         STORAGE);

#undef GENERATOR_PLAIN_COMPARATOR
#define GENERATOR_PLAIN_COMPARATOR(Mapping, K)                                 \
    FUNC_STATIC int NSMTD(Mapping, comparator, /, const typeof(K) *MPROT(a),   \
                          const typeof(K) *MPROT(b)) {                         \
        return *MPROT(a) < *MPROT(b) ? -1 : (*MPROT(a) > *MPROT(b) ? 1 : 0);   \
    }

#undef GENERATOR_CLASS_COMPARATOR
#define GENERATOR_CLASS_COMPARATOR(Mapping, K)                                 \
    FUNC_STATIC int NSMTD(Mapping, comparator, /, const typeof(K) *MPROT(a),   \
                          const typeof(K) *MPROT(b)) {                         \
        return NSCALL(K, compare, /, MPROT(a), MPROT(b));                      \
    }

#undef GENERATOR_CUSTOM_COMPARATOR
#define GENERATOR_CUSTOM_COMPARATOR(Mapping, K)

#undef GENERATOR_PLAIN_KEY
#define GENERATOR_PLAIN_KEY(Mapping, K)                                        \
    FUNC_STATIC void NSMTD(Mapping, drop_key, /,                               \
                           ATTR_UNUSED typeof(K) *MPROT(key)) {}               \
    FUNC_STATIC typeof(K) NSMTD(Mapping, clone_key, /,                         \
                                const typeof(K) *MPROT(other)) {               \
        return *MPROT(other);                                                  \
    }

#undef GENERATOR_CLASS_KEY
#define GENERATOR_CLASS_KEY(Mapping, K)                                        \
    FUNC_STATIC void NSMTD(Mapping, drop_key, /, typeof(K) *MPROT(key)) {      \
        CALL(K, *MPROT(key), drop, /);                                         \
    }                                                                          \
    FUNC_STATIC typeof(K) NSMTD(Mapping, clone_key, /,                         \
                                const typeof(K) *MPROT(other)) {               \
        return CALL(K, *MPROT(other), clone, /);                               \
    }

#undef GENERATOR_PLAIN_VALUE
#define GENERATOR_PLAIN_VALUE(Mapping, V)                                      \
    FUNC_STATIC void NSMTD(Mapping, drop_value, /,                             \
                           ATTR_UNUSED typeof(V) *MPROT(value)) {}             \
    FUNC_STATIC typeof(V) NSMTD(Mapping, clone_value, /,                       \
                                const typeof(V) *MPROT(other)) {               \
        return *MPROT(other);                                                  \
    }

#undef GENERATOR_CLASS_VALUE
#define GENERATOR_CLASS_VALUE(Mapping, V)                                      \
    FUNC_STATIC void NSMTD(Mapping, drop_value, /, typeof(V) *MPROT(value)) {  \
        CALL(V, *MPROT(value), drop, /);                                       \
    }                                                                          \
    FUNC_STATIC typeof(V) NSMTD(Mapping, clone_value, /,                       \
                                const typeof(V) *MPROT(other)) {               \
        return CALL(V, *MPROT(other), clone, /);                               \
    }

#undef DECLARE_MAPPING_INNER
#define DECLARE_MAPPING_INNER(Mapping, MappingNode, MappingInsertResult,       \
                              MappingIterator, K, V, STORAGE)                  \
    typedef struct MappingNode {                                               \
        K key;                                                                 \
        V value;                                                               \
        struct MappingNode *left_son;                                          \
        struct MappingNode *right_son;                                         \
        struct MappingNode *parent;                                            \
        u64 random_value;                                                      \
    } MappingNode;                                                             \
                                                                               \
    typedef struct Mapping {                                                   \
        MappingNode *root;                                                     \
        usize size;                                                            \
    } Mapping;                                                                 \
                                                                               \
    typedef MappingNode *MappingIterator;                                      \
                                                                               \
    typedef struct MappingInsertResult {                                       \
        MappingIterator node;                                                  \
        bool inserted;                                                         \
    } MappingInsertResult;                                                     \
                                                                               \
    /* NOTE: Methods that are required to defined outside the template */      \
                                                                               \
    /* Mapping::comparator(K a, K b) -> int */                                 \
    FUNC_STATIC int NSMTD(Mapping, comparator, /, const K *a, const K *b);     \
                                                                               \
    /* Mapping::drop_key(K *key) */                                            \
    FUNC_STATIC void NSMTD(Mapping, drop_key, /, K * key);                     \
                                                                               \
    /* Mapping::drop_value(V *value) */                                        \
    FUNC_STATIC void NSMTD(Mapping, drop_value, /, V * value);                 \
                                                                               \
    /* Mapping::clone_key(const K *other) -> K */                              \
    FUNC_STATIC K NSMTD(Mapping, clone_key, /, const K *other);                \
                                                                               \
    /* Mapping::clone_value(const V *other) -> V */                            \
    FUNC_STATIC V NSMTD(Mapping, clone_value, /, const V *other);              \
                                                                               \
    /* NOTE: Methods to implement in .c */                                     \
                                                                               \
    /* Mapping.drop() */                                                       \
    STORAGE void MTD(Mapping, drop, /);                                        \
                                                                               \
    /* Mapping.clone_from(const Mapping *other) */                             \
    STORAGE void MTD(Mapping, clone_from, /, const Mapping *other);            \
                                                                               \
    /* Mapping.insert(K key, V value) -> MappingInsertResult */                \
    STORAGE MappingInsertResult MTD(Mapping, insert, /, K key, V value);       \
                                                                               \
    /* Mapping.insert_or_assign(K key, V value) -> MappingInsertResult */      \
    STORAGE MappingInsertResult MTD(Mapping, insert_or_assign, /, K key,       \
                                    V value);                                  \
                                                                               \
    /* Mapping.find(const K (key)) -> MappingIterator */                       \
    STORAGE MappingIterator MTD(Mapping, find, /, const K *key);               \
                                                                               \
    /* Mapping.find_owned(K key) -> MappingIterator */                         \
    STORAGE MappingIterator MTD(Mapping, find_owned, /, K key);                \
                                                                               \
    /* Mapping.find_or_insert(K key, V or_insert_value) -> MappingIterator */  \
    STORAGE MappingIterator MTD(Mapping, find_or_insert, /, K key,             \
                                V or_insert_value);                            \
                                                                               \
    /* Mapping.erase(MappingIterator node) */                                  \
    STORAGE void MTD(Mapping, erase, /, MappingIterator node);                 \
                                                                               \
    /* Mapping.swap(Mapping *other) */                                         \
    STORAGE void MTD(Mapping, swap, /, Mapping * other);                       \
                                                                               \
    /* Mapping.begin() -> MappingIterator */                                   \
    STORAGE MappingIterator MTD(Mapping, begin, /);                            \
                                                                               \
    /* Mapping.next(MappingIterator node) -> MappingIterator */                \
    STORAGE MappingIterator MTD(Mapping, next, /, MappingIterator node);       \
                                                                               \
    /* Mapping.prev(MappingIterator node) -> MappingIterator */                \
    STORAGE MappingIterator MTD(Mapping, prev, /, MappingIterator node);       \
                                                                               \
    /* Mapping.lower_bound(const K *key) -> MappingIterator */                 \
    STORAGE MappingIterator MTD(Mapping, lower_bound, /, const K *key);        \
                                                                               \
    /* Mapping.upper_bound(const K *key) -> MappingIterator */                 \
    STORAGE MappingIterator MTD(Mapping, upper_bound, /, const K *key);        \
                                                                               \
    /* NOTE: static methods that are not required to implement in .c */        \
                                                                               \
    /* Mapping.init() */                                                       \
    FUNC_STATIC void MTD(Mapping, init, /) {                                   \
        self->root = NULL;                                                     \
        self->size = 0;                                                        \
    }                                                                          \
                                                                               \
    /* Mapping.clone() const -> Mapping */                                     \
    FUNC_STATIC DEFAULT_DERIVE_CLONE(Mapping, /);                              \
                                                                               \
    /* Mapping.empty() -> bool */                                              \
    FUNC_STATIC bool MTD(Mapping, empty, /) { return self->size == 0; }        \
                                                                               \
    /* Mapping.clear() */                                                      \
    FUNC_STATIC void MTD(Mapping, clear, /) { CALL(Mapping, *self, drop, /); }

#undef DEFINE_MAPPING_INNER
#define DEFINE_MAPPING_INNER(Mapping, MappingNode, MappingInsertResult,        \
                             MappingIterator, K, V, STORAGE)                   \
    /* MappingNode::random_value() -> u64 */                                   \
    static u64 NSMTD(MappingNode, random_value, /) {                           \
        /* NOTE: the thread safety is not guaranteed */                        \
        static u64 MPROT(seed) = 123213;                                       \
        MPROT(seed) ^= (MPROT(seed) << 2) * 1321;                              \
        MPROT(seed) ^= (MPROT(seed) >> 5) * 2133;                              \
        MPROT(seed) += 13223;                                                  \
        return MPROT(seed);                                                    \
    }                                                                          \
                                                                               \
    /* MappingNode.init(K key, V value) */                                     \
    static void MTD(MappingNode, init, /, K MPROT(key), V MPROT(value)) {      \
        self->key = MPROT(key);                                                \
        self->value = MPROT(value);                                            \
        self->left_son = NULL;                                                 \
        self->right_son = NULL;                                                \
        self->parent = NULL;                                                   \
        self->random_value = NSMTD(MappingNode, random_value, /);              \
    }                                                                          \
                                                                               \
    /* MappingNode.drop() */                                                   \
    static void MTD(MappingNode, drop, /) {                                    \
        NSCALL(Mapping, drop_key, /, &self->key);                              \
        NSCALL(Mapping, drop_value, /, &self->value);                          \
                                                                               \
        if (self->left_son) {                                                  \
            DROPOBJHEAP(MappingNode, self->left_son);                          \
            self->left_son = NULL;                                             \
        }                                                                      \
        if (self->right_son) {                                                 \
            DROPOBJHEAP(MappingNode, self->right_son);                         \
            self->right_son = NULL;                                            \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* MappingNode.clone_from(const MappingNode *other, bool free_before) */   \
    static void MTD(MappingNode, clone_from, /,                                \
                    const MappingNode *MPROT(other),                           \
                    bool MPROT(free_before)) {                                 \
        ASSERT(self);                                                          \
        ASSERT(MPROT(other));                                                  \
                                                                               \
        if (MPROT(free_before)) {                                              \
            NSCALL(Mapping, drop_key, /, &self->key);                          \
            NSCALL(Mapping, drop_value, /, &self->value);                      \
        }                                                                      \
                                                                               \
        self->key = NSCALL(Mapping, clone_key, /, &MPROT(other)->key);         \
        self->value = NSCALL(Mapping, clone_value, /, &MPROT(other)->value);   \
                                                                               \
        if (MPROT(other)->left_son) {                                          \
            bool MPROT(free_son_kv) = true;                                    \
            if (!self->left_son) {                                             \
                self->left_son = CREOBJRAWHEAP(MappingNode);                   \
                self->left_son->left_son = NULL;                               \
                self->left_son->right_son = NULL;                              \
                self->left_son->parent = self;                                 \
                self->left_son->random_value =                                 \
                    MPROT(other)->left_son->random_value;                      \
                MPROT(free_son_kv) = false;                                    \
            }                                                                  \
            CALL(MappingNode, *self->left_son, clone_from, /,                  \
                 MPROT(other)->left_son, MPROT(free_son_kv));                  \
        } else if (self->left_son) {                                           \
            DROPOBJHEAP(MappingNode, self->left_son);                          \
            self->left_son = NULL;                                             \
        }                                                                      \
                                                                               \
        if (MPROT(other)->right_son) {                                         \
            bool MPROT(free_son_kv) = true;                                    \
            if (!self->right_son) {                                            \
                self->right_son = CREOBJRAWHEAP(MappingNode);                  \
                self->right_son->left_son = NULL;                              \
                self->right_son->right_son = NULL;                             \
                self->right_son->parent = self;                                \
                self->right_son->random_value =                                \
                    MPROT(other)->right_son->random_value;                     \
                MPROT(free_son_kv) = false;                                    \
            }                                                                  \
            CALL(MappingNode, *self->right_son, clone_from, /,                 \
                 MPROT(other)->right_son, MPROT(free_son_kv));                 \
        } else if (self->right_son) {                                          \
            DROPOBJHEAP(MappingNode, self->right_son);                         \
            self->right_son = NULL;                                            \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* MappingNode::lturn(&MappingNode) */                                     \
    static void NSMTD(MappingNode, lturn, /, MappingNode * *MPROT(p)) {        \
        MappingNode *MPROT(son) = (*MPROT(p))->right_son;                      \
        ASSERT(MPROT(son));                                                    \
        (*MPROT(p))->right_son = MPROT(son)->left_son;                         \
        if (MPROT(son)->left_son) {                                            \
            MPROT(son)->left_son->parent = *MPROT(p);                          \
        }                                                                      \
        MPROT(son)->left_son = *MPROT(p);                                      \
        MPROT(son)->parent = (*MPROT(p))->parent;                              \
        (*MPROT(p))->parent = MPROT(son);                                      \
        *MPROT(p) = MPROT(son);                                                \
    }                                                                          \
                                                                               \
    /* MappingNode::rturn(&MappingNode) */                                     \
    static void NSMTD(MappingNode, rturn, /, MappingNode * *MPROT(p)) {        \
        MappingNode *MPROT(son) = (*MPROT(p))->left_son;                       \
        ASSERT(MPROT(son));                                                    \
        (*MPROT(p))->left_son = MPROT(son)->right_son;                         \
        if (MPROT(son)->right_son) {                                           \
            MPROT(son)->right_son->parent = *MPROT(p);                         \
        }                                                                      \
        MPROT(son)->right_son = *MPROT(p);                                     \
        MPROT(son)->parent = (*MPROT(p))->parent;                              \
        (*MPROT(p))->parent = MPROT(son);                                      \
        *MPROT(p) = MPROT(son);                                                \
    }                                                                          \
                                                                               \
    /* MappingNode::insert(&MappingNode, K key, V value) ->                    \
     * MappingInsertResult */                                                  \
    static MappingInsertResult NSMTD(MappingNode, insert, /,                   \
                                     MappingNode * *MPROT(p), K MPROT(key),    \
                                     V MPROT(value), bool MPROT(overwrite)) {  \
        ASSERT(MPROT(p));                                                      \
        MappingNode *MPROT(node) = *MPROT(p);                                  \
        if (!MPROT(node)) {                                                    \
            *MPROT(p) = CREOBJHEAP(MappingNode, /, MPROT(key), MPROT(value));  \
            return (MappingInsertResult){*MPROT(p), true};                     \
        }                                                                      \
        int MPROT(cmp_val) =                                                   \
            NSCALL(Mapping, comparator, /, &MPROT(node)->key, &MPROT(key));    \
        if (MPROT(cmp_val) == 0) {                                             \
            if (MPROT(overwrite)) {                                            \
                NSCALL(Mapping, drop_key, /, &MPROT(key));                     \
                NSCALL(Mapping, drop_value, /, &MPROT(node)->value);           \
                MPROT(node)->value = MPROT(value);                             \
                return (MappingInsertResult){MPROT(node), false};              \
            } else {                                                           \
                NSCALL(Mapping, drop_key, /, &MPROT(key));                     \
                NSCALL(Mapping, drop_value, /, &MPROT(value));                 \
                return (MappingInsertResult){MPROT(node), false};              \
            }                                                                  \
        } else if (MPROT(cmp_val) > 0) {                                       \
            MappingInsertResult MPROT(result) =                                \
                NSCALL(MappingNode, insert, /, &MPROT(node)->left_son,         \
                       MPROT(key), MPROT(value), MPROT(overwrite));            \
            if (MPROT(result).inserted) {                                      \
                ASSERT(MPROT(node)->left_son);                                 \
                MPROT(node)->left_son->parent = MPROT(node);                   \
                if (MPROT(node)->left_son->random_value <                      \
                    MPROT(node)->random_value) {                               \
                    NSCALL(MappingNode, rturn, /, MPROT(p));                   \
                }                                                              \
            }                                                                  \
            return MPROT(result);                                              \
        } else {                                                               \
            MappingInsertResult MPROT(result) =                                \
                NSCALL(MappingNode, insert, /, &MPROT(node)->right_son,        \
                       MPROT(key), MPROT(value), MPROT(overwrite));            \
            if (MPROT(result).inserted) {                                      \
                ASSERT(MPROT(node)->right_son);                                \
                MPROT(node)->right_son->parent = MPROT(node);                  \
                if (MPROT(node)->right_son->random_value <                     \
                    MPROT(node)->random_value) {                               \
                    NSCALL(MappingNode, lturn, /, MPROT(p));                   \
                }                                                              \
            }                                                                  \
            return MPROT(result);                                              \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* MappingNode.find(const K * key) -> MappingIterator */                   \
    static MappingIterator MTD(MappingNode, find, /, const K *MPROT(key)) {    \
        if (!self) {                                                           \
            return NULL;                                                       \
        }                                                                      \
        int MPROT(cmp_val) =                                                   \
            NSCALL(Mapping, comparator, /, &self->key, MPROT(key));            \
        if (MPROT(cmp_val) == 0) {                                             \
            return self;                                                       \
        } else if (MPROT(cmp_val) > 0) {                                       \
            return NSCALL(MappingNode, find, /, self->left_son, MPROT(key));   \
        } else {                                                               \
            return NSCALL(MappingNode, find, /, self->right_son, MPROT(key));  \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* MappingNode::erase(&MappingNode) */                                     \
    static void NSMTD(MappingNode, erase, /, MappingNode * *MPROT(p)) {        \
        ASSERT(MPROT(p));                                                      \
        while (true) {                                                         \
            MappingNode *MPROT(node) = *MPROT(p);                              \
            if (!MPROT(node)) {                                                \
                return;                                                        \
            }                                                                  \
            if (!MPROT(node)->left_son) {                                      \
                *MPROT(p) = MPROT(node)->right_son;                            \
                if (*MPROT(p)) {                                               \
                    (*MPROT(p))->parent = MPROT(node)->parent;                 \
                }                                                              \
                MPROT(node)->right_son = NULL;                                 \
                DROPOBJHEAP(MappingNode, MPROT(node));                         \
                return;                                                        \
            } else if (!MPROT(node)->right_son) {                              \
                *MPROT(p) = MPROT(node)->left_son;                             \
                if (*MPROT(p)) {                                               \
                    (*MPROT(p))->parent = MPROT(node)->parent;                 \
                }                                                              \
                MPROT(node)->left_son = NULL;                                  \
                DROPOBJHEAP(MappingNode, MPROT(node));                         \
                return;                                                        \
            } else {                                                           \
                if (MPROT(node)->left_son->random_value <                      \
                    MPROT(node)->right_son->random_value) {                    \
                    NSCALL(MappingNode, rturn, /, MPROT(p));                   \
                    MPROT(p) = &(*MPROT(p))->right_son;                        \
                } else {                                                       \
                    NSCALL(MappingNode, lturn, /, MPROT(p));                   \
                    MPROT(p) = &(*MPROT(p))->left_son;                         \
                }                                                              \
            }                                                                  \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* MappingNode.is_left_son() -> bool */                                    \
    static bool MTD(MappingNode, is_left_son, /) {                             \
        return self->parent && self->parent->left_son == self;                 \
    }                                                                          \
                                                                               \
    /* MappingNode.next() -> MappingIterator */                                \
    static MappingIterator MTD(MappingNode, next, /) {                         \
        if (self->right_son) {                                                 \
            MappingNode *MPROT(node) = self->right_son;                        \
            while (MPROT(node)->left_son) {                                    \
                MPROT(node) = MPROT(node)->left_son;                           \
            }                                                                  \
            return MPROT(node);                                                \
        } else {                                                               \
            MappingNode *MPROT(node) = self;                                   \
            while (MPROT(node)->parent &&                                      \
                   MPROT(node)->parent->right_son == MPROT(node)) {            \
                MPROT(node) = MPROT(node)->parent;                             \
            }                                                                  \
            return MPROT(node)->parent;                                        \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* MappingNode.prev() -> MappingIterator */                                \
    static MappingIterator MTD(MappingNode, prev, /) {                         \
        if (self->left_son) {                                                  \
            MappingNode *MPROT(node) = self->left_son;                         \
            while (MPROT(node)->right_son) {                                   \
                MPROT(node) = MPROT(node)->right_son;                          \
            }                                                                  \
            return MPROT(node);                                                \
        } else {                                                               \
            ASSERT(self->parent);                                              \
            MappingNode *MPROT(node) = self;                                   \
            while (MPROT(node)->parent &&                                      \
                   MPROT(node)->parent->left_son == MPROT(node)) {             \
                MPROT(node) = MPROT(node)->parent;                             \
            }                                                                  \
            return MPROT(node)->parent;                                        \
        }                                                                      \
    }                                                                          \
                                                                               \
    /* Implement the interface */                                              \
                                                                               \
    STORAGE void MTD(Mapping, drop, /) {                                       \
        if (self->root) {                                                      \
            DROPOBJHEAP(MappingNode, self->root);                              \
            self->root = NULL;                                                 \
        }                                                                      \
        self->size = 0;                                                        \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Mapping, clone_from, /, const Mapping *MPROT(other)) {    \
        if (self == MPROT(other)) {                                            \
            return;                                                            \
        }                                                                      \
        if (MPROT(other)->root) {                                              \
            bool MPROT(free_kv) = true;                                        \
            if (!self->root) {                                                 \
                self->root = CREOBJRAWHEAP(MappingNode);                       \
                self->root->left_son = NULL;                                   \
                self->root->right_son = NULL;                                  \
                self->root->parent = NULL;                                     \
                self->root->random_value = MPROT(other)->root->random_value;   \
                MPROT(free_kv) = false;                                        \
            }                                                                  \
            CALL(MappingNode, *self->root, clone_from, /, MPROT(other)->root,  \
                 MPROT(free_kv));                                              \
        } else if (self->root) {                                               \
            DROPOBJHEAP(MappingNode, self->root);                              \
            self->root = NULL;                                                 \
        }                                                                      \
        self->size = MPROT(other)->size;                                       \
    }                                                                          \
                                                                               \
    static MappingInsertResult MTD(Mapping, insert_inner, /, K MPROT(key),     \
                                   V MPROT(value), bool MPROT(overwrite)) {    \
        MappingInsertResult MPROT(res) =                                       \
            NSCALL(MappingNode, insert, /, &self->root, MPROT(key),            \
                   MPROT(value), MPROT(overwrite));                            \
        if (MPROT(res).inserted) {                                             \
            self->size++;                                                      \
        }                                                                      \
        return MPROT(res);                                                     \
    }                                                                          \
                                                                               \
    STORAGE MappingInsertResult MTD(Mapping, insert, /, K MPROT(key),          \
                                    V MPROT(value)) {                          \
        return CALL(Mapping, *self, insert_inner, /, MPROT(key), MPROT(value), \
                    false);                                                    \
    }                                                                          \
                                                                               \
    STORAGE MappingInsertResult MTD(Mapping, insert_or_assign, /,              \
                                    K MPROT(key), V MPROT(value)) {            \
        return CALL(Mapping, *self, insert_inner, /, MPROT(key), MPROT(value), \
                    true);                                                     \
    }                                                                          \
                                                                               \
    STORAGE MappingIterator MTD(Mapping, find, /, const K *MPROT(key)) {       \
        return NSCALL(MappingNode, find, /, self->root, MPROT(key));           \
    }                                                                          \
                                                                               \
    STORAGE MappingIterator MTD(Mapping, find_owned, /, K MPROT(key)) {        \
        MappingIterator MPROT(res) =                                           \
            NSCALL(MappingNode, find, /, self->root, &MPROT(key));             \
        NSCALL(Mapping, drop_key, /, &MPROT(key));                             \
        return MPROT(res);                                                     \
    }                                                                          \
                                                                               \
    STORAGE MappingIterator MTD(Mapping, find_or_insert, /, K MPROT(key),      \
                                V MPROT(or_insert_value)) {                    \
        MappingIterator MPROT(res) =                                           \
            NSCALL(MappingNode, find, /, self->root, &MPROT(key));             \
        if (!MPROT(res)) {                                                     \
            MappingInsertResult MPROT(ins_res) =                               \
                CALL(Mapping, *self, insert, /, MPROT(key),                    \
                     MPROT(or_insert_value));                                  \
            ASSERT(MPROT(ins_res).inserted);                                   \
            MPROT(res) = MPROT(ins_res).node;                                  \
        } else {                                                               \
            NSCALL(Mapping, drop_key, /, &MPROT(key));                         \
            NSCALL(Mapping, drop_value, /, &MPROT(or_insert_value));           \
        }                                                                      \
        return MPROT(res);                                                     \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Mapping, erase, /, MappingIterator MPROT(node)) {         \
        ASSERT(MPROT(node));                                                   \
        MappingNode **MPROT(p) = &self->root;                                  \
        if (MPROT(node)->parent) {                                             \
            MPROT(p) = CALL(MappingNode, *MPROT(node), is_left_son, /)         \
                           ? &MPROT(node)->parent->left_son                    \
                           : &MPROT(node)->parent->right_son;                  \
        }                                                                      \
        NSCALL(MappingNode, erase, /, MPROT(p));                               \
        self->size--;                                                          \
    }                                                                          \
                                                                               \
    STORAGE void MTD(Mapping, swap, /, Mapping * MPROT(other)) {               \
        if (self == MPROT(other)) {                                            \
            return;                                                            \
        }                                                                      \
        Mapping tmp = *self;                                                   \
        *self = *MPROT(other);                                                 \
        *MPROT(other) = tmp;                                                   \
    }                                                                          \
                                                                               \
    STORAGE MappingIterator MTD(Mapping, begin, /) {                           \
        if (!self->root) {                                                     \
            return NULL;                                                       \
        }                                                                      \
        MappingNode *MPROT(node) = self->root;                                 \
        while (MPROT(node)->left_son) {                                        \
            MPROT(node) = MPROT(node)->left_son;                               \
        }                                                                      \
        return MPROT(node);                                                    \
    }                                                                          \
                                                                               \
    STORAGE MappingIterator MTD(Mapping, next, /,                              \
                                MappingIterator MPROT(node)) {                 \
        ASSERT(MPROT(node));                                                   \
        MappingIterator MPROT(res) = CALL(MappingNode, *MPROT(node), next, /); \
        if (!MPROT(res)) {                                                     \
            return NULL;                                                       \
        } else {                                                               \
            return MPROT(res);                                                 \
        }                                                                      \
    }                                                                          \
                                                                               \
    STORAGE MappingIterator MTD(Mapping, prev, /,                              \
                                MappingIterator MPROT(node)) {                 \
        if (!MPROT(node)) {                                                    \
            ASSERT(self->root);                                                \
            MappingNode *MPROT(res) = self->root;                              \
            while (MPROT(res)->right_son) {                                    \
                MPROT(res) = MPROT(res)->right_son;                            \
            }                                                                  \
            return MPROT(res);                                                 \
        } else {                                                               \
            return CALL(MappingNode, *MPROT(node), prev, /);                   \
        }                                                                      \
    }                                                                          \
                                                                               \
    STORAGE MappingIterator MTD(Mapping, lower_bound, /,                       \
                                const K *MPROT(key)) {                         \
        MappingNode *MPROT(node) = self->root;                                 \
        MappingNode *MPROT(res) = NULL;                                        \
        while (MPROT(node)) {                                                  \
            int MPROT(cmp_val) =                                               \
                NSCALL(Mapping, comparator, /, &MPROT(node)->key, MPROT(key)); \
            if (MPROT(cmp_val) >= 0) {                                         \
                MPROT(res) = MPROT(node);                                      \
                MPROT(node) = MPROT(node)->left_son;                           \
            } else {                                                           \
                MPROT(node) = MPROT(node)->right_son;                          \
            }                                                                  \
        }                                                                      \
        return MPROT(res);                                                     \
    }                                                                          \
                                                                               \
    STORAGE MappingIterator MTD(Mapping, upper_bound, /,                       \
                                const K *MPROT(key)) {                         \
        MappingNode *MPROT(node) = self->root;                                 \
        MappingNode *MPROT(res) = NULL;                                        \
        while (MPROT(node)) {                                                  \
            int MPROT(cmp_val) =                                               \
                NSCALL(Mapping, comparator, /, &MPROT(node)->key, MPROT(key)); \
            if (MPROT(cmp_val) > 0) {                                          \
                MPROT(res) = MPROT(node);                                      \
                MPROT(node) = MPROT(node)->left_son;                           \
            } else {                                                           \
                MPROT(node) = MPROT(node)->right_son;                          \
            }                                                                  \
        }                                                                      \
        return MPROT(res);                                                     \
    }

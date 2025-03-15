// clang-format off
/// str.h: provides a string type and its methods
///
/// String is efectively Vec of char; it provides methods to manipulate strings in addition:
///
///     String::from_raw(const char *s) -> String: creates a string from a C string
///     String::from_f(const char *format, ...) -> String: creates a string from a formatted string
///     String::from_fv(const char *format, va_list args) -> String: creates a string from a formatted string with va_list
///     String::mock_raw(const char *s) -> String: creates a mocked string with s; NEVER MODIFY IT!
///     String::mock_raw_with_len(const char *s, usize len) -> String: creates a mocked string with s, len
///     String.c_str() -> const char *: returns the string as a C string
///     String.push_str(const char *s): appends a C string to the string
///     String.pushf(const char *format, ...) -> int: appends a formatted string to the string
///     String.pushfv(const char *format, va_list args) -> int: appends a formatted string with va_list to the string
///     String::compare(const String *a, const String *b) -> int: compares two strings
///
/// Macros:
///     STRING_C_STR(s): returns the C string of the string s

#pragma once

#include <stdarg.h>

#include "utils.h"
#include "tem_vec.h"

DECLARE_PLAIN_VEC(String, char, extern);

/* String.from_raw(const char *s) -> String */
String NSMTD(String, from_raw, /, const char *s);

/* String.from_f(const char *format, ...) -> String */
String NSMTD(String, from_f, /, const char *format, ...)
    __attribute__((format(printf, 1, 2)));

/* String.from_fv(const char *format, va_list args) -> String */
String NSMTD(String, from_fv, /, const char *format, va_list args) __attribute__((format(printf, 1, 0)));

/* String.mock_raw(const char *s) -> String */
String NSMTD(String, mock_raw, /, const char *s);

/* String.mock_raw_with_len(const char *s, usize len) -> String */
String NSMTD(String, mock_raw_with_len, /, const char *s, usize len);

/* String.c_str() -> const char * */
const char *MTD(String, c_str, /);

/* String.push_str(const char *s) */
void MTD(String, push_str, /, const char *s);

/* String.pushf(const char *format, ...) -> int */
int MTD(String, pushf, /, const char *format, ...)
    __attribute__((format(printf, 2, 3)));

/* String.pushfv(const char *format, va_list args) -> int */
int MTD(String, pushfv, /, const char *format, va_list args) __attribute__((format(printf, 2, 0)));

/* String.compare(const String *a, const String *b) -> int */
int NSMTD(String, compare, /, const String *a, const String *b);

#undef STRING_C_STR
#define STRING_C_STR(s) CALL(String, s, c_str, /)

/// HString

typedef struct HString {
    String s;
    u64 stored_hash;
} HString;

/// Every time modifies s, call this to update stored_hash
static inline void MTD(HString, rehash, /) {
    u64 hash = 0;
    for (usize i = 0; i < self->s.size; i++) {
        hash = hash * (u64)277 + (u64)self->s.data[i];
    }
    self->stored_hash = hash;
}

static inline void MTD(HString, init, /) {
    self->s = CREOBJ(String, /);
    self->stored_hash = 0;
}

static inline void MTD(HString, drop, /) {
    CALL(String, self->s, drop, /);
    self->stored_hash = 0;
}

static inline void MTD(HString, clone_from, /, const HString *other) {
    CALL(String, self->s, clone_from, /, &other->s);
    self->stored_hash = other->stored_hash;
}

static inline DEFAULT_DERIVE_CLONE(HString, /);

static inline HString NSMTD(HString, from_inner, /, String s) {
    HString hstr = CREOBJ(HString, /);
    hstr.s = s;
    CALL(HString, hstr, rehash, /);
    return hstr;
}

int NSMTD(HString, compare, /, const HString *a, const HString *b);

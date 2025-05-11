#define _GNU_SOURCE
#include <stdarg.h>
#include <stdio.h>

#include "str.h"
#include "tem_vec.h"

DEFINE_PLAIN_VEC(String, char, extern);

String NSMTD(String, from_raw, /, const char *s) {
    String ret = CREOBJ(String, /);
    CALL(String, ret, push_str, /, s);
    return ret;
}

String NSMTD(String, from_f, /, const char *format, ...) {
    va_list args;
    va_start(args, format);
    String ret = NSMTD(String, from_fv, /, format, args);
    va_end(args);
    return ret;
}

String NSMTD(String, from_fv, /, const char *format, va_list args) {
    String ret = CREOBJ(String, /);
    int v = CALL(String, ret, pushfv, /, format, args);
    ASSERT(v >= 0, "pushfv failed");
    return ret;
}

String NSMTD(String, mock_raw, /, const char *s) {
    usize len = strlen(s);
    return NSCALL(String, mock_raw_with_len, /, s, len);
}

String NSMTD(String, mock_raw_with_len, /, const char *s, usize len) {
    return (String){
        .size = len,
        .capacity = len + 1,
        .data = (char *)s,
    };
}

const char *MTD(String, c_str, /) {
    CALL(String, *self, check_expansion, /);
    if (self->data[self->size] != '\0') {
        self->data[self->size] = '\0';
    }
    return self->data;
}

void MTD(String, push_str, /, const char *s) {
    if (!s) {
        return;
    }
    usize len = strlen(s);
    CALL(String, *self, reserve, /, self->size + len + 1);
    memcpy(self->data + self->size, s, len);
    self->size += len;
}

int MTD(String, pushf, /, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret_val = CALL(String, *self, pushfv, /, format, args);
    va_end(args);
    return ret_val;
}

int MTD(String, pushfv, /, const char *format, va_list args) {
    char *buffer = NULL;
    usize size = 0;
    FILE *stream = open_memstream(&buffer, &size);
    if (!stream) {
        return -1;
    }

    int ret_val = vfprintf(stream, format, args);

    fclose(stream);

    if (ret_val > 0 && buffer) {
        CALL(String, *self, push_str, /, buffer);
    }
    free(buffer);
    return ret_val;
}

int NSMTD(String, compare, /, const String *a, const String *b) {
    for (usize i = 0; i < a->size && i < b->size; i++) {
        if (a->data[i] != b->data[i]) {
            return NORMALCMP(a->data[i], b->data[i]);
        }
    }
    return NORMALCMP(a->size, b->size);
}

int NSMTD(HString, compare, /, const HString *a, const HString *b) {
    if (a->stored_hash != b->stored_hash) {
        return NORMALCMP(a->stored_hash, b->stored_hash);
    }
    return NSCALL(String, compare, /, &a->s, &b->s);
}

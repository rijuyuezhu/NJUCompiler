#include "debug.h"
#include "gen_vec.h"
#include "str.h"
#include "utils.h"

static VecI32 gen_range(usize n) {
    VecI32 v = CREOBJ(VecI32, /);
    for (usize i = 0; i < n; i++) {
        CALL(VecI32, v, push_back, /, (i32)i);
    }
    return v;
}
static void assert_same_vec(VecI32 *v1, VecI32 *v2) {
    ASSERT(v1->size == v2->size);

    for (usize j = 0; j < v1->size; j++) {
        ASSERT(v1->data[j] == v2->data[j]);
    }
}

static void plain_simple() {
    for (usize i = 0; i < 100; i++) {
        VecI32 v = gen_range(i);
        for (i32 j = 0; j < (int)i; j++) {
            ASSERT(*CALL(VecI32, v, at, /, (usize)j) == j);
            ASSERT(v.data[(usize)j] == j);
        }

        VecI32 copied_v = CALL(VecI32, v, clone, /);

        assert_same_vec(&copied_v, &v);

        ASSERT(v.size == i);

        CALL(VecI32, v, push_back, /, i);

        ASSERT(copied_v.size + 1 == v.size);

        CALL(VecI32, v, swap, /, &copied_v);

        ASSERT(copied_v.size == v.size + 1);

        CALL(VecI32, v, clone_from, /, &copied_v);

        assert_same_vec(&copied_v, &v);

        ASSERT(v.size == i + 1);

        for (usize j = 0; j < i; j++)
            CALL(VecI32, v, pop_back, /);

        CALL(VecI32, copied_v, resize, /, 1);

        assert_same_vec(&copied_v, &v);

        ASSERT(v.size == 1);

        usize cap = Max(v.capacity, copied_v.capacity);

        CALL(VecI32, v, reserve, /, cap * 2);

        ASSERT(v.capacity == cap * 2);
        CALL(VecI32, v, reserve, /, cap);
        ASSERT(v.capacity == cap * 2);

        assert_same_vec(&copied_v, &v);
        DROPOBJ(VecI32, copied_v);

        DROPOBJ(VecI32, v);
    }
}

static void plain_ins_rem() {
    VecI32 v = CREOBJ(VecI32, /);
    for (usize i = 0; i < 4; i++) {
        CALL(VecI32, v, push_back, /, i);
    }
    // v == [0, 1, 2, 3]
    CALL(VecI32, v, insert, /, 0, 10);
    // v == [10, 0, 1, 2, 3]
    ASSERT(v.size == 5);
    ASSERT(*CALL(VecI32, v, at, /, 0) == 10);
    ASSERT(*CALL(VecI32, v, at, /, 1) == 0);
    ASSERT(*CALL(VecI32, v, at, /, 2) == 1);
    ASSERT(*CALL(VecI32, v, at, /, 3) == 2);
    ASSERT(*CALL(VecI32, v, at, /, 4) == 3);

    CALL(VecI32, v, insert, /, 2, 20);

    // v == [10, 0, 20, 1, 2, 3]
    ASSERT(v.size == 6);
    ASSERT(*CALL(VecI32, v, at, /, 0) == 10);
    ASSERT(*CALL(VecI32, v, at, /, 1) == 0);
    ASSERT(*CALL(VecI32, v, at, /, 2) == 20);
    ASSERT(*CALL(VecI32, v, at, /, 3) == 1);
    ASSERT(*CALL(VecI32, v, at, /, 4) == 2);
    ASSERT(*CALL(VecI32, v, at, /, 5) == 3);

    CALL(VecI32, v, erase, /, 1);

    // v == [10, 20, 1, 2, 3]
    ASSERT(v.size == 5);
    ASSERT(*CALL(VecI32, v, at, /, 0) == 10);
    ASSERT(*CALL(VecI32, v, at, /, 1) == 20);
    ASSERT(*CALL(VecI32, v, at, /, 2) == 1);
    ASSERT(*CALL(VecI32, v, at, /, 3) == 2);
    ASSERT(*CALL(VecI32, v, at, /, 4) == 3);

    CALL(VecI32, v, erase, /, 4);

    // v == [10, 20, 1, 2]
    ASSERT(v.size == 4);
    ASSERT(*CALL(VecI32, v, at, /, 0) == 10);
    ASSERT(*CALL(VecI32, v, at, /, 1) == 20);
    ASSERT(*CALL(VecI32, v, at, /, 2) == 1);
    ASSERT(*CALL(VecI32, v, at, /, 3) == 2);

    CALL(VecI32, v, pop_back, /);

    // v == [10, 20, 1]
    ASSERT(v.size == 3);
    ASSERT(*CALL(VecI32, v, at, /, 0) == 10);
    ASSERT(*CALL(VecI32, v, at, /, 1) == 20);
    ASSERT(*CALL(VecI32, v, at, /, 2) == 1);

    ASSERT(*CALL(VecI32, v, front, /) == 10);
    ASSERT(*CALL(VecI32, v, back, /) == 1);

    CALL(VecI32, v, resize, /, 2);

    // v == [10, 20]
    ASSERT(v.size == 2);
    ASSERT(*CALL(VecI32, v, at, /, 0) == 10);
    ASSERT(*CALL(VecI32, v, at, /, 1) == 20);

    CALL(VecI32, v, resize, /, 0);

    // v == []
    ASSERT(v.size == 0);
    ASSERT(CALL(VecI32, v, empty, /));

    CALL(VecI32, v, push_back, /, 100);

    // v = [100]
    ASSERT(v.size == 1);
    ASSERT(*CALL(VecI32, v, at, /, 0) == 100);

    CALL(VecI32, v, shrink_to_fit, /);
    ASSERT(v.size == 1);
    ASSERT(v.capacity == v.size);
    ASSERT(*CALL(VecI32, v, at, /, 0) == 100);

    CALL(VecI32, v, clear, /);

    ASSERT(v.size == 0);
    ASSERT(v.capacity == 1);
    ASSERT(CALL(VecI32, v, empty, /));

    DROPOBJ(VecI32, v);
}

void test_plain_vec() {
    plain_simple();
    plain_ins_rem();
}

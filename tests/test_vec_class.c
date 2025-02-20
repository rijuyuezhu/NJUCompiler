#include "debug.h"
#include "gen_vec.h"
#include "str.h"
#include "utils.h"

static VecStr gen_range(usize n) {
    VecStr v = CREOBJ(VecStr, /);
    for (usize i = 0; i < n; i++) {
        String s = NSCALL(String, from_f, /, "%zu", i);
        CALL(VecStr, v, push_back, /, s); // invalid s
    }
    return v;
}
static void assert_same_vec(VecStr *v1, VecStr *v2) {
    ASSERT(v1->size == v2->size);

    for (usize j = 0; j < v1->size; j++) {
        ASSERT_EQ_STR(STRING_C_STR(v1->data[j]), STRING_C_STR(v2->data[j]));
    }
}

static void class_simple() {
    for (usize i = 0; i < 100; i++) {
        VecStr v = gen_range(i);
        for (i32 j = 0; j < (int)i; j++) {
            char t[20];
            snprintf(t, 20, "%d", j);
            String *s = CALL(VecStr, v, at, /, (usize)j);
            ASSERT_EQ_STR(STRING_C_STR(*s), t);
        }

        VecStr copied_v = CALL(VecStr, v, clone, /);

        assert_same_vec(&copied_v, &v);

        ASSERT(v.size == i);

        String si = NSCALL(String, from_f, /, "%zu", i);

        CALL(VecStr, v, push_back, /, si); // invalidate si

        ASSERT(copied_v.size + 1 == v.size);

        CALL(VecStr, v, swap, /, &copied_v);

        ASSERT(copied_v.size == v.size + 1);

        CALL(VecStr, v, clone_from, /, &copied_v);

        assert_same_vec(&copied_v, &v);

        ASSERT(v.size == i + 1);

        VecStr v3 = CREOBJ(VecStr, /);

        CALL(VecStr, v3, clone_from, /, &v);

        for (usize j = 0; j < i; j++)
            CALL(VecStr, v, pop_back, /);

        String needless_padding = CREOBJ(String, /);
        CALL(VecStr, copied_v, resize, /, 1,
             needless_padding); // invalidate needless_padding
        CALL(VecStr, v3, truncate, /, 1);
        assert_same_vec(&copied_v, &v);
        assert_same_vec(&copied_v, &v3);

        ASSERT(v.size == 1);

        usize cap = Max(v.capacity, copied_v.capacity);

        CALL(VecStr, v, reserve, /, cap * 2);

        ASSERT(v.capacity == cap * 2);
        CALL(VecStr, v, reserve, /, cap);
        ASSERT(v.capacity == cap * 2);

        assert_same_vec(&copied_v, &v);
        DROPOBJ(VecStr, copied_v);
        DROPOBJ(VecStr, v3);

        DROPOBJ(VecStr, v);
    }
}

static void class_ins_rem() {
    VecStr v = CREOBJ(VecStr, /);
    for (usize i = 0; i < 4; i++) {
        String si = NSCALL(String, from_f, /, "%zu", i);
        CALL(VecStr, v, push_back, /, si);
    }
    // v == [0, 1, 2, 3]
    String si = NSCALL(String, from_f, /, "%d", 10);
    CALL(VecStr, v, insert, /, 0, si);

    // v == [10, 0, 1, 2, 3]
    ASSERT(v.size == 5);
    ASSERT_EQ_STR(STRING_C_STR(v.data[0]), "10");
    ASSERT_EQ_STR(STRING_C_STR(v.data[1]), "0");
    ASSERT_EQ_STR(STRING_C_STR(v.data[2]), "1");
    ASSERT_EQ_STR(STRING_C_STR(v.data[3]), "2");
    ASSERT_EQ_STR(STRING_C_STR(v.data[4]), "3");

    si = CREOBJ(String, /);
    CALL(String, si, pushf, /, "%d", 20);
    CALL(VecStr, v, insert, /, 2, si);

    // v == [10, 0, 20, 1, 2, 3]
    ASSERT(v.size == 6);
    ASSERT_EQ_STR(STRING_C_STR(v.data[0]), "10");
    ASSERT_EQ_STR(STRING_C_STR(v.data[1]), "0");
    ASSERT_EQ_STR(STRING_C_STR(v.data[2]), "20");
    ASSERT_EQ_STR(STRING_C_STR(v.data[3]), "1");
    ASSERT_EQ_STR(STRING_C_STR(v.data[4]), "2");
    ASSERT_EQ_STR(STRING_C_STR(v.data[5]), "3");

    CALL(VecStr, v, erase, /, 1);

    // v == [10, 20, 1, 2, 3]
    ASSERT(v.size == 5);
    ASSERT_EQ_STR(STRING_C_STR(v.data[0]), "10");
    ASSERT_EQ_STR(STRING_C_STR(v.data[1]), "20");
    ASSERT_EQ_STR(STRING_C_STR(v.data[2]), "1");
    ASSERT_EQ_STR(STRING_C_STR(v.data[3]), "2");
    ASSERT_EQ_STR(STRING_C_STR(v.data[4]), "3");

    CALL(VecStr, v, erase, /, 4);

    // v == [10, 20, 1, 2]
    ASSERT(v.size == 4);
    ASSERT_EQ_STR(STRING_C_STR(v.data[0]), "10");
    ASSERT_EQ_STR(STRING_C_STR(v.data[1]), "20");
    ASSERT_EQ_STR(STRING_C_STR(v.data[2]), "1");
    ASSERT_EQ_STR(STRING_C_STR(v.data[3]), "2");

    CALL(VecStr, v, pop_back, /);

    // v == [10, 20, 1]
    ASSERT(v.size == 3);
    ASSERT_EQ_STR(STRING_C_STR(v.data[0]), "10");
    ASSERT_EQ_STR(STRING_C_STR(v.data[1]), "20");
    ASSERT_EQ_STR(STRING_C_STR(v.data[2]), "1");

    String *front = CALL(VecStr, v, front, /);
    String *back = CALL(VecStr, v, back, /);
    ASSERT_EQ_STR(STRING_C_STR(*front), "10");
    ASSERT_EQ_STR(STRING_C_STR(*back), "1");

    String padding = CREOBJ(String, /);
    CALL(VecStr, v, resize, /, 2, padding);

    // v == [10, 20]
    ASSERT(v.size == 2);
    ASSERT_EQ_STR(STRING_C_STR(v.data[0]), "10");
    ASSERT_EQ_STR(STRING_C_STR(v.data[1]), "20");

    padding = NSCALL(String, from_f, /, "%d", 100);
    CALL(VecStr, v, resize, /, 4, padding);

    // v == [10, 20, 100, 100]
    ASSERT(v.size == 4);
    ASSERT_EQ_STR(STRING_C_STR(v.data[0]), "10");
    ASSERT_EQ_STR(STRING_C_STR(v.data[1]), "20");
    ASSERT_EQ_STR(STRING_C_STR(v.data[2]), "100");
    ASSERT_EQ_STR(STRING_C_STR(v.data[3]), "100");

    CALL(VecStr, v, truncate, /, 2);

    // v == [10, 20]
    ASSERT(v.size == 2);
    ASSERT_EQ_STR(STRING_C_STR(v.data[0]), "10");
    ASSERT_EQ_STR(STRING_C_STR(v.data[1]), "20");

    padding = CREOBJ(String, /);
    CALL(VecStr, v, resize, /, 0, padding);

    // v == []
    ASSERT(v.size == 0);
    ASSERT(CALL(VecStr, v, empty, /));

    si = NSCALL(String, from_f, /, "%d", 100);
    CALL(VecStr, v, push_back, /, si);

    // v = [100]
    ASSERT(v.size == 1);
    ASSERT_EQ_STR(STRING_C_STR(v.data[0]), "100");

    CALL(VecStr, v, shrink_to_fit, /);
    ASSERT(v.size == 1);
    ASSERT(v.capacity == v.size);
    ASSERT_EQ_STR(STRING_C_STR(v.data[0]), "100");

    CALL(VecStr, v, clear, /);

    ASSERT(v.size == 0);
    ASSERT(v.capacity == 1);
    ASSERT(CALL(VecStr, v, empty, /));

    DROPOBJ(VecStr, v);
}

void test_class_vec() {
    class_simple();
    class_ins_rem();
}

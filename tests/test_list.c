#include "gen_list.h"
#include "str.h"
#include "utils.h"

static void naive() {
    ListI32 list = CREOBJ(ListI32, /);
    CALL(ListI32, list, push_back, /, 1);
    DROPOBJ(ListI32, list);

    list = CREOBJ(ListI32, /);
    CALL(ListI32, list, push_back, /, 1);
    CALL(ListI32, list, push_back, /, 4);
    CALL(ListI32, list, push_back, /, 9);
    CALL(ListI32, list, push_back, /, 16);
    i32 i = 1;
    while (i <= 4) {
        i32 x = CALL(ListI32, list, pop_front, /);
        ASSERT(i * i == x);
        i++;
    }
    ASSERT(list.size == 0);
    ASSERT(CALL(ListI32, list, empty, /));

    CALL(ListI32, list, push_front, /, 1);
    CALL(ListI32, list, push_front, /, 4);
    CALL(ListI32, list, push_front, /, 9);
    CALL(ListI32, list, push_front, /, 16);
    i = 4;
    while (i >= 1) {
        i32 x = CALL(ListI32, list, pop_front, /);
        ASSERT(i * i == x);
        i--;
    }
    ASSERT(list.size == 0);
    ASSERT(CALL(ListI32, list, empty, /));

    CALL(ListI32, list, push_front, /, 1);
    CALL(ListI32, list, push_front, /, 4);
    CALL(ListI32, list, push_front, /, 9);
    CALL(ListI32, list, push_front, /, 16);
    i = 1;
    while (i <= 4) {
        i32 x = CALL(ListI32, list, pop_back, /);
        ASSERT(i * i == x);
        i++;
    }
    ASSERT(list.size == 0);
    ASSERT(CALL(ListI32, list, empty, /));

    CALL(ListI32, list, push_back, /, 1);
    CALL(ListI32, list, push_back, /, 4);
    CALL(ListI32, list, push_back, /, 9);
    CALL(ListI32, list, push_back, /, 16);
    i = 4;
    while (i >= 1) {
        i32 x = CALL(ListI32, list, pop_back, /);
        ASSERT(i * i == x);
        i--;
    }
    ASSERT(list.size == 0);
    ASSERT(CALL(ListI32, list, empty, /));
    DROPOBJ(ListI32, list);

    CALL(ListI32, list, push_back, /, 1);
    CALL(ListI32, list, push_back, /, 4);
    CALL(ListI32, list, push_back, /, 9);
    CALL(ListI32, list, push_back, /, 16);
    ListI32 list2 = NSCALL(ListI32, clone, /, &list);
    i = 4;
    while (i >= 1) {
        i32 x = CALL(ListI32, list2, pop_back, /);
        ASSERT(i * i == x);
        i--;
    }
    ASSERT(list2.size == 0);
    ASSERT(CALL(ListI32, list2, empty, /));
    CALL(ListI32, list2, push_back, /, 1);
    DROPOBJ(ListI32, list2);
    i = 4;
    while (i >= 1) {
        i32 x = CALL(ListI32, list, pop_back, /);
        ASSERT(i * i == x);
        i--;
    }
    ASSERT(list.size == 0);
    ASSERT(CALL(ListI32, list, empty, /));
    DROPOBJ(ListI32, list);

    list = CREOBJ(ListI32, /);
    CALL(ListI32, list, push_back, /, 1);
    CALL(ListI32, list, push_back, /, 4);
    ListI32Node *it = list.head;
    ASSERT(it && it->data == 1);
    it = it->next;
    ASSERT(it && it->data == 4);
    it = CALL(ListI32, list, remove, /, it);
    ASSERT(!it);
    ASSERT(list.size == 1);
    DROPOBJ(ListI32, list);
}

static void class_test() {
    ListString list = CREOBJ(ListString, /);
    String s = NSCALL(String, from_raw, /, "Hello");
    CALL(ListString, list, push_back, /, s);
    DROPOBJ(ListString, list);
    list = CREOBJ(ListString, /);
    s = NSCALL(String, from_raw, /, "Hello");
    CALL(ListString, list, push_back, /, s);
    s = NSCALL(String, from_raw, /, "World");
    CALL(ListString, list, push_back, /, s);
    s = CALL(ListString, list, pop_front, /);
    ASSERT(strcmp(STRING_C_STR(s), "Hello") == 0);
    DROPOBJ(String, s);
    s = CALL(ListString, list, pop_front, /);
    ASSERT(strcmp(STRING_C_STR(s), "World") == 0);
    DROPOBJ(String, s);
    ASSERT(list.size == 0);
    ASSERT(CALL(ListString, list, empty, /));

    list = CREOBJ(ListString, /);
    s = NSCALL(String, from_raw, /, "Hello");
    CALL(ListString, list, push_back, /, s);
    s = NSCALL(String, from_raw, /, "World");
    CALL(ListString, list, push_back, /, s);

    String *str = CALL(ListString, list, front, /);
    ASSERT(strcmp(STRING_C_STR(*str), "Hello") == 0);
    CALL(ListString, list, remove_front, /);
    str = CALL(ListString, list, front, /);
    ASSERT(strcmp(STRING_C_STR(*str), "World") == 0);
    CALL(ListString, list, remove_front, /);
    ASSERT(list.size == 0);
    ASSERT(CALL(ListString, list, empty, /));

    s = NSCALL(String, from_raw, /, "Hello");
    CALL(ListString, list, push_back, /, s);
    DROPOBJ(ListString, list);
}
void test_list() {
    naive();
    class_test();
}

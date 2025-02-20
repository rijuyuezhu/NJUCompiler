#include "debug.h"
#include "gen_map.h"
#include "utils.h"

DECLARE_MAPPING(HMapSS, HString, HString, FUNC_STATIC, GENERATOR_CLASS_KEY,
                GENERATOR_CLASS_VALUE, GENERATOR_CLASS_COMPARATOR);
DEFINE_MAPPING(HMapSS, HString, HString, FUNC_STATIC);

static void easy() {
    MapSS m = CREOBJ(MapSS, /);
    ASSERT(m.size == 0);
    String k = NSCALL(String, from_raw, /, "nihao");
    String v = NSCALL(String, from_raw, /, "ma");
    MapSSInsertResult res = CALL(MapSS, m, insert, /, k, v);
    ASSERT(res.inserted);
    MapSSIterator it1 = res.node;
    ASSERT_EQ_STR(STRING_C_STR(it1->key), "nihao");
    ASSERT_EQ_STR(STRING_C_STR(it1->value), "ma");
    ASSERT(m.size == 1);

    k = NSCALL(String, from_raw, /, "haha");
    v = NSCALL(String, from_raw, /, "hehe");
    res = CALL(MapSS, m, insert, /, k, v);
    ASSERT(res.inserted);
    MapSSIterator it2 = res.node;
    ASSERT_EQ_STR(STRING_C_STR(it2->key), "haha");
    ASSERT_EQ_STR(STRING_C_STR(it2->value), "hehe");
    ASSERT(m.size == 2);

    k = NSCALL(String, from_raw, /, "haha");
    v = NSCALL(String, from_raw, /, "what");
    res = CALL(MapSS, m, insert, /, k, v);
    ASSERT(!res.inserted);
    ASSERT(it2 == res.node);
    ASSERT_EQ_STR(STRING_C_STR(it2->key), "haha");
    ASSERT_EQ_STR(STRING_C_STR(it2->value), "hehe");
    ASSERT(m.size == 2);

    ASSERT(CALL(MapSS, m, begin, /) == it2);

    MapSSIterator preend = CALL(MapSS, m, prev, /, NULL);
    ASSERT(it1 == preend);

    ASSERT(CALL(MapSS, m, next, /, it2) == it1);

    k = NSCALL(String, from_raw, /, "haha");
    v = NSCALL(String, from_raw, /, "what");
    res = CALL(MapSS, m, insert_or_assign, /, k, v);
    ASSERT(!res.inserted);
    ASSERT(it2 == res.node);
    ASSERT_EQ_STR(STRING_C_STR(it2->key), "haha");
    ASSERT_EQ_STR(STRING_C_STR(it2->value), "what");
    ASSERT(m.size == 2);

    CALL(MapSS, m, erase, /, it1);
    CALL(MapSS, m, erase, /, it2);
    ASSERT(m.size == 0);
    ASSERT(CALL(MapSS, m, begin, /) == NULL);
    ASSERT(CALL(MapSS, m, empty, /));

    for (usize i = 0; i < 100; i++) {
        String key = NSCALL(String, from_f, /, "key %02zu", i);
        String value = NSCALL(String, from_f, /, "value %02zu", i);
        CALL(MapSS, m, insert, /, key, value);
    }

    usize cnt = 0;
    for (MapSSIterator i = CALL(MapSS, m, begin, /); i;
         i = CALL(MapSS, m, next, /, i)) {
        char s[20];
        snprintf(s, 20, "key %02zu", cnt);
        ASSERT_EQ_STR(STRING_C_STR(i->key), s);
        snprintf(s, 20, "value %02zu", cnt);
        ASSERT_EQ_STR(STRING_C_STR(i->value), s);
        CALL(String, i->value, pushf, /, " what");
        cnt++;
    }

    ASSERT(m.size == 100);

    MapSS m2 = CALL(MapSS, m, clone, /);

    for (usize i = 0; i < 100; i++) {
        char s[20];
        snprintf(s, 20, "key %02zu", i);
        String key = NSCALL(String, mock_raw, /, s);
        // String key = NSCALL(String, from_f, /, "key %02zu", i);
        String value = NSCALL(String, from_f, /, "value %02zu what", i);
        MapSSIterator it = CALL(MapSS, m, find, /, &key);
        ASSERT(it);
        ASSERT_EQ_STR(STRING_C_STR(it->key), STRING_C_STR(key));
        ASSERT_EQ_STR(STRING_C_STR(it->value), STRING_C_STR(value));
        // DROPOBJ(String, key);
        DROPOBJ(String, value);
        key = NSCALL(String, from_f, /, "key %02zu", i);
        value = NSCALL(String, from_f, /, "value %02zu what", i);
        it = CALL(MapSS, m, find_owned, /, key);
        ASSERT(it);
        ASSERT_EQ_STR(STRING_C_STR(it->value), STRING_C_STR(value));
        DROPOBJ(String, value);
    }
    ASSERT(m.size == 100);

    cnt = 0;
    for (MapSSIterator i = CALL(MapSS, m, begin, /); i;
         i = CALL(MapSS, m, next, /, i)) {
        char s[20];
        snprintf(s, 20, "key %02zu", cnt);
        ASSERT_EQ_STR(STRING_C_STR(i->key), s);
        snprintf(s, 20, "value %02zu what", cnt);
        ASSERT_EQ_STR(STRING_C_STR(i->value), s);
        cnt++;
    }

    for (usize i = 50; i < 100; i++) {
        String key = NSCALL(String, from_f, /, "key %02zu", i);
        MapSSIterator it = CALL(MapSS, m, find_owned, /, key);
        CALL(MapSS, m, erase, /, it);
    }
    ASSERT(m.size == 50);

    for (usize i = 0; i < 50; i++) {
        String key = NSCALL(String, from_f, /, "key %02zu", i);
        String value = NSCALL(String, from_f, /, "value %02zu what", i);
        CALL(MapSS, m, find_or_insert, /, key, value);
    }
    ASSERT(m.size == 50);

    for (usize i = 50; i < 100; i++) {
        String key = NSCALL(String, from_f, /, "key %02zu", i);
        String value = NSCALL(String, from_f, /, "value %02zu what", i);
        CALL(MapSS, m, find_or_insert, /, key, value);
    }
    ASSERT(m.size == 100);
    MapSS m3 = CREOBJ(MapSS, /);
    CALL(MapSS, m3, clone_from, /, &m2);

    for (MapSSIterator i1 = CALL(MapSS, m, begin, /),
                       i2 = CALL(MapSS, m2, begin, /),
                       i3 = CALL(MapSS, m3, begin, /);
         i1 || i2 || i3;
         i1 = CALL(MapSS, m, next, /, i1), i2 = CALL(MapSS, m2, next, /, i2),
                       i3 = CALL(MapSS, m3, next, /, i3)) {
        ASSERT(i1 && i2 && i3);
        ASSERT_EQ_STR(STRING_C_STR(i1->key), STRING_C_STR(i2->key));
        ASSERT_EQ_STR(STRING_C_STR(i1->value), STRING_C_STR(i2->value));
        ASSERT_EQ_STR(STRING_C_STR(i1->key), STRING_C_STR(i3->key));
        ASSERT_EQ_STR(STRING_C_STR(i1->value), STRING_C_STR(i3->value));
    }

    CALL(MapSS, m2, clear, /);
    ASSERT(m2.size == 0);

    for (usize i = 0; i < 50; i++) {
        String sk = NSCALL(String, from_f, /, "random string %zu", i);
        String sv = CALL(String, sk, clone, /);
        CALL(MapSS, m2, insert, /, sk, sv);
    }
    CALL(MapSS, m2, swap, /, &m);
    ASSERT(m.size == 50);
    CALL(MapSS, m, clone_from, /, &m2);
    ASSERT(m2.size == 100);

    DROPOBJ(MapSS, m);
    DROPOBJ(MapSS, m2);
    DROPOBJ(MapSS, m3);
}

static void finders() {
    MapSS m = CREOBJ(MapSS, /);
    for (char ch = 'a'; ch <= 'z'; ch += 2) {
        String key = NSCALL(String, from_f, /, "%c", ch);
        String value = NSCALL(String, from_f, /, "%c", ch);
        CALL(MapSS, m, insert, /, key, value);
    }
    for (char ch = 'a'; ch <= 'z'; ch += 2) {
        String key = NSCALL(String, from_f, /, "%c", ch);
        MapSSIterator it = CALL(MapSS, m, lower_bound, /, &key);
        ASSERT(it->value.data[0] == ch);
        it = CALL(MapSS, m, upper_bound, /, &key);
        if (ch != 'y') {
            ASSERT_EQ(it->value.data[0], ch + 2, "%c");
        } else {
            ASSERT(!it);
        }
        DROPOBJ(String, key);
    }
    DROPOBJ(MapSS, m);
}

void test_map() {
    easy();
    finders();
}

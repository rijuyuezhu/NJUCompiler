#include "inttypes.h"
#include "str.h"

#define N 50
void test_hstr() {
    HString hs[N];
    for (usize i = 0; i < N; i++) {
        String temp = NSCALL(String, from_f, /, "hello, %zu!", i);
        hs[i] = NSCALL(HString, from_inner, /, temp);
    }
    qsort(hs, N, sizeof(HString),
          (int (*)(const void *, const void *))MTDNAME(HString, compare));
    for (usize i = 0; i < N; i++) {
        DROPOBJ(HString, hs[i]);
    }
}

#include <sys/wait.h>
#include <unistd.h>

#include "gen_vec.h"
#include "string.h"
#include "test_main.h"

DECLARE_PLAIN_VEC(VecTest, TestEntry, FUNC_STATIC);
DEFINE_PLAIN_VEC(VecTest, TestEntry, FUNC_STATIC);

static void fork_to_run_test(const TestEntry *test, bool *shall_exit,
                             int *exit_code);

int main(int argc, char *argv[]) {
    const TestEntry tests[] = {
        TESTENTRY(plain_vec), TESTENTRY(class_vec), TESTENTRY(map),
        TESTENTRY(hstr),
        // TESTENTRY(lexical), // stage-used test
        TESTENTRY(syntax), // stage-used test
    };

    const usize n_tests = LENGTH(tests);

    VecCString plus_pat = CREOBJ(VecCString, /);
    VecCString minus_pat = CREOBJ(VecCString, /);
    VecTest test_run = CREOBJ(VecTest, /);
    VecTest test_filter = CREOBJ(VecTest, /);

    for (int i = 1; i < argc; i++) {
        const char *s = argv[i];
        usize len = strlen(s);
        if (len == 0) {
            continue;
        }
        if (s[0] == '-') {
            CALL(VecCString, minus_pat, push_back, /, s + 1);
        } else if (s[0] == '+') {
            CALL(VecCString, plus_pat, push_back, /, s + 1);
        } else {
            CALL(VecCString, plus_pat, push_back, /, s);
        }
    }

    for (usize i = 0; i < n_tests; i++) {
        const TestEntry *test = tests + i;
        const char *name = test->test_name;
        bool all_match = true;
        for (usize j = 0; all_match && j < plus_pat.size; j++) {
            if (!strstr(name, plus_pat.data[j])) {
                all_match = false;
                break;
            }
        }
        for (usize j = 0; all_match && j < minus_pat.size; j++) {
            if (strstr(name, minus_pat.data[j])) {
                all_match = false;
                break;
            }
        }
        if (all_match) {
            CALL(VecTest, test_run, push_back, /, *test);
        } else {
            CALL(VecTest, test_filter, push_back, /, *test);
        }
    }

    printf("\033[0;36mtests to run    : \033[0m");
    for (usize i = 0; i < test_run.size; i++) {
        printf("\033[0;33m%s \033[0m", test_run.data[i].test_name);
    }
    printf("\n");
    printf("\033[0;36mtests filtered  : \033[0m");
    for (usize i = 0; i < test_filter.size; i++) {
        printf("\033[0;33m%s \033[0m", test_filter.data[i].test_name);
    }
    printf("\n\n");

    for (usize i = 0; i < test_run.size; i++) {
        const TestEntry *test = test_run.data + i;
        bool shall_exit = false;
        int exit_code = 0;
        fork_to_run_test(test, &shall_exit, &exit_code);
        if (shall_exit) {
            // subprocess, free the vectors to avoid memory leak false positive
            // in valgrind. Not necessary in normal execution.
            DROPOBJ(VecTest, test_filter);
            DROPOBJ(VecTest, test_run);
            DROPOBJ(VecCString, minus_pat);
            DROPOBJ(VecCString, plus_pat);
            exit(exit_code);
        }
    }

    DROPOBJ(VecTest, test_filter);
    DROPOBJ(VecTest, test_run);
    DROPOBJ(VecCString, minus_pat);
    DROPOBJ(VecCString, plus_pat);
}

static void fork_to_run_test(const TestEntry *test, bool *shall_exit,
                             int *exit_code) {
    printf("\033[0;36m:: Start running test: \033[0m\033[0;33m%s\033[0m\n",
           test->test_name);

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        *shall_exit = true;
        *exit_code = EXIT_FAILURE;
    } else if (pid == 0) {
        test->test_fn();
        *shall_exit = true;
        *exit_code = EXIT_SUCCESS;
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (!(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS)) {
            printf(
                "\033[0;31m!! Failure on    test: \033[0m\033[0;33m%s\033[0m\n",
                test->test_name);
            *shall_exit = true;
            *exit_code = EXIT_FAILURE;
        } else {
            printf(
                "\033[0;36m:: End   running test: \033[0m\033[0;33m%s\033[0m\n",
                test->test_name);
            *shall_exit = false;
        }
    }
}

#pragma once

#include <stdarg.h>
#include <stdio.h>

#include "ast.h"
#include "utils.h"

typedef enum SemError {
    SemErrorNoErr = 0,
    SemErrorVarUndef = 1,
    SemErrorFunUndef = 2,
    SemErrorVarRedef = 3,
    SemErrorFunRedef = 4,
    SemErrorAssignTypeErr = 5,
    SemErrorAssignToRValue = 6,
    SemErrorOPTypeErr = 7,
    SemErrorReturnTypeErr = 8,
    SemErrorCallArgParaMismatch = 9,
    SemErrorIndexIdTypeErr = 10,
    SemErrorCallIdTypeErr = 11,
    SemErrorIndexNonInt = 12,
    SemErrorFieldIdTypeErr = 13,
    SemErrorFieldNotExist = 14,
    SemErrorStructDefWrong = 15,
    SemErrorStructRedef = 16,
    SemErrorStructUndef = 17,
    SemErrorFunDecButNotDef = 18,
    SemErrorFunDecConflict = 19,
} SemError;

#define report_semerr(line_no, error, ...)                                     \
    ({                                                                         \
        printf("Error type %d at Line %d: semantic error" VA_OPT_ONE(          \
                   ", %s", ##__VA_ARGS__) "\n",                                \
               error, node->line_no, ##__VA_ARGS__);                           \
    })
#define report_semerr_fmt(line_no, error, format, ...)                         \
    ({                                                                         \
        printf("Error type %d at Line %d: semantic error, " format "\n",       \
               error, node->line_no, ##__VA_ARGS__);                           \
    })

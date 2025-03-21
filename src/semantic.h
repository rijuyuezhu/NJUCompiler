#pragma once
#include <stdio.h>

#include "utils.h"

typedef enum SemError {
    SemErrorInvalid = 0,
    SemErrorVarUndef = 1,
    SemErrorFunUndef = 2,
    SemErrorVarRedef = 3,
    SemErrorFunRedef = 4,
    SemErrorAssignTypeErr = 5,
    SemErrorAssignToRValue = 6,
    SemErrorOPTypeErr = 7,
    SemErrorReturnTypeErr = 8,
    SemErrorCallArgParaMismatch = 9,
    SemErrorIndexBaseTypeErr = 10,
    SemErrorCallBaseTypeErr = 11,
    SemErrorIndexNonInt = 12,
    SemErrorFieldBaseTypeErr = 13,
    SemErrorFieldNotExist = 14,
    SemErrorStructDefWrong = 15,
    SemErrorStructRedef = 16,
    SemErrorStructUndef = 17,
    SemErrorFunDecButNotDef = 18,
    SemErrorFunDecConflict = 19,
} SemError;

/* SemResolver */

typedef struct SemResolver {
    struct TypeManager *type_manager;
    struct SymbolManager *symbol_manager;
    usize current_symtab_idx;
    bool sem_error;
} SemResolver;

void MTD(SemResolver, init, /, struct TypeManager *type_manager,
         struct SymbolManager *symbol_manager);
FUNC_STATIC DEFAULT_DROPER(SemResolver);

struct AstNode;
void MTD(SemResolver, resolve, /, struct AstNode *node);

/* semerr reporter */

#define report_semerr(line_no, error, ...)                                     \
    ({                                                                         \
        printf("Error type %d at Line %d: semantic error" VA_OPT_ONE(          \
                   ", %s", ##__VA_ARGS__) ".\n",                               \
               error, line_no, ##__VA_ARGS__);                                 \
        self->sem_error = true;                                                \
    })

#define report_semerr_fmt(line_no, error, format, ...)                         \
    ({                                                                         \
        printf("Error type %d at Line %d: semantic error, " format ".\n",      \
               error, line_no, ##__VA_ARGS__);                                 \
        self->sem_error = true;                                                \
    })

#pragma once

#include "utils.h"

struct TypeManager;
struct SymbolManager;
struct AstNode;

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
    usize cur_symtab_idx;
    bool sem_error;
} SemResolver;

void MTD(SemResolver, init, /, struct TypeManager *type_manager,
         struct SymbolManager *symbol_manager);
FUNC_STATIC DEFAULT_DROPER(SemResolver);

void MTD(SemResolver, resolve, /, struct AstNode *node, bool add_builtin);

/* semerr reporter */

#define report_semerr(line_no, error, ...)                                     \
    ({                                                                         \
        printf("Error type %d at Line %d: semantic error" VA_ARGS_FIRST(       \
                   __VA_ARGS__) ".\n",                                         \
               error, line_no VA_ARGS_EXCEPT_FIRST(__VA_ARGS__));              \
        self->sem_error = true;                                                \
    })

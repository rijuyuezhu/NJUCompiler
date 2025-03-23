#include "semantic.h"
#include "ast.h"
#include "grammar_symbol.h"
#include "str.h"
#include "symbol.h"
#include "type.h"
#include "utils.h"

typedef struct SemPassInfo {
    // inherits; requires initialization
    usize inherit_type_idx;
    usize enclosing_struct_type_idx;
    bool is_fun_dec;
    bool is_in_struct;

    // synthesized
    struct String *gen_symbol_str;
    usize gen_symtab_idx;
} SemPassInfo;

static void MTD(SemPassInfo, init, /, usize inherit_type_idx,
                usize enclosing_struct_type_idx, bool is_fun_dec,
                bool is_in_struct) {
    self->inherit_type_idx = inherit_type_idx;
    self->enclosing_struct_type_idx = enclosing_struct_type_idx;
    self->is_fun_dec = is_fun_dec;
    self->is_in_struct = is_in_struct;

    self->gen_symbol_str = NULL;
    self->gen_symtab_idx = -1;
}

void MTD(SemResolver, init, /, TypeManager *type_manager,
         SymbolManager *symbol_manager) {
    self->type_manager = type_manager;
    self->symbol_manager = symbol_manager;
    self->cur_symtab_idx = self->symbol_manager->root_idx;
    self->sem_error = false;
}

// Visitor operations

#define VISITOR(gs)                                                            \
    static void MTD(SemResolver, CONCATENATE(visit_, gs), /, AstNode * node,   \
                    ATTR_UNUSED SemPassInfo * info)

#define DISPATCH(gs)                                                           \
    ({ CALL(SemResolver, *self, CONCATENATE(visit_, gs), /, node, info); })

// Declare Visitor functions;
// e.g. void MTD(SemResolver, visit_SEMI, /, AstNode * node); etc
#define DECLVISITOR_SEMRESOLVER_GRAMMAR_SYMBOL_AID(gs) VISITOR(gs);
APPLY_GRAMMAR_SYMBOL_SYNTAX(DECLVISITOR_SEMRESOLVER_GRAMMAR_SYMBOL_AID);

// Info management
#define INFO ASSERT(info, "info must not be NULL")
#define NOINFO ASSERT(!info, "info must be NULL")
#define NEW_SINFO(...) SemPassInfo sinfo = CREOBJ(SemPassInfo, /, ##__VA_ARGS__)
#define SINFO(...) sinfo = CREOBJ(SemPassInfo, /, ##__VA_ARGS__)

// Data/flow management

#define NUM_CHILDREN() (node->children.size)
#define DATA_CHILD(idx)                                                        \
    ({                                                                         \
        ASSERT(idx < NUM_CHILDREN(), "index out of range");                    \
        (AstNode *)node->children.data[idx];                                   \
    })
#define VISIT_WITH(gs, idx, info)                                              \
    ({                                                                         \
        ASSERT(idx < NUM_CHILDREN(), "index out of range");                    \
        ASSERT(DATA_CHILD(idx)->grammar_symbol == CONCATENATE(GS_, gs),        \
               "unexpected grammar symbol");                                   \
        CALL(SemResolver, *self, CONCATENATE(visit_, gs), /, DATA_CHILD(idx),  \
             info);                                                            \
    })
#define VISIT_CHILD(gs, idx) VISIT_WITH(gs, idx, NULL);

// Type management

#define SAVE_TYPE_BASIC(basic_type)                                            \
    ({                                                                         \
        node->type_idx =                                                       \
            self->type_manager->CONCATENATE(basic_type, _type_idx);            \
    })
#define SAVE_TYPE(type) ({ node->type_idx = (type); })
#define SAVE_TYPE_CHILD(idx)                                                   \
    ({                                                                         \
        ASSERT(idx < NUM_CHILDREN(), "index out of range");                    \
        SAVE_TYPE(DATA_CHILD(idx)->type_idx);                                  \
    })

// Symtab management

#define SAVE_SYMTAB() ({ node->symtab_idx = self->cur_symtab_idx; })

#define GET_SYMTAB()                                                           \
    SymbolTable *now_symtab = CALL(SymbolManager, *self->symbol_manager,       \
                                   get_table, /, self->cur_symtab_idx)
#define GET_TOPSYMTAB()                                                        \
    SymbolTable *top_symtab =                                                  \
        CALL(SymbolManager, *self->symbol_manager, get_table, /,               \
             self->symbol_manager->root_idx)

#define CHNGPUSH_SYMTAB(idx)                                                   \
    usize orig_symtab_idx = self->cur_symtab_idx;                              \
    self->cur_symtab_idx = idx

#define PUSH_SYMTAB()                                                          \
    usize orig_symtab_idx = self->cur_symtab_idx;                              \
    self->cur_symtab_idx = CALL(SymbolManager, *self->symbol_manager,          \
                                add_table, /, self->cur_symtab_idx)
#define POP_SYMTAB() self->cur_symtab_idx = orig_symtab_idx

/* ------ */

void MTD(SemResolver, resolve, /, AstNode *node) {
    // first, do a visit
    CALL(SemResolver, *self, visit_Program, /, node, NULL);

    // then, check if there is any function that is declared but not defined
    GET_TOPSYMTAB();
    MapSymtab *top_mapping = &top_symtab->mapping;
    MapSymtabIterator it = CALL(MapSymtab, *top_mapping, begin, /);
    while (it) {
        if (it->value.kind == SymbolEntryFun && !it->value.as_fun.is_defined) {
            report_semerr_fmt(it->value.as_fun.first_decl_line_no,
                              SemErrorFunDecButNotDef,
                              "function \"%s\" declared but not defined",
                              STRING_C_STR(it->key.s));
        }
        it = CALL(MapSymtab, *top_mapping, next, /, it);
    }
}

// clang-format off
// Program         NOINFO | void
// ExtDefList      NOINFO | void
// ExtDef          NOINFO | void
// ExtDecList      {give: inherit_type_idx}
//                     - inherit_type_idx: the type of the declaration
// Specifier       NOINFO | [int, float, (Sturct type from StructSpecifier)]
// StructSpecifier NOINFO | (Struct type)
// OptTag          {expect: gen_symbol_str @ nullable} | void
//                     - gen_symbol_str: the name of the struct
// Tag             {expect: gen_symbol_str} | void
//                     - gen_symbol_str: the name of the struct
// VarDec          {give: inherit_type_idx, expect: gen_symbol_str} | syn_type
//                     - inherit_type_idx: the subtype of the array
//                     - gen_symbol_str: the name of the var
//                     - syn_type: type formed from now and the suffix
//                     sym_gen is due to the father
// FunDec          {give: inherit_type_idx, is_fun_dec; expect: gen_symtab_idx} | func_type
//                     - inherit_type_idx: the return type of the function
//                     - is_fun_dec: whether it is a function declaration
//                     - gen_symtab_idx: the new symbol table index for the func
//                     - func_type: the complete type of the function
// VarList         {give: inherit_type_idx} | void
//                     - inherit_type_idx: the belonging function type
// ParamDec        {give: inherit_type_idx} | void
//                     - inherit_type_idx: the belonging function type
// CompSt          {give: inherit_type_idx} | void
//                     - inherit_type_idx: the return type of the function
// StmtList        {give: inherit_type_idx} | void
//                     - inherit_type_idx: the return type of the function
// Stmt            {give: inherit_type_idx} | void
//                     - inherit_type_idx: the return type of the function
// DefList         {give: enclosing_struct_type_idx, is_in_struct} | void
//                     - enclosing_struct_type_idx: the struct type of the enclosing struct (if is_in_struct == true)
//                     - is_in_struct: whether the DefList is in a struct
// Def             {give: enclosing_struct_type_idx, is_in_struct} | void
//                     - enclosing_struct_type_idx: the struct type of the enclosing struct (if is_in_struct == true)
//                     - is_in_struct: whether the Def is in a struct
// DecList         {give: inherit_type_idx, enclosing_struct_type_idx, is_in_struct} | void
//                     - inherit_type_idx: the type of the declaration (local)
//                     - enclosing_struct_type_idx: the struct type of the enclosing struct (if is_in_struct == true)
//                     - is_in_struct: whether the DecList is in a struct
// Dec             {give: inherit_type_idx, enclosing_struct_type_idx, is_in_struct} | void
//                     - inherit_type_idx: the type of the declaration (local)
//                     - enclosing_struct_type_idx: the struct type of the enclosing struct (if is_in_struct == true)
//                     - is_in_struct: whether the Dec is in a struct
// Exp             NOINFO | (type of the expresion)
// Args            {give: inherit_type_idx} | void
//                     - inherit_type_idx: the type that is being determined (func call type)
// clang-format on

void MTD(SemResolver, insert_var_dec, /, String *symbol_name, usize type_idx,
         AstNode *node, SemPassInfo *info) {
    ASSERT(symbol_name);
    // key here is a hack! Do not drop it
    HString key = NSCALL(HString, from_inner, /, *symbol_name);

    GET_SYMTAB();
    // recursively find to avoid name conflict with structs
    MapSymtabIterator it = CALL(SymbolTable, *now_symtab, find_recursive, /,
                                &key, self->symbol_manager);
    bool can_insert_in = false;
    if (it == NULL) {
        can_insert_in = true;
    } else if (it->value.kind == SymbolEntryVar &&
               it->value.table != now_symtab) {
        can_insert_in = true;
    } else {
        if (info->is_in_struct) {
            report_semerr_fmt(node->line_no, SemErrorStructDefWrong,
                              "redefinition of field \"%s\"",
                              STRING_C_STR(*symbol_name));
        } else {
            report_semerr_fmt(node->line_no, SemErrorVarRedef,
                              "redefinition of variable \"%s\"",
                              STRING_C_STR(*symbol_name));
        }
    }

    if (can_insert_in) {
        HString key_to_insert = CALL(HString, key, clone, /);
        MapSymtabInsertResult result =
            CALL(SymbolTable, *now_symtab, insert, /, key_to_insert,
                 SymbolEntryVar, type_idx);
        ASSERT(result.inserted, "insertion shall be successful");
    }
}

// Program -> ExtDefList
VISITOR(Program) {
    NOINFO;
    SAVE_SYMTAB();
    VISIT_CHILD(ExtDefList, 0);
    SAVE_TYPE_BASIC(void);
}

// ExtDefList -> ExtDef ExtDefList
// ExtDefList -> \epsilon
VISITOR(ExtDefList) {
    NOINFO;
    SAVE_SYMTAB();
    if (NUM_CHILDREN() == 2) {
        // ExtDefList -> ExtDef ExtDefList
        VISIT_CHILD(ExtDef, 0);
        VISIT_CHILD(ExtDefList, 1);
    } else {
        // ExtDefList -> \epsilon
    }
    SAVE_TYPE_BASIC(void);
}

// ExtDef -> Specifier ExtDecList SEMI
VISITOR(ExtDefCase1) {
    NOINFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Specifier, 0);

    usize decl_type_idx = DATA_CHILD(0)->type_idx;
    NEW_SINFO(decl_type_idx, -1, false, false);
    VISIT_WITH(ExtDecList, 1, &sinfo);
    SAVE_TYPE_BASIC(void);
}

// ExtDef -> Specifier SEMI
VISITOR(ExtDefCase2) {
    NOINFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Specifier, 0);
    SAVE_TYPE_BASIC(void);
}

// ExtDef -> Specifier FunDec CompSt
VISITOR(ExtDefCase3) {
    NOINFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Specifier, 0);

    usize ret_type_idx = DATA_CHILD(0)->type_idx;
    NEW_SINFO(ret_type_idx, -1, false, false);
    VISIT_WITH(FunDec, 1, &sinfo);

    usize func_symtab_idx = sinfo.gen_symtab_idx;

    SINFO(ret_type_idx, -1, false, false);
    CHNGPUSH_SYMTAB(func_symtab_idx);
    VISIT_WITH(CompSt, 2, &sinfo);
    POP_SYMTAB();
    SAVE_TYPE_BASIC(void);
}

// ExtDef -> Specifier FunDec SEMI
VISITOR(ExtDefCase4) {
    NOINFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Specifier, 0);

    usize ret_type_idx = DATA_CHILD(0)->type_idx;
    NEW_SINFO(ret_type_idx, -1, true, false);
    VISIT_WITH(FunDec, 1, &sinfo);
    SAVE_TYPE_BASIC(void);
}

// ExtDef Dispatcher
// - ExtDef -> Specifier ExtDecList SEMI
// - ExtDef -> Specifier SEMI
// - ExtDef -> Specifier FunDec CompSt
// - ExtDef -> Specifier FunDec SEMI
VISITOR(ExtDef) {
    if (NUM_CHILDREN() == 2) {
        // ExtDef -> Specifier SEMI
        DISPATCH(ExtDefCase2);
    } else if (DATA_CHILD(1)->grammar_symbol == GS_ExtDecList) {
        // ExtDef -> Specifier ExtDecList SEMI
        DISPATCH(ExtDefCase1);
    } else if (DATA_CHILD(2)->grammar_symbol == GS_CompSt) {
        // ExtDef -> Specifier FunDec CompSt
        DISPATCH(ExtDefCase3);
    } else {
        // ExtDef -> Specifier FunDec SEMI
        DISPATCH(ExtDefCase4);
    }
}

// ExtDecList -> VarDec COMMA ExtDecList
// ExtDecList -> VarDec
VISITOR(ExtDecList) {
    INFO;
    SAVE_SYMTAB();

    VISIT_WITH(VarDec, 0, info);

    String *symbol_name = info->gen_symbol_str;
    ASSERT(symbol_name);
    usize var_type_idx = DATA_CHILD(0)->type_idx;
    CALL(SemResolver, *self, insert_var_dec, /, symbol_name, var_type_idx, node,
         info);

    if (NUM_CHILDREN() == 3) {
        // ExtDecList -> VarDec COMMA ExtDecList
        VISIT_WITH(ExtDecList, 2, info);
    } else {
        // ExtDecList -> VarDec
    }
    SAVE_TYPE_BASIC(void);
}

// Specifier -> TYPE
VISITOR(SpecifierCase1) {
    NOINFO;
    SAVE_SYMTAB();
    const char *type_str = DATA_CHILD(0)->str_val.data;
    if (type_str[0] == 'i') {
        SAVE_TYPE_BASIC(int);
    } else {
        SAVE_TYPE_BASIC(float);
    }
}

// Specifier -> StructSpecifier
VISITOR(SpecifierCase2) {
    NOINFO;
    SAVE_SYMTAB();
    VISIT_CHILD(StructSpecifier, 0);
    SAVE_TYPE_CHILD(0);
}

// Specifier Dispatcher
// - Specifier -> TYPE
// - Specifier -> StructSpecifier
VISITOR(Specifier) {
    if (DATA_CHILD(0)->grammar_symbol == GS_TYPE) {
        // Specifier -> TYPE
        DISPATCH(SpecifierCase1);
    } else {
        // Specifier -> StructSpecifier
        DISPATCH(SpecifierCase2);
    }
}

// StructSpecifier -> STRUCT OptTag LC DefList RC
VISITOR(StructSpecifierCase1) {
    NOINFO;
    SAVE_SYMTAB();

    NEW_SINFO(-1, -1, false, false);
    VISIT_WITH(OptTag, 1, &sinfo);

    // construct the struct type
    usize struct_symtab_idx = CALL(SymbolManager, *self->symbol_manager,
                                   add_table, /, self->cur_symtab_idx);
    usize struct_type_idx = CALL(TypeManager, *self->type_manager, make_struct,
                                 /, struct_symtab_idx);

    String *struct_tag = sinfo.gen_symbol_str;

    // struct body requires a new symtab
    SINFO(-1, struct_type_idx, false, true);
    CHNGPUSH_SYMTAB(struct_symtab_idx);
    VISIT_WITH(DefList, 3, &sinfo);
    POP_SYMTAB();

    // The struct type is now ready; get its representative
    CALL(TypeManager, *self->type_manager, fill_in_repr, /, struct_type_idx);

    // try to fill the tag in the symtab
    if (struct_tag) {
        // key here is a hack! Do not drop it
        HString key = NSCALL(HString, from_inner, /, *struct_tag);

        // check if the struct is already defined
        GET_SYMTAB();
        MapSymtabIterator it = CALL(SymbolTable, *now_symtab, find_recursive, /,
                                    &key, self->symbol_manager);
        if (it == NULL) {
            // ok to insert it (in the top symtab)
            HString key_to_insert = CALL(HString, key, clone, /);

            GET_TOPSYMTAB();
            MapSymtabInsertResult result =
                CALL(SymbolTable, *top_symtab, insert, /, key_to_insert,
                     SymbolEntryStruct, struct_type_idx);
            ASSERT(result.inserted, "insertion shall be successful");
        } else {
            report_semerr_fmt(node->line_no, SemErrorStructRedef,
                              "redefinition of struct \"%s\"",
                              STRING_C_STR(*struct_tag));
        }
    } else {
        // An anonymous struct; we do not insert anything to the symtab
    }

    SAVE_TYPE(struct_type_idx);
}

// StructSpecifier -> STRUCT Tag
VISITOR(StructSpecifierCase2) {
    NOINFO;
    SAVE_SYMTAB();

    NEW_SINFO(-1, -1, false, false);
    VISIT_WITH(Tag, 1, &sinfo);

    String *struct_tag = sinfo.gen_symbol_str;
    ASSERT(struct_tag, "struct with `Tag` must not be NULL");

    // key here is a hack! Do not drop it
    HString key = NSCALL(HString, from_inner, /, *struct_tag);

    // check if the struct is already defined
    GET_SYMTAB();
    MapSymtabIterator it = CALL(SymbolTable, *now_symtab, find_recursive, /,
                                &key, self->symbol_manager);

    if (it == NULL || it->value.kind != SymbolEntryStruct) {
        report_semerr_fmt(node->line_no, SemErrorStructUndef,
                          "undefined struct \"%s\"", STRING_C_STR(*struct_tag));
        SAVE_TYPE_BASIC(void);
    } else {
        SAVE_TYPE(it->value.type_idx);
    }
}

// StructSpecifier Dispatcher
// - StructSpecifier -> STRUCT OptTag LC DefList RC
// - StructSpecifier -> STRUCT Tag
VISITOR(StructSpecifier) {
    if (NUM_CHILDREN() == 5) {
        // StructSpecifier -> STRUCT OptTag LC DefList RC
        DISPATCH(StructSpecifierCase1);
    } else {
        // StructSpecifier -> STRUCT Tag
        DISPATCH(StructSpecifierCase2);
    }
}

// OptTag -> ID
// OptTag -> \epsilon
VISITOR(OptTag) {
    INFO;
    SAVE_SYMTAB();
    if (NUM_CHILDREN() == 1) {
        // OptTag -> ID
        info->gen_symbol_str = &DATA_CHILD(0)->str_val;
    } else {
        // OptTag -> \epsilon
        info->gen_symbol_str = NULL;
    }
    SAVE_TYPE_BASIC(void);
}

// Tag -> ID
VISITOR(Tag) {
    INFO;
    SAVE_SYMTAB();
    info->gen_symbol_str = &DATA_CHILD(0)->str_val;
    SAVE_TYPE_BASIC(void);
}

// VarDec -> ID
VISITOR(VarDecCase1) {
    INFO;
    SAVE_SYMTAB();

    info->gen_symbol_str = &DATA_CHILD(0)->str_val;
    usize subtype_idx = info->inherit_type_idx;
    SAVE_TYPE(subtype_idx);
}

// VarDec -> VarDec LB INT RB
VISITOR(VarDecCase2) {
    INFO;
    SAVE_SYMTAB();

    usize size = DATA_CHILD(2)->int_val;
    usize subtype_idx = info->inherit_type_idx;
    usize type_idx = CALL(TypeManager, *self->type_manager, make_array, /, size,
                          subtype_idx);
    // The array type is now ready; get its representative
    CALL(TypeManager, *self->type_manager, fill_in_repr, /, type_idx);

    NEW_SINFO(type_idx, -1, false, false);
    VISIT_WITH(VarDec, 0, &sinfo);

    info->gen_symbol_str = sinfo.gen_symbol_str;
    SAVE_TYPE_CHILD(0);
}

// VarDec Dispatcher
// - VarDec -> ID
// - VarDec -> VarDec LB INT RB
VISITOR(VarDec) {
    if (NUM_CHILDREN() == 1) {
        // VarDec -> ID
        DISPATCH(VarDecCase1);
    } else {
        // VarDec -> VarDec LB INT RB
        DISPATCH(VarDecCase2);
    }
}

// FunDec -> ID LP VarList RP
// FunDec -> ID LP RP
VISITOR(FunDec) {
    INFO;
    SAVE_SYMTAB();

    usize ret_type_idx = info->inherit_type_idx;
    bool is_fun_dec = info->is_fun_dec;

    // create the function type, and add the return type as its 0th-elem
    usize func_type_idx = CALL(TypeManager, *self->type_manager, make_fun, /);
    CALL(TypeManager, *self->type_manager, add_fun_ret_par, /, func_type_idx,
         ret_type_idx);

    // Handle VarList

    PUSH_SYMTAB();
    info->gen_symtab_idx = self->cur_symtab_idx;

    if (NUM_CHILDREN() == 4) {
        // FunDec -> ID LP VarList RP
        NEW_SINFO(func_type_idx, -1, false, false);
        VISIT_WITH(VarList, 2, &sinfo);
    } else {
        // FunDec -> ID LP RP
        // do nothing
    }
    POP_SYMTAB();

    // The fun_type is now ready; get its representative
    CALL(TypeManager, *self->type_manager, fill_in_repr, /, func_type_idx);

    // Handle fun_name

    String *fun_name = &DATA_CHILD(0)->str_val;

    // key here is a hack! Do not drop it
    HString key = NSCALL(HString, from_inner, /, *fun_name);

    GET_SYMTAB();
    MapSymtabIterator it = CALL(SymbolTable, *now_symtab, find_recursive, /,
                                &key, self->symbol_manager);

    if (it == NULL) {
        // ok to insert it in.
        HString key_to_insert = CALL(HString, key, clone, /);
        MapSymtabInsertResult result =
            CALL(SymbolTable, *now_symtab, insert, /, key_to_insert,
                 SymbolEntryFun, func_type_idx);
        ASSERT(result.inserted, "insertion shall be successful");

        SymbolEntry *entry = &result.node->value;

        if (is_fun_dec) {
            // First time dec
            entry->as_fun.is_defined = false;
        } else {
            // First time dec + def
            entry->as_fun.is_defined = true;
        }
        entry->as_fun.first_decl_line_no = node->line_no;
    } else if (it->value.kind != SymbolEntryFun) {
        report_semerr_fmt(
            node->line_no, SemErrorFunRedef,
            "conflict definition with previous identifiers for function \"%s\"",
            STRING_C_STR(*fun_name));
    } else {
        SymbolEntry *entry = &it->value;
        if (!is_fun_dec && entry->as_fun.is_defined) {
            // double definition
            report_semerr_fmt(node->line_no, SemErrorFunRedef,
                              "redefinition of function \"%s\"",
                              STRING_C_STR(*fun_name));
        } else {
            if (!CALL(TypeManager, *self->type_manager, is_type_consistency, /,
                      func_type_idx, entry->type_idx)) {
                // not consistent
                report_semerr_fmt(node->line_no, SemErrorFunDecConflict,
                                  "conflicting types for function \"%s\"",
                                  STRING_C_STR(*fun_name));
            } else if (!is_fun_dec) {
                // consistent; now it is defined
                entry->as_fun.is_defined = true;
            }
        }
    }

    SAVE_TYPE(func_type_idx);
}

// VarList -> ParamDec COMMA VarList
// VarList -> ParamDec
VISITOR(VarList) {
    INFO;
    SAVE_SYMTAB();
    VISIT_WITH(ParamDec, 0, info);
    if (NUM_CHILDREN() == 3) {
        // VarList -> ParamDec COMMA VarList
        VISIT_WITH(VarList, 2, info);
    } else {
        // VarList -> ParamDec
        // do nothing
    }
    SAVE_TYPE_BASIC(void);
}

// ParamDec -> Specifier VarDec
VISITOR(ParamDec) {
    INFO;
    SAVE_SYMTAB();

    usize fun_type_idx = info->inherit_type_idx;

    VISIT_CHILD(Specifier, 0);
    usize decl_type_idx = DATA_CHILD(0)->type_idx;

    NEW_SINFO(decl_type_idx, -1, false, false);
    VISIT_WITH(VarDec, 1, &sinfo);

    String *symbol_name = sinfo.gen_symbol_str;
    ASSERT(symbol_name);

    usize param_type_idx = DATA_CHILD(1)->type_idx;
    CALL(SemResolver, *self, insert_var_dec, /, symbol_name, param_type_idx,
         node, info);

    CALL(TypeManager, *self->type_manager, add_fun_ret_par, /, fun_type_idx,
         param_type_idx);

    SAVE_TYPE_BASIC(void);
}

// CompSt -> LC DefList StmtList RC
VISITOR(CompSt) {
    INFO;
    SAVE_SYMTAB();

    NEW_SINFO(-1, -1, false, false);
    VISIT_WITH(DefList, 1, &sinfo);
    VISIT_WITH(StmtList, 2, info);

    SAVE_TYPE_BASIC(void);
}

// StmtList -> Stmt StmtList
// StmtList -> \epsilon
VISITOR(StmtList) {
    INFO;
    SAVE_SYMTAB();
    if (NUM_CHILDREN() == 2) {
        // StmtList -> Stmt StmtList
        VISIT_WITH(Stmt, 0, info);
        VISIT_WITH(StmtList, 1, info);
    } else {
        // StmtList -> \epsilon
        // do nothing
    }
    SAVE_TYPE_BASIC(void);
}

// Stmt -> Exp SEMI
VISITOR(StmtCase1) {
    INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 0);
    SAVE_TYPE_BASIC(void);
}

// Stmt -> CompSt
VISITOR(StmtCase2) {
    INFO;
    SAVE_SYMTAB();

    PUSH_SYMTAB();
    VISIT_WITH(CompSt, 0, info);
    POP_SYMTAB();

    SAVE_TYPE_BASIC(void);
}

// Stmt -> RETURN Exp SEMI
VISITOR(StmtCase3) {
    INFO;
    SAVE_SYMTAB();

    VISIT_CHILD(Exp, 1);

    usize ret_type_idx = info->inherit_type_idx;
    usize exp_type_idx = DATA_CHILD(1)->type_idx;

    if (!CALL(TypeManager, *self->type_manager, is_type_consistency, /,
              ret_type_idx, exp_type_idx)) {
        report_semerr(node->line_no, SemErrorReturnTypeErr,
                      "return type mismatched");
    }

    SAVE_TYPE_BASIC(void);
}

// Stmt -> IF LP Exp RP Stmt
VISITOR(StmtCase4) {
    INFO;
    SAVE_SYMTAB();

    VISIT_CHILD(Exp, 2);
    usize cond_type_idx = DATA_CHILD(2)->type_idx;
    if (!CALL(TypeManager, *self->type_manager, is_type_consistency, /,
              cond_type_idx, self->type_manager->int_type_idx)) {
        report_semerr(node->line_no, SemErrorOPTypeErr,
                      "if condition must be of type int");
    }
    VISIT_WITH(Stmt, 4, info);

    SAVE_TYPE_BASIC(void);
}

// Stmt -> IF LP Exp RP Stmt ELSE Stmt
VISITOR(StmtCase5) {
    INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 2);
    usize cond_type_idx = DATA_CHILD(2)->type_idx;
    if (!CALL(TypeManager, *self->type_manager, is_type_consistency, /,
              cond_type_idx, self->type_manager->int_type_idx)) {
        report_semerr(node->line_no, SemErrorOPTypeErr,
                      "if condition must be of type int");
    }
    VISIT_WITH(Stmt, 4, info);
    VISIT_WITH(Stmt, 6, info);
    SAVE_TYPE_BASIC(void);
}

// Stmt -> WHILE LP Exp RP Stmt
VISITOR(StmtCase6) {
    INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 2);
    usize cond_type_idx = DATA_CHILD(2)->type_idx;
    if (!CALL(TypeManager, *self->type_manager, is_type_consistency, /,
              cond_type_idx, self->type_manager->int_type_idx)) {
        report_semerr(node->line_no, SemErrorOPTypeErr,
                      "while condition must be of type int");
    }
    VISIT_WITH(Stmt, 4, info);
    SAVE_TYPE_BASIC(void);
}

// Stmt Dispatcher
// - Stmt -> Exp SEMI
// - Stmt -> CompSt
// - Stmt -> RETURN Exp SEMI
// - Stmt -> IF LP Exp RP Stmt
// - Stmt -> IF LP Exp RP Stmt ELSE Stmt
// - Stmt -> WHILE LP Exp RP Stmt
VISITOR(Stmt) {
    if (NUM_CHILDREN() == 2) {
        // Stmt -> Exp SEMI
        DISPATCH(StmtCase1);
    } else if (NUM_CHILDREN() == 1) {
        // Stmt -> CompSt
        DISPATCH(StmtCase2);
    } else if (NUM_CHILDREN() == 3) {
        // Stmt -> RETURN Exp SEMI
        DISPATCH(StmtCase3);
    } else if (NUM_CHILDREN() == 7) {
        // Stmt -> IF LP Exp RP Stmt ELSE Stmt
        DISPATCH(StmtCase5);
    } else if (DATA_CHILD(0)->grammar_symbol == GS_IF) {
        // Stmt -> IF LP Exp RP Stmt
        DISPATCH(StmtCase4);
    } else {
        // Stmt -> WHILE LP Exp RP Stmt
        DISPATCH(StmtCase6);
    }
}

// DefList -> Def DefList
// DefList -> \epsilon
VISITOR(DefList) {
    INFO;
    SAVE_SYMTAB();

    if (NUM_CHILDREN() == 2) {
        // DefList -> Def DefList
        VISIT_WITH(Def, 0, info);
        VISIT_WITH(DefList, 1, info);
    } else {
        // DefList -> \epsilon
        // do nothing
    }
    SAVE_TYPE_BASIC(void);
}

// Def -> Specifier DecList SEMI
VISITOR(Def) {
    INFO;
    SAVE_SYMTAB();

    VISIT_CHILD(Specifier, 0);
    usize decl_type_idx = DATA_CHILD(0)->type_idx;

    NEW_SINFO(decl_type_idx, info->enclosing_struct_type_idx, false,
              info->is_in_struct);
    VISIT_WITH(DecList, 1, &sinfo);

    SAVE_TYPE_BASIC(void);
}

// DecList -> Dec
// DecList -> Dec COMMA DecList
VISITOR(DecList) {
    INFO;
    SAVE_SYMTAB();
    VISIT_WITH(Dec, 0, info);
    if (NUM_CHILDREN() == 3) {
        // DecList -> Dec COMMA DecList
        VISIT_WITH(DecList, 2, info);
    } else {
        // DecList -> Dec
        // do nothing
    }
    SAVE_TYPE_BASIC(void);
}

// Dec -> VarDec
VISITOR(DecCase1) {
    INFO;
    SAVE_SYMTAB();

    usize decl_type_idx = info->inherit_type_idx;

    NEW_SINFO(decl_type_idx, -1, false, false);
    VISIT_WITH(VarDec, 0, &sinfo);

    String *symbol_name = sinfo.gen_symbol_str;
    ASSERT(symbol_name);
    usize var_type_idx = DATA_CHILD(0)->type_idx;
    CALL(SemResolver, *self, insert_var_dec, /, symbol_name, var_type_idx, node,
         info);

    if (info->is_in_struct) {
        CALL(TypeManager, *self->type_manager, add_struct_field, /,
             info->enclosing_struct_type_idx, var_type_idx);
    }
    SAVE_TYPE_BASIC(void);
}

// Dec -> VarDec ASSIGNOP Exp
VISITOR(DecCase2) {
    INFO;
    SAVE_SYMTAB();

    if (info->is_in_struct) {
        report_semerr(node->line_no, SemErrorStructDefWrong,
                      "initialization in struct definition");
    }

    usize decl_type_idx = info->inherit_type_idx;
    NEW_SINFO(decl_type_idx, -1, false, false);

    VISIT_WITH(VarDec, 0, &sinfo);
    String *symbol_name = sinfo.gen_symbol_str;
    ASSERT(symbol_name);

    usize var_type_idx = DATA_CHILD(0)->type_idx;
    CALL(SemResolver, *self, insert_var_dec, /, symbol_name, var_type_idx, node,
         info);

    if (info->is_in_struct) {
        CALL(TypeManager, *self->type_manager, add_struct_field, /,
             info->enclosing_struct_type_idx, var_type_idx);
    }

    VISIT_CHILD(Exp, 2);

    if (!info->is_in_struct) {
        usize exp_type_idx = DATA_CHILD(2)->type_idx;

        if (!CALL(TypeManager, *self->type_manager, is_type_consistency, /,
                  var_type_idx, exp_type_idx)) {
            report_semerr(node->line_no, SemErrorAssignTypeErr,
                          "assignment type mismatched");
        }
    }
    SAVE_TYPE_BASIC(void);
}

// Dec Dispatcher
// - Dec -> VarDec
// - Dec -> VarDec ASSIGNOP Exp
VISITOR(Dec) {
    if (NUM_CHILDREN() == 1) {
        // Dec -> VarDec
        DISPATCH(DecCase1);
    } else {
        // Dec -> VarDec ASSIGNOP Exp
        DISPATCH(DecCase2);
    }
}

static bool MTD(SemResolver, is_lvalue, /, AstNode *node) {
    if (node->grammar_symbol != GS_Exp) {
        return false;
    }
    if (NUM_CHILDREN() == 1 && DATA_CHILD(0)->grammar_symbol == GS_ID) {
        return true;
    }
    if (NUM_CHILDREN() == 4 && DATA_CHILD(1)->grammar_symbol == GS_LB) {
        return true;
    }
    if (NUM_CHILDREN() == 3 && DATA_CHILD(1)->grammar_symbol == GS_DOT) {
        return true;
    }
    return false;
}

#define PREPARE_BINARY                                                         \
    NOINFO;                                                                    \
    SAVE_SYMTAB();                                                             \
    VISIT_CHILD(Exp, 0);                                                       \
    VISIT_CHILD(Exp, 2);                                                       \
    usize left_type_idx = DATA_CHILD(0)->type_idx;                             \
    usize right_type_idx = DATA_CHILD(2)->type_idx;                            \
    usize syn_type_idx = (left_type_idx == self->type_manager->void_type_idx)  \
                             ? right_type_idx                                  \
                             : left_type_idx;                                  \
    bool has_err = false;

#define FINISH_BINARY                                                          \
    SAVE_TYPE(has_err ? self->type_manager->void_type_idx : syn_type_idx);

static bool MTD(SemResolver, is_int_type, /, usize type_idx) {
    return type_idx == self->type_manager->int_type_idx ||
           type_idx == self->type_manager->void_type_idx;
}

static bool MTD(SemResolver, is_basic_type, /, usize type_idx) {
    return type_idx == self->type_manager->int_type_idx ||
           type_idx == self->type_manager->float_type_idx ||
           type_idx == self->type_manager->void_type_idx;
}

static bool MTD(SemResolver, is_arith_type_with_error, /, AstNode *node,
                usize left_type_idx, usize right_type_idx) {
    if (!CALL(SemResolver, *self, is_basic_type, /, left_type_idx)) {
        report_semerr(
            node->line_no, SemErrorOPTypeErr,
            "arithmetic operator requires the left operand of type int "
            "or float");
        return false;
    } else if (!CALL(SemResolver, *self, is_basic_type, /, right_type_idx)) {
        report_semerr(
            node->line_no, SemErrorOPTypeErr,
            "arithmetic operator requires the right operand of type int "
            "or float");
        return false;
    } else if (!CALL(TypeManager, *self->type_manager, is_type_consistency, /,
                     left_type_idx, right_type_idx)) {
        report_semerr(node->line_no, SemErrorOPTypeErr,
                      "arithmetic operator requires two operands of the same "
                      "type");
        return false;
    }
    return true;
}

// Exp -> Exp ASSIGNOP Exp
VISITOR(ExpASSIGNOP) {
    PREPARE_BINARY;

    if (!CALL(TypeManager, *self->type_manager, is_type_consistency, /,
              left_type_idx, right_type_idx)) {
        report_semerr(node->line_no, SemErrorAssignTypeErr,
                      "assignment type mismatched");
        has_err = true;
    }

    // check if left_type_idx is lvalue
    bool is_lvalue = CALL(SemResolver, *self, is_lvalue, /, DATA_CHILD(0));
    if (!is_lvalue) {
        report_semerr(node->line_no, SemErrorAssignToRValue,
                      "assignment to non-lvalue");
        has_err = true;
    }
    FINISH_BINARY;
}

// Exp -> Exp AND Exp
VISITOR(ExpAND) {
    PREPARE_BINARY;
    if (!CALL(SemResolver, *self, is_int_type, /, left_type_idx) ||
        !CALL(SemResolver, *self, is_int_type, /, right_type_idx)) {
        report_semerr(node->line_no, SemErrorOPTypeErr,
                      "AND operator requires two operands of type int");
        has_err = true;
    }
    FINISH_BINARY;
}

// Exp -> Exp OR Exp
VISITOR(ExpOR) {
    PREPARE_BINARY;
    if (!CALL(SemResolver, *self, is_int_type, /, left_type_idx) ||
        !CALL(SemResolver, *self, is_int_type, /, right_type_idx)) {
        report_semerr(node->line_no, SemErrorOPTypeErr,
                      "OR operator requires two operands of type int");
        has_err = true;
    }
    FINISH_BINARY;
}

#define ARITH_CHECK                                                            \
    if (!CALL(SemResolver, *self, is_arith_type_with_error, /, node,           \
              left_type_idx, right_type_idx)) {                                \
        has_err = true;                                                        \
    }

// Exp -> Exp RELOP Exp
VISITOR(ExpRELOP) {
    PREPARE_BINARY;
    ARITH_CHECK;
    syn_type_idx = self->type_manager->int_type_idx;
    FINISH_BINARY;
}

// Exp -> Exp PLUS Exp
VISITOR(ExpPLUS) {
    PREPARE_BINARY;
    ARITH_CHECK;
    FINISH_BINARY;
}

// Exp -> Exp MINUS Exp
VISITOR(ExpMINUS) {
    PREPARE_BINARY;
    ARITH_CHECK;
    FINISH_BINARY;
}

// Exp -> Exp STAR Exp
VISITOR(ExpSTAR) {
    PREPARE_BINARY;
    ARITH_CHECK;
    FINISH_BINARY;
}

// Exp -> Exp DIV Exp
VISITOR(ExpDIV) {
    PREPARE_BINARY;
    ARITH_CHECK;
    FINISH_BINARY;
}

// Exp -> LP Exp RP
VISITOR(ExpParen) {
    NOINFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 1);
    SAVE_TYPE_CHILD(1);
}

// Exp -> MINUS Exp
VISITOR(ExpUMINUS) {
    NOINFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 1);
    usize type_idx = DATA_CHILD(1)->type_idx;
    if (!CALL(SemResolver, *self, is_basic_type, /, type_idx)) {
        report_semerr(node->line_no, SemErrorOPTypeErr,
                      "unary minus operator requires operand of type int or "
                      "float");
        SAVE_TYPE_BASIC(void);
    } else {
        SAVE_TYPE(type_idx);
    }
}

// Exp -> NOT Exp
VISITOR(ExpNOT) {
    NOINFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 1);
    usize type_idx = DATA_CHILD(1)->type_idx;
    if (!CALL(SemResolver, *self, is_int_type, /, type_idx)) {
        report_semerr(node->line_no, SemErrorOPTypeErr,
                      "NOT operator requires operand of type int");
        SAVE_TYPE_BASIC(void);
    } else {
        SAVE_TYPE(type_idx);
    }
}

// Exp -> ID LP Args RP
// Exp -> ID LP RP
VISITOR(ExpCall) {
    NOINFO;
    SAVE_SYMTAB();

    usize call_type_idx = CALL(TypeManager, *self->type_manager, make_fun, /);

    // add a dummy (void) return type
    CALL(TypeManager, *self->type_manager, add_fun_ret_par, /, call_type_idx,
         self->type_manager->void_type_idx);

    if (NUM_CHILDREN() == 4) {
        // Exp -> ID LP Args RP
        NEW_SINFO(call_type_idx, -1, false, false);
        VISIT_WITH(Args, 2, &sinfo);
    } else {
        // Exp -> ID LP RP
        // do nothing
    }

    String *name = &DATA_CHILD(0)->str_val;
    ASSERT(name);

    // key here is a hack! Do not drop it
    HString key = NSCALL(HString, from_inner, /, *name);

    GET_SYMTAB();
    MapSymtabIterator it = CALL(SymbolTable, *now_symtab, find_recursive, /,
                                &key, self->symbol_manager);
    if (it == NULL) {
        report_semerr_fmt(node->line_no, SemErrorFunUndef,
                          "undefined function \"%s\"", STRING_C_STR(*name));
    } else if (it->value.kind != SymbolEntryFun) {
        report_semerr_fmt(node->line_no, SemErrorCallBaseTypeErr,
                          "\"%s\" is not a function", STRING_C_STR(*name));
    } else if (!CALL(TypeManager, *self->type_manager,
                     is_type_consistency_with_fun_fix, /, call_type_idx,
                     it->value.type_idx)) {
        report_semerr_fmt(node->line_no, SemErrorCallArgParaMismatch,
                          "conflicting types for function \"%s\"",
                          STRING_C_STR(*name));
    }

    // if correctly match, check_type_consistency_with_fun_fix will fix the
    // return type.
    Type *type =
        CALL(TypeManager, *self->type_manager, get_type, /, call_type_idx);
    ASSERT(type->kind == TypeKindFun);
    usize ret_type_idx = type->as_fun.ret_par_idxes.data[0];

    // to save memory: pop the temporary type
    bool success_pop =
        CALL(TypeManager, *self->type_manager, pop_type, /, call_type_idx);
    ASSERT(success_pop, "pop should be successful");

    SAVE_TYPE(ret_type_idx);
}

// Exp -> Exp LB Exp RB
VISITOR(ExpIndex) {
    NOINFO;
    SAVE_SYMTAB();

    VISIT_CHILD(Exp, 0);
    VISIT_CHILD(Exp, 2);

    usize base_type_idx = DATA_CHILD(0)->type_idx;
    usize idx_type_idx = DATA_CHILD(2)->type_idx;

    Type *base_type =
        CALL(TypeManager, *self->type_manager, get_type, /, base_type_idx);

    if (base_type->kind != TypeKindArray) {
        report_semerr(
            node->line_no, SemErrorIndexBaseTypeErr,
            "index operator requires the base operand to be an array");
        SAVE_TYPE_BASIC(void);
    } else {
        if (!CALL(SemResolver, *self, is_int_type, /, idx_type_idx)) {
            report_semerr(
                node->line_no, SemErrorIndexNonInt,
                "index operator requires the index to be of type int");
        }
        usize ret_type_id = base_type->as_array.subtype_idx;
        SAVE_TYPE(ret_type_id);
    }
}

// Exp -> Exp DOT ID
VISITOR(ExpField) {
    NOINFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 0);
    usize base_type_idx = DATA_CHILD(0)->type_idx;
    Type *base_type =
        CALL(TypeManager, *self->type_manager, get_type, /, base_type_idx);
    if (base_type->kind != TypeKindStruct) {
        report_semerr(
            node->line_no, SemErrorFieldBaseTypeErr,
            "field operator requires the base operand to be a struct");
        SAVE_TYPE_BASIC(void);
    } else {
        String *field_name = &DATA_CHILD(2)->str_val;
        ASSERT(field_name);
        // key here is a hack! Do not drop it
        HString key = NSCALL(HString, from_inner, /, *field_name);
        SymbolTable *struct_symtab =
            CALL(SymbolManager, *self->symbol_manager, get_table, /,
                 base_type->as_struct.symtab_idx);

        // we do not find recursively: the fields are not inherited
        MapSymtabIterator it = CALL(SymbolTable, *struct_symtab, find, /, &key);
        if (it == NULL) {
            report_semerr_fmt(node->line_no, SemErrorFieldNotExist,
                              "field \"%s\" does not exist in struct",
                              STRING_C_STR(*field_name));
            SAVE_TYPE_BASIC(void);
        } else {
            usize field_type_idx = it->value.type_idx;
            SAVE_TYPE(field_type_idx);
        }
    }
}

// Exp -> ID
VISITOR(ExpID) {
    NOINFO;
    SAVE_SYMTAB();

    String *name = &DATA_CHILD(0)->str_val;
    ASSERT(name);

    // key here is a hack! Do not drop it
    HString key = NSCALL(HString, from_inner, /, *name);

    GET_SYMTAB();
    MapSymtabIterator it = CALL(SymbolTable, *now_symtab, find_recursive, /,
                                &key, self->symbol_manager);
    if (it == NULL) {
        report_semerr_fmt(node->line_no, SemErrorVarUndef,
                          "undefined variable \"%s\"", STRING_C_STR(*name));
        SAVE_TYPE_BASIC(void);
    } else if (it->value.kind != SymbolEntryVar) {
        report_semerr_fmt(node->line_no, SemErrorVarUndef,
                          "\"%s\" is not a variable", STRING_C_STR(*name));
        SAVE_TYPE_BASIC(void);
    } else {
        SAVE_TYPE(it->value.type_idx);
    }
}

// Exp -> INT
VISITOR(ExpINT) {
    NOINFO;
    SAVE_SYMTAB();
    SAVE_TYPE_BASIC(int);
}

// Exp -> FLOAT
VISITOR(ExpFLOAT) {
    NOINFO;
    SAVE_SYMTAB();
    SAVE_TYPE_BASIC(float);
}

// Exp Dispatcher
// 1. Exp -> Exp ASSIGNOP Exp
// 2. Exp -> Exp AND Exp
// 3. Exp -> Exp OR Exp
// 4. Exp -> Exp RELOP Exp
// 5. Exp -> Exp PLUS Exp
// 6. Exp -> Exp MINUS Exp
// 7. Exp -> Exp STAR Exp
// 8. Exp -> Exp DIV Exp
// 9. Exp -> LP Exp RP
// 10. Exp -> MINUS Exp
// 11. Exp -> NOT Exp
// 12. Exp -> ID LP Args RP
// 13. Exp -> ID LP RP
// 14. Exp -> Exp LB Exp RB
// 15. Exp -> Exp DOT ID
// 16. Exp -> ID
// 17. Exp -> INT
// 18. Exp -> FLOAT
VISITOR(Exp) {
    if (NUM_CHILDREN() == 3) {
        switch (DATA_CHILD(1)->grammar_symbol) {
        case GS_ASSIGNOP:
            // Exp -> Exp ASSIGNOP Exp
            DISPATCH(ExpASSIGNOP);
            break;
        case GS_AND:
            // Exp -> Exp AND Exp
            DISPATCH(ExpAND);
            break;
        case GS_OR:
            // Exp -> Exp OR Exp
            DISPATCH(ExpOR);
            break;
        case GS_RELOP:
            // Exp -> Exp RELOP Exp
            DISPATCH(ExpRELOP);
            break;
        case GS_PLUS:
            // Exp -> Exp PLUS Exp
            DISPATCH(ExpPLUS);
            break;
        case GS_MINUS:
            // Exp -> Exp MINUS Exp
            DISPATCH(ExpMINUS);
            break;
        case GS_STAR:
            // Exp -> Exp STAR Exp
            DISPATCH(ExpSTAR);
            break;
        case GS_DIV:
            // Exp -> Exp DIV Exp
            DISPATCH(ExpDIV);
            break;
        case GS_Exp:
            // Exp -> LP Exp RP
            DISPATCH(ExpParen);
            break;
        case GS_LP:
            // Exp -> ID LP RP
            DISPATCH(ExpCall);
            break;
        case GS_DOT:
            // Exp -> Exp DOT ID
            DISPATCH(ExpField);
            break;
        default:
            PANIC("unexpected grammar symbol");
        }
    } else if (NUM_CHILDREN() == 2) {
        if (DATA_CHILD(0)->grammar_symbol == GS_MINUS) {
            // Exp -> MINUS Exp
            DISPATCH(ExpUMINUS);
        } else if (DATA_CHILD(0)->grammar_symbol == GS_NOT) {
            // Exp -> NOT Exp
            DISPATCH(ExpNOT);
        } else {
            PANIC("unexpected grammar symbol");
        }
    } else if (NUM_CHILDREN() == 1) {
        if (DATA_CHILD(0)->grammar_symbol == GS_ID) {
            // Exp -> ID
            DISPATCH(ExpID);
        } else if (DATA_CHILD(0)->grammar_symbol == GS_INT) {
            // Exp -> INT
            DISPATCH(ExpINT);
        } else if (DATA_CHILD(0)->grammar_symbol == GS_FLOAT) {
            // Exp -> FLOAT
            DISPATCH(ExpFLOAT);
        } else {
            PANIC("unexpected grammar symbol");
        }
    } else if (NUM_CHILDREN() == 4) {
        if (DATA_CHILD(1)->grammar_symbol == GS_LP) {
            // Exp -> ID LP Args RP
            DISPATCH(ExpCall);
        } else if (DATA_CHILD(1)->grammar_symbol == GS_LB) {
            // Exp -> Exp LB Exp RB
            DISPATCH(ExpIndex);
        } else {
            PANIC("unexpected grammar symbol");
        }
    } else {
        PANIC("unexpected number of children");
    }
}

// Args -> Exp COMMA Args
// Args -> Exp
VISITOR(Args) {
    INFO;
    SAVE_SYMTAB();

    VISIT_CHILD(Exp, 0);

    usize call_type_idx = info->inherit_type_idx;
    usize param_type_idx = DATA_CHILD(0)->type_idx;

    CALL(TypeManager, *self->type_manager, add_fun_ret_par, /, call_type_idx,
         param_type_idx);

    if (NUM_CHILDREN() == 3) {
        // Args -> Exp COMMA Args
        VISIT_WITH(Args, 2, info);
    } else {
        // Args -> Exp
        // do nothing
    }

    SAVE_TYPE_BASIC(void);
}

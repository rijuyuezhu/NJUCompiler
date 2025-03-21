#include "semantic.h"
#include "ast.h"
#include "grammar_symbol.h"
#include "str.h"
#include "symbol.h"
#include "type.h"
#include "utils.h"

void MTD(SemResolver, init, /, TypeManager *type_manager,
         SymbolManager *symbol_manager) {
    self->type_manager = type_manager;
    self->symbol_manager = symbol_manager;
    self->current_symtab_idx = self->symbol_manager->root_idx;
    self->sem_error = false;
}

#define DEF_VISITOR(gs)                                                        \
    FUNC_STATIC void MTD(SemResolver, CONCATENATE(visit_, gs), /,              \
                         AstNode * node, ATTR_UNUSED SemPassInfo * info)

#define DISPATCH_TO(gs)                                                        \
    ({ CALL(SemResolver, *self, CONCATENATE(visit_, gs), /, node, info); })

// Declare Visitor functions;
// e.g. void MTD(SemResolver, visit_SEMI, /, AstNode * node); etc
#define DECL_SEMRESOLVER_GRAMMAR_SYMBOL_AID(gs) DEF_VISITOR(gs);
APPLY_GRAMMAR_SYMBOL_SYNTAX(DECL_SEMRESOLVER_GRAMMAR_SYMBOL_AID);

#define UNDEF ((usize) - 1)
#define INFO ASSERT(info, "info must not be NULL")
#define NO_INFO ASSERT(!info, "info must be NULL")
#define NEWNXTINFO(...)                                                        \
    SemPassInfo nxtinfo = CREOBJ(SemPassInfo, /, ##__VA_ARGS__)
#define NXTINFO(...) nxtinfo = CREOBJ(SemPassInfo, /, ##__VA_ARGS__)

#define NUM_CHILDREN() (node->children.size)
#define DATA_CHILD(idx)                                                        \
    ({                                                                         \
        ASSERT(idx < NUM_CHILDREN(), "index out of range");                    \
        (AstNode *)node->children.data[idx];                                   \
    })
#define VISIT_CHILD(gs, idx)                                                   \
    ({                                                                         \
        ASSERT(idx < NUM_CHILDREN(), "index out of range");                    \
        ASSERT(DATA_CHILD(idx) && DATA_CHILD(idx)->grammar_symbol == GS_##gs,  \
               "unexpected grammar symbol");                                   \
        CALL(SemResolver, *self, CONCATENATE(visit_, gs), /, DATA_CHILD(idx),  \
             NULL);                                                            \
    })
#define VISIT_WITH(gs, idx, info)                                              \
    ({                                                                         \
        ASSERT(idx < NUM_CHILDREN(), "index out of range");                    \
        ASSERT(DATA_CHILD(idx) && DATA_CHILD(idx)->grammar_symbol == GS_##gs,  \
               "unexpected grammar symbol");                                   \
        CALL(SemResolver, *self, CONCATENATE(visit_, gs), /, DATA_CHILD(idx),  \
             info);                                                            \
    })

#define SAVE_SYMTAB() ({ node->symtab_idx = self->current_symtab_idx; })
#define SAVE_TYPE_BASIC(basic_type)                                            \
    ({                                                                         \
        node->type_idx =                                                       \
            self->type_manager->CONCATENATE(basic_type, _type_idx);            \
    })
#define SAVE_TYPE(type) ({ node->type_idx = type; })
#define SAVE_TYPE_CHILD(idx) SAVE_TYPE(DATA_CHILD(idx)->type_idx)

#define GET_SYMTAB()                                                           \
    SymbolTable *now_symtab = CALL(SymbolManager, *self->symbol_manager,       \
                                   get_table, /, self->current_symtab_idx)
#define CHNGPUSH_SYMTAB(idx)                                                   \
    usize origin_symtab_idx = self->current_symtab_idx;                        \
    self->current_symtab_idx = idx

#define PUSH_SYMTAB()                                                          \
    usize origin_symtab_idx = self->current_symtab_idx;                        \
    self->current_symtab_idx = CALL(SymbolManager, *self->symbol_manager,      \
                                    add_table, /, self->current_symtab_idx)
#define POP_SYMTAB() self->current_symtab_idx = origin_symtab_idx

void MTD(SemResolver, resolve, /, AstNode *node) {
    CALL(SemResolver, *self, visit_Program, /, node, NULL);
}

void MTD(SemResolver, insert_var_dec, /, String *symbol_name, usize type_idx,
         AstNode *node, SemPassInfo *info) {
    // NOTE: key here is a hack! Do not drop it
    HString key = NSCALL(HString, from_inner, /, *symbol_name);
    GET_SYMTAB();

    // do not recursively find: we only need to check the current scope
    MapSymbolTableIterator it = CALL(SymbolTable, *now_symtab, find, /, &key);
    if (it == NULL) {
        // ok to insert it in.
        HString key_to_insert = CALL(HString, key, clone, /);
        MapSymbolTableInsertResult result =
            CALL(SymbolTable, *now_symtab, insert, /, key_to_insert,
                 SymbolEntryVar, type_idx);
        ASSERT(result.inserted, "insertion shall be successful");
    } else {
        if (info->is_in_struct) {
            report_semerr_fmt(node->line_no, SemErrorStructDefWrong,
                              "redefinition of field \"%s\"",
                              symbol_name->data);
        } else {
            report_semerr_fmt(node->line_no, SemErrorVarRedef,
                              "redefinition of variable \"%s\"",
                              symbol_name->data);
        }
    }
}

// clang-format off
// Program         NOINFO | void
// ExtDefList      NOINFO | void
// ExtDef          NOINFO | void
// ExtDecList      {give: prefix_type_idx}
//                     - prefix_type_idx: the type of the declaration
// Specifier       NOINFO | [int, float, (Sturct type from StructSpecifier)]
// StructSpecifier NOINFO | (Struct type)
// OptTag          {expect: gen_symbol_str @ nullable} | void
//                     - gen_symbol_str: the name of the struct
// Tag             {expect: gen_symbol_str} | void
//                     - gen_symbol_str: the name of the struct
// VarDec          {give: prefix_type_idx, expect: gen_symbol_str} | syn_type
//                     - prefix_type_idx: the type from suffix (or the base type)
//                     - gen_symbol_str: the name of the var
//                     - syn_type: type formed from now and the suffix
//                     sym_gen is due to the father
// FunDec          {give: prefix_type_idx, is_fun_dec; expect: gen_symtab_idx} | func_type
//                     - prefix_type_idx: the return type of the function
//                     - is_fun_dec: whether it is a function declaration
//                     - gen_symtab_idx: the new symbol table index for the func
//                     - func_type: the complete type of the function
// VarList         {give: prefix_type_idx} | void
//                     - prefix_type_idx: the belonging function type
// ParamDec        {give: prefix_type_idx} | void
//                     - prefix_type_idx: the belonging function type
// CompSt          {give: prefix_type_idx} | void
//                     - prefix_type_idx: the return type of the function
// StmtList        {give: prefix_type_idx} | void
//                     - prefix_type_idx: the return type of the function
// Stmt            {give: prefix_type_idx} | void
//                     - prefix_type_idx: the return type of the function
// DefList         {give: enclosing_struct_type_idx, is_in_struct} | void
//                     - enclosing_struct_type_idx: the struct type of the enclosing struct (if is_in_struct == true)
//                     - is_in_struct: whether the DefList is in a struct
// Def             {give: enclosing_struct_type_idx, is_in_struct} | void
//                     - enclosing_struct_type_idx: the struct type of the enclosing struct (if is_in_struct == true)
//                     - is_in_struct: whether the Def is in a struct
// DecList         {give: prefix_type_idx, enclosing_struct_type_idx, is_in_struct} | void
//                     - prefix_type_idx: the type of the declaration (local)
//                     - enclosing_struct_type_idx: the struct type of the enclosing struct (if is_in_struct == true)
//                     - is_in_struct: whether the DecList is in a struct
// Dec             {give: prefix_type_idx, enclosing_struct_type_idx, is_in_struct} | void
//                     - prefix_type_idx: the type of the declaration (local)
//                     - enclosing_struct_type_idx: the struct type of the enclosing struct (if is_in_struct == true)
//                     - is_in_struct: whether the Dec is in a struct
// Exp             NOINFO | (type of the expresion)
// Args            {give: prefix_type_idx} | void
//                     - prefix_type_idx: the type that is being determined (func call type)
// clang-format on

// Program -> ExtDefList
DEF_VISITOR(Program) {
    NO_INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(ExtDefList, 0);
    SAVE_TYPE_BASIC(void);
}

// ExtDefList -> ExtDef ExtDefList
// ExtDefList -> \epsilon
DEF_VISITOR(ExtDefList) {
    NO_INFO;
    SAVE_SYMTAB();
    if (NUM_CHILDREN() == 2) {
        // ExtDefList -> ExtDef ExtDefList
        VISIT_CHILD(ExtDef, 0);
        VISIT_CHILD(ExtDefList, 1);
    } // else ExtDefList -> \epsilon; no more actions
    SAVE_TYPE_BASIC(void);
}

// ExtDef -> Specifier ExtDecList SEMI
DEF_VISITOR(ExtDefCase1) {
    NO_INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Specifier, 0);

    usize decl_type_idx = DATA_CHILD(0)->type_idx;
    NEWNXTINFO(decl_type_idx, UNDEF, false, false);
    VISIT_WITH(ExtDecList, 1, &nxtinfo);
    SAVE_TYPE_BASIC(void);
}

// ExtDef -> Specifier SEMI
DEF_VISITOR(ExtDefCase2) {
    NO_INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Specifier, 0);
    SAVE_TYPE_BASIC(void);
}

// ExtDef -> Specifier FunDec CompSt
DEF_VISITOR(ExtDefCase3) {
    NO_INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Specifier, 0);

    usize ret_type_idx = DATA_CHILD(0)->type_idx;
    NEWNXTINFO(ret_type_idx, UNDEF, false, false);
    VISIT_WITH(FunDec, 1, &nxtinfo);

    usize func_symtab_idx = nxtinfo.gen_symtab_idx;

    NXTINFO(ret_type_idx, UNDEF, false, false);
    CHNGPUSH_SYMTAB(func_symtab_idx);
    VISIT_WITH(CompSt, 2, &nxtinfo);
    POP_SYMTAB();
    SAVE_TYPE_BASIC(void);
}

// ExtDef -> Specifier FunDec SEMI
DEF_VISITOR(ExtDefCase4) {
    NO_INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Specifier, 0);

    usize ret_type_idx = DATA_CHILD(0)->type_idx;
    NEWNXTINFO(ret_type_idx, UNDEF, true, false);
    VISIT_WITH(FunDec, 1, &nxtinfo);
    SAVE_TYPE_BASIC(void);
}

// ExtDef Dispatcher
// - ExtDef -> Specifier ExtDecList SEMI
// - ExtDef -> Specifier SEMI
// - ExtDef -> Specifier FunDec CompSt
// - ExtDef -> Specifier FunDec SEMI
DEF_VISITOR(ExtDef) {
    if (NUM_CHILDREN() == 2) {
        // ExtDef -> Specifier SEMI
        DISPATCH_TO(ExtDefCase2);
    } else if (DATA_CHILD(1)->grammar_symbol == GS_ExtDecList) {
        // ExtDef -> Specifier ExtDecList SEMI
        DISPATCH_TO(ExtDefCase1);
    } else if (DATA_CHILD(2)->grammar_symbol == GS_CompSt) {
        // ExtDef -> Specifier FunDec CompSt
        DISPATCH_TO(ExtDefCase3);
    } else {
        // ExtDef -> Specifier FunDec SEMI
        DISPATCH_TO(ExtDefCase4);
    }
}

// ExtDecList -> VarDec COMMA ExtDecList
// ExtDecList -> VarDec
DEF_VISITOR(ExtDecList) {
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
    } // else, ExtDecList -> VarDec; no more actions
    SAVE_TYPE_BASIC(void);
}

// Specifier -> TYPE
DEF_VISITOR(SpecifierCase1) {
    NO_INFO;
    SAVE_SYMTAB();
    const char *type_str = DATA_CHILD(0)->str_val.data;
    if (type_str[0] == 'i') {
        SAVE_TYPE_BASIC(int);
    } else {
        SAVE_TYPE_BASIC(float);
    }
}

// Specifier -> StructSpecifier
DEF_VISITOR(SpecifierCase2) {
    NO_INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(StructSpecifier, 0);
    SAVE_TYPE_CHILD(0);
}

// Specifier Dispatcher
// - Specifier -> TYPE
// - Specifier -> StructSpecifier
DEF_VISITOR(Specifier) {
    if (DATA_CHILD(0)->grammar_symbol == GS_TYPE) {
        // Specifier -> TYPE
        DISPATCH_TO(SpecifierCase1);
    } else {
        // Specifier -> StructSpecifier
        DISPATCH_TO(SpecifierCase2);
    }
}

// StructSpecifier -> STRUCT OptTag LC DefList RC
DEF_VISITOR(StructSpecifierCase1) {
    NO_INFO;
    SAVE_SYMTAB();
    NEWNXTINFO(UNDEF, UNDEF, false, false);
    VISIT_WITH(OptTag, 1, &nxtinfo);

    // construct the struct type
    usize struct_symtab_idx = CALL(SymbolManager, *self->symbol_manager,
                                   add_table, /, self->current_symtab_idx);
    usize struct_type_idx = CALL(TypeManager, *self->type_manager, make_struct,
                                 /, struct_symtab_idx);

    // try to fill in the symtab
    String *struct_tag = nxtinfo.gen_symbol_str;
    if (struct_tag) {
        // NOTE: key here is a hack! Do not drop it
        HString key = NSCALL(HString, from_inner, /, *struct_tag);

        // check if the struct is already defined
        GET_SYMTAB();
        MapSymbolTableIterator it =
            CALL(SymbolTable, *now_symtab, find_recursive, /, &key,
                 self->symbol_manager);
        if (it == NULL) {
            // ok to insert it in.
            HString key_to_insert = CALL(HString, key, clone, /);
            MapSymbolTableInsertResult result =
                CALL(SymbolTable, *now_symtab, insert, /, key_to_insert,
                     SymbolEntryStruct, struct_type_idx);
            ASSERT(result.inserted, "insertion shall be successful");
        } else {
            report_semerr_fmt(node->line_no, SemErrorStructRedef,
                              "redefinition of struct \"%s\"",
                              struct_tag->data);
        }
    }

    NXTINFO(UNDEF, struct_type_idx, false, true);

    // struct body requires a new symtab
    CHNGPUSH_SYMTAB(struct_symtab_idx);
    VISIT_WITH(DefList, 3, &nxtinfo);
    POP_SYMTAB();

    SAVE_TYPE(struct_type_idx);
}

// StructSpecifier -> STRUCT Tag
DEF_VISITOR(StructSpecifierCase2) {
    NO_INFO;
    SAVE_SYMTAB();
    NEWNXTINFO(UNDEF, UNDEF, false, false);
    VISIT_WITH(Tag, 1, &nxtinfo);
    String *struct_tag = nxtinfo.gen_symbol_str;
    ASSERT(struct_tag, "struct tag must not be NULL");

    // NOTE: key here is a hack! Do not drop it
    HString key = NSCALL(HString, from_inner, /, *struct_tag);

    // check if the struct is already defined
    GET_SYMTAB();
    MapSymbolTableIterator it = CALL(SymbolTable, *now_symtab, find_recursive,
                                     /, &key, self->symbol_manager);
    if (it == NULL) {
        report_semerr_fmt(node->line_no, SemErrorStructUndef,
                          "undefined struct \"%s\"", struct_tag->data);
        SAVE_TYPE_BASIC(void);
    } else {
        SAVE_TYPE(it->value.type_idx);
    }
}

// StructSpecifier Dispatcher
// - StructSpecifier -> STRUCT OptTag LC DefList RC
// - StructSpecifier -> STRUCT Tag
DEF_VISITOR(StructSpecifier) {
    if (NUM_CHILDREN() == 5) {
        // StructSpecifier -> STRUCT OptTag LC DefList RC
        DISPATCH_TO(StructSpecifierCase1);
    } else {
        // StructSpecifier -> STRUCT Tag
        DISPATCH_TO(StructSpecifierCase2);
    }
}

// OptTag -> ID
// OptTag -> \epsilon
DEF_VISITOR(OptTag) {
    INFO;
    SAVE_SYMTAB();
    if (DATA_CHILD(0)->grammar_symbol == GS_ID) {
        // OptTag -> ID
        info->gen_symbol_str = &DATA_CHILD(0)->str_val;
    } else {
        // OptTag -> \epsilon
        info->gen_symbol_str = NULL;
    }
    SAVE_TYPE_BASIC(void);
}

// Tag -> ID
DEF_VISITOR(Tag) {
    INFO;
    SAVE_SYMTAB();
    info->gen_symbol_str = &DATA_CHILD(0)->str_val;
    SAVE_TYPE_BASIC(void);
}

// VarDec -> ID
DEF_VISITOR(VarDecCase1) {
    INFO;
    SAVE_SYMTAB();
    info->gen_symbol_str = &DATA_CHILD(0)->str_val;
    usize upper_type_idx = info->prefix_type_idx;
    SAVE_TYPE(upper_type_idx);
}

// VarDec -> VarDec LB INT RB
DEF_VISITOR(VarDecCase2) {
    INFO;
    SAVE_SYMTAB();

    usize size = DATA_CHILD(2)->int_val;
    usize upper_type_idx = info->prefix_type_idx;
    usize now_type_idx = CALL(TypeManager, *self->type_manager, make_array, /,
                              size, upper_type_idx);
    NEWNXTINFO(now_type_idx, UNDEF, false, false);
    VISIT_WITH(VarDec, 0, &nxtinfo);
    info->gen_symbol_str = nxtinfo.gen_symbol_str;
    SAVE_TYPE_CHILD(0);
}

// VarDec Dispatcher
// - VarDec -> ID
// - VarDec -> VarDec LB INT RB
DEF_VISITOR(VarDec) {
    if (NUM_CHILDREN() == 1) {
        // VarDec -> ID
        DISPATCH_TO(VarDecCase1);
    } else {
        // VarDec -> VarDec LB INT RB
        DISPATCH_TO(VarDecCase2);
    }
}

// FunDec -> ID LP VarList RP
// FunDec -> ID LP RP
DEF_VISITOR(FunDec) {
    INFO;
    SAVE_SYMTAB();
    usize ret_type_idx = info->prefix_type_idx;
    bool is_fun_dec = info->is_fun_dec;

    // create the function type
    usize func_type_idx = CALL(TypeManager, *self->type_manager, make_fun, /);
    CALL(TypeManager, *self->type_manager, add_fun_ret_par, /, func_type_idx,
         ret_type_idx);

    // 1. Handle ID

    String *fun_name = &DATA_CHILD(0)->str_val;

    // NOTE: key here is a hack! Do not drop it
    HString key = NSCALL(HString, from_inner, /, *fun_name);

    GET_SYMTAB();
    MapSymbolTableIterator it = CALL(SymbolTable, *now_symtab, find_recursive,
                                     /, &key, self->symbol_manager);
    if (it == NULL) {
        // ok to insert it in.
        HString key_to_insert = CALL(HString, key, clone, /);
        MapSymbolTableInsertResult result =
            CALL(SymbolTable, *now_symtab, insert, /, key_to_insert,
                 SymbolEntryFun, func_type_idx);
        ASSERT(result.inserted, "insertion shall be successful");

        SymbolEntry *entry = &result.node->value;

        if (is_fun_dec) {
            // First time dec
            entry->as_fun.is_defined = false;
        } else {
            // First time def
            entry->as_fun.is_defined = true;
        }
    } else {
        SymbolEntry *entry = &it->value;
        if (!info->is_fun_dec && entry->as_fun.is_defined) {
            report_semerr_fmt(node->line_no, SemErrorFunRedef,
                              "redefinition of function \"%s\"",
                              fun_name->data);
        } else if (!info->is_fun_dec && !entry->as_fun.is_defined) {
            entry->as_fun.is_defined = true;
        }
    }

    PUSH_SYMTAB();
    info->gen_symtab_idx = self->current_symtab_idx;

    // 2. Handle VarList
    if (NUM_CHILDREN() == 4) {
        // FunDec -> ID LP VarList RP
        NEWNXTINFO(func_type_idx, UNDEF, false, false);
        VISIT_WITH(VarList, 2, &nxtinfo);
    }
    POP_SYMTAB();

    if (it) {
        // check the consistency of the function declaration
        if (!CALL(TypeManager, *self->type_manager, check_type_consistency, /,
                  func_type_idx, it->value.type_idx)) {
            report_semerr_fmt(node->line_no, SemErrorFunDecConflict,
                              "conflicting types for function \"%s\"",
                              fun_name->data);
        }
    }

    SAVE_TYPE(func_type_idx);
}

// VarList -> ParamDec COMMA VarList
// VarList -> ParamDec
DEF_VISITOR(VarList) {
    INFO;
    SAVE_SYMTAB();
    VISIT_WITH(ParamDec, 0, info);
    if (NUM_CHILDREN() == 3) {
        // VarList -> ParamDec COMMA VarList
        VISIT_WITH(VarList, 2, info);
    }
    SAVE_TYPE_BASIC(void);
}

// ParamDec -> Specifier VarDec
DEF_VISITOR(ParamDec) {
    INFO;
    SAVE_SYMTAB();

    VISIT_CHILD(Specifier, 0);
    usize fun_type_idx = info->prefix_type_idx;

    usize base_type_idx = DATA_CHILD(0)->type_idx;
    NEWNXTINFO(base_type_idx, UNDEF, false, false);
    VISIT_WITH(VarDec, 1, &nxtinfo);
    String *symbol_name = nxtinfo.gen_symbol_str;
    ASSERT(symbol_name);
    usize param_type_idx = DATA_CHILD(1)->type_idx;
    CALL(SemResolver, *self, insert_var_dec, /, symbol_name, param_type_idx,
         node, info);

    CALL(TypeManager, *self->type_manager, add_fun_ret_par, /, fun_type_idx,
         param_type_idx);
    SAVE_TYPE_BASIC(void);
}

// CompSt -> LC DefList StmtList RC
DEF_VISITOR(CompSt) {
    INFO;
    NEWNXTINFO(UNDEF, UNDEF, false, false);
    VISIT_WITH(DefList, 1, &nxtinfo);
    VISIT_WITH(StmtList, 2, info);
    SAVE_TYPE_BASIC(void);
}

// StmtList -> Stmt StmtList
// StmtList -> \epsilon
DEF_VISITOR(StmtList) {
    INFO;
    SAVE_SYMTAB();
    if (NUM_CHILDREN() == 2) {
        // StmtList -> Stmt StmtList
        VISIT_WITH(Stmt, 0, info);
        VISIT_WITH(StmtList, 1, info);
    } // else StmtList -> \epsilon; no more actions
    SAVE_TYPE_BASIC(void);
}

// Stmt -> Exp SEMI
DEF_VISITOR(StmtCase1) {
    INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 0);
    SAVE_TYPE_BASIC(void);
}

// Stmt -> CompSt
DEF_VISITOR(StmtCase2) {
    INFO;
    SAVE_SYMTAB();
    PUSH_SYMTAB();
    VISIT_WITH(CompSt, 0, info);
    POP_SYMTAB();
    SAVE_TYPE_BASIC(void);
}

// Stmt -> RETURN Exp SEMI
DEF_VISITOR(StmtCase3) {
    INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 1);
    usize ret_type_idx = info->prefix_type_idx;
    if (DATA_CHILD(1)->type_idx != ret_type_idx) {
        report_semerr(node->line_no, SemErrorReturnTypeErr,
                      "return type mismatched");
    }
    SAVE_TYPE_BASIC(void);
}

// Stmt -> IF LP Exp RP Stmt
DEF_VISITOR(StmtCase4) {
    INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 2);
    if (DATA_CHILD(2)->type_idx != self->type_manager->int_type_idx) {
        report_semerr(node->line_no, SemErrorOPTypeErr,
                      "if condition must be of type int");
    }
    VISIT_WITH(Stmt, 4, info);
    SAVE_TYPE_BASIC(void);
}

// Stmt -> IF LP Exp RP Stmt ELSE Stmt
DEF_VISITOR(StmtCase5) {
    INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 2);
    if (DATA_CHILD(2)->type_idx != self->type_manager->int_type_idx) {
        report_semerr(node->line_no, SemErrorOPTypeErr,
                      "if condition must be of type int");
    }
    VISIT_WITH(Stmt, 4, info);
    VISIT_WITH(Stmt, 6, info);
    SAVE_TYPE_BASIC(void);
}

// Stmt -> WHILE LP Exp RP Stmt
DEF_VISITOR(StmtCase6) {
    INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 2);
    if (DATA_CHILD(2)->type_idx != self->type_manager->int_type_idx) {
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
DEF_VISITOR(Stmt) {
    if (NUM_CHILDREN() == 2) {
        // Stmt -> Exp SEMI
        DISPATCH_TO(StmtCase1);
    } else if (NUM_CHILDREN() == 1) {
        // Stmt -> CompSt
        DISPATCH_TO(StmtCase2);
    } else if (NUM_CHILDREN() == 3) {
        // Stmt -> RETURN Exp SEMI
        DISPATCH_TO(StmtCase3);
    } else if (NUM_CHILDREN() == 7) {
        // Stmt -> IF LP Exp RP Stmt ELSE Stmt
        DISPATCH_TO(StmtCase5);
    } else if (DATA_CHILD(0)->grammar_symbol == GS_IF) {
        // Stmt -> IF LP Exp RP Stmt
        DISPATCH_TO(StmtCase4);
    } else {
        // Stmt -> WHILE LP Exp RP Stmt
        DISPATCH_TO(StmtCase6);
    }
}

// DefList -> Def DefList
// DefList -> \epsilon
DEF_VISITOR(DefList) {
    INFO;
    SAVE_SYMTAB();
    if (NUM_CHILDREN() == 2) {
        // DefList -> Def DefList
        VISIT_WITH(Def, 0, info);
        VISIT_WITH(DefList, 1, info);
    } // else DefList -> \epsilon; no more actions
    SAVE_TYPE_BASIC(void);
}

// Def -> Specifier DecList SEMI
DEF_VISITOR(Def) {
    INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Specifier, 0);
    usize decl_type_idx = DATA_CHILD(0)->type_idx;
    NEWNXTINFO(decl_type_idx, info->enclosing_struct_type_idx, false,
               info->is_in_struct);
    VISIT_WITH(DecList, 1, &nxtinfo);
    SAVE_TYPE_BASIC(void);
}

// DecList -> Dec
// DecList -> Dec COMMA DecList
DEF_VISITOR(DecList) {
    INFO;
    SAVE_SYMTAB();
    VISIT_WITH(Dec, 0, info);
    if (NUM_CHILDREN() == 3) {
        // DecList -> Dec COMMA DecList
        VISIT_WITH(DecList, 2, info);
    } // else DecList -> Dec; no more actions
    SAVE_TYPE_BASIC(void);
}

// Dec -> VarDec
DEF_VISITOR(DecCase1) {
    INFO;
    SAVE_SYMTAB();
    usize decl_type_idx = info->prefix_type_idx;
    NEWNXTINFO(decl_type_idx, UNDEF, false, false);

    VISIT_WITH(VarDec, 0, &nxtinfo);
    String *symbol_name = nxtinfo.gen_symbol_str;
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
DEF_VISITOR(DecCase2) {
    INFO;
    SAVE_SYMTAB();
    if (info->is_in_struct) {
        report_semerr(node->line_no, SemErrorStructDefWrong,
                      "initialization in struct definition");
    }
    usize decl_type_idx = info->prefix_type_idx;
    NEWNXTINFO(decl_type_idx, UNDEF, false, false);

    VISIT_WITH(VarDec, 0, &nxtinfo);
    String *symbol_name = nxtinfo.gen_symbol_str;
    ASSERT(symbol_name);
    usize var_type_idx = DATA_CHILD(0)->type_idx;
    CALL(SemResolver, *self, insert_var_dec, /, symbol_name, var_type_idx, node,
         info);
    if (info->is_in_struct) {
        CALL(TypeManager, *self->type_manager, add_struct_field, /,
             info->enclosing_struct_type_idx, var_type_idx);
    }

    VISIT_CHILD(Exp, 2);
    usize exp_type_idx = DATA_CHILD(2)->type_idx;
    if (!CALL(TypeManager, *self->type_manager, check_type_consistency, /,
              var_type_idx, exp_type_idx)) {
        report_semerr(node->line_no, SemErrorAssignTypeErr,
                      "assignment type mismatched");
    }
}

// Dec Dispatcher
// - Dec -> VarDec
// - Dec -> VarDec ASSIGNOP Exp
DEF_VISITOR(Dec) {
    if (NUM_CHILDREN() == 1) {
        // Dec -> VarDec
        DISPATCH_TO(DecCase1);
    } else {
        // Dec -> VarDec ASSIGNOP Exp
        DISPATCH_TO(DecCase2);
    }
}

FUNC_STATIC bool MTD(SemResolver, is_lvalue, /, AstNode *node) {
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

// Exp -> Exp ASSIGNOP Exp
DEF_VISITOR(ExpASSIGNOP) {
    NO_INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 0);
    VISIT_CHILD(Exp, 2);
    usize left_type_idx = DATA_CHILD(0)->type_idx;
    usize right_type_idx = DATA_CHILD(2)->type_idx;
    if (!CALL(TypeManager, *self->type_manager, check_type_consistency, /,
              left_type_idx, right_type_idx)) {
        report_semerr(node->line_no, SemErrorAssignTypeErr,
                      "assignment type mismatched");
    }
    // check if left_type_idx is lvalue
    bool is_lvalue = CALL(SemResolver, *self, is_lvalue, /, DATA_CHILD(0));
    if (!is_lvalue) {
        report_semerr(node->line_no, SemErrorAssignToRValue,
                      "assignment to non-lvalue");
    }
    SAVE_TYPE(left_type_idx);
}

#define PREPARE_BINARY                                                         \
    NO_INFO;                                                                   \
    SAVE_SYMTAB();                                                             \
    VISIT_CHILD(Exp, 0);                                                       \
    VISIT_CHILD(Exp, 2);                                                       \
    usize left_type_idx = DATA_CHILD(0)->type_idx;                             \
    usize right_type_idx = DATA_CHILD(2)->type_idx

#define FINISH_BINARY SAVE_TYPE(left_type_idx)

#define ARITH_CHECK                                                            \
    if (left_type_idx != self->type_manager->int_type_idx &&                   \
        left_type_idx != self->type_manager->float_type_idx) {                 \
        report_semerr(                                                         \
            node->line_no, SemErrorOPTypeErr,                                  \
            "arithmetic operator requires the left operand of type int "       \
            "or float");                                                       \
    } else if (right_type_idx != self->type_manager->int_type_idx &&           \
               right_type_idx != self->type_manager->float_type_idx) {         \
        report_semerr(                                                         \
            node->line_no, SemErrorOPTypeErr,                                  \
            "arithmetic operator requires the right operand of type int "      \
            "or float");                                                       \
    } else if (left_type_idx != right_type_idx) {                              \
        report_semerr(node->line_no, SemErrorOPTypeErr,                        \
                      "arithmetic operator requires two operands of the same " \
                      "type");                                                 \
    }

// Exp -> Exp AND Exp
DEF_VISITOR(ExpAND) {
    PREPARE_BINARY;
    if (left_type_idx != self->type_manager->int_type_idx ||
        right_type_idx != self->type_manager->int_type_idx) {
        report_semerr(node->line_no, SemErrorOPTypeErr,
                      "AND operator requires two operands of type int");
    }
    FINISH_BINARY;
}

// Exp -> Exp OR Exp
DEF_VISITOR(ExpOR) {
    PREPARE_BINARY;
    if (left_type_idx != self->type_manager->int_type_idx ||
        right_type_idx != self->type_manager->int_type_idx) {
        report_semerr(node->line_no, SemErrorOPTypeErr,
                      "OR operator requires two operands of type int");
    }
    FINISH_BINARY;
}

// Exp -> Exp RELOP Exp
DEF_VISITOR(ExpRELOP) {
    PREPARE_BINARY;
    ARITH_CHECK;
    FINISH_BINARY;
}

// Exp -> Exp PLUS Exp
DEF_VISITOR(ExpPLUS) {
    PREPARE_BINARY;
    ARITH_CHECK;
    FINISH_BINARY;
}

// Exp -> Exp MINUS Exp
DEF_VISITOR(ExpMINUS) {
    PREPARE_BINARY;
    ARITH_CHECK;
    FINISH_BINARY;
}

// Exp -> Exp STAR Exp
DEF_VISITOR(ExpSTAR) {
    PREPARE_BINARY;
    ARITH_CHECK;
    FINISH_BINARY;
}

// Exp -> Exp DIV Exp
DEF_VISITOR(ExpDIV) {
    PREPARE_BINARY;
    ARITH_CHECK;
    FINISH_BINARY;
}

// Exp -> LP Exp RP
DEF_VISITOR(ExpParen) {
    NO_INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 1);
    SAVE_TYPE_CHILD(1);
}

// Exp -> MINUS Exp
DEF_VISITOR(ExpUMINUS) {
    NO_INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 1);
    usize type_idx = DATA_CHILD(1)->type_idx;
    if (type_idx != self->type_manager->int_type_idx &&
        type_idx != self->type_manager->float_type_idx) {
        report_semerr(node->line_no, SemErrorOPTypeErr,
                      "unary minus operator requires operand of type int or "
                      "float");
    }
    SAVE_TYPE(type_idx);
}

// Exp -> NOT Exp
DEF_VISITOR(ExpNOT) {
    NO_INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 1);
    usize type_idx = DATA_CHILD(1)->type_idx;
    if (type_idx != self->type_manager->int_type_idx) {
        report_semerr(node->line_no, SemErrorOPTypeErr,
                      "NOT operator requires operand of type int");
    }
    SAVE_TYPE(type_idx);
}

// Exp -> ID LP Args RP
// Exp -> ID LP RP
DEF_VISITOR(ExpCall) {
    NO_INFO;
    SAVE_SYMTAB();

    usize call_type_idx = CALL(TypeManager, *self->type_manager, make_fun, /);
    // add a dummy return type
    CALL(TypeManager, *self->type_manager, add_fun_ret_par, /, call_type_idx,
         self->type_manager->void_type_idx);

    if (NUM_CHILDREN() == 4) {
        // Exp -> ID LP Args RP
        NEWNXTINFO(call_type_idx, UNDEF, false, false);
        VISIT_WITH(Args, 2, &nxtinfo);
    } // else, Exp -> ID LP RP; no more actions

    String *name = &DATA_CHILD(0)->str_val;
    ASSERT(name);
    // NOTE: key here is a hack! Do not drop it
    HString key = NSCALL(HString, from_inner, /, *name);
    GET_SYMTAB();
    MapSymbolTableIterator it = CALL(SymbolTable, *now_symtab, find_recursive,
                                     /, &key, self->symbol_manager);
    if (it == NULL) {
        report_semerr_fmt(node->line_no, SemErrorFunUndef,
                          "undefined function \"%s\"", STRING_C_STR(*name));
    } else if (it->value.kind != SymbolEntryFun) {
        report_semerr_fmt(node->line_no, SemErrorCallBaseTypeErr,
                          "\"%s\" is not a function", STRING_C_STR(*name));
    } else if (!CALL(TypeManager, *self->type_manager,
                     check_type_consistency_with_fun_fix, /, call_type_idx,
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
    SAVE_TYPE(ret_type_idx);
}

// Exp -> Exp LB Exp RB
DEF_VISITOR(ExpIndex) {
    NO_INFO;
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
        if (idx_type_idx != self->type_manager->int_type_idx) {
            report_semerr(
                node->line_no, SemErrorIndexNonInt,
                "index operator requires the index to be of type int");
        }
        usize ret_type_id = base_type->as_array.subtype_idx;
        SAVE_TYPE(ret_type_id);
    }
}

// Exp -> Exp DOT ID
DEF_VISITOR(ExpField) {
    NO_INFO;
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
        // NOTE: key here is a hack! Do not drop it
        HString key = NSCALL(HString, from_inner, /, *field_name);
        SymbolTable *struct_symtab =
            CALL(SymbolManager, *self->symbol_manager, get_table, /,
                 base_type->as_struct.symtab_idx);
        MapSymbolTableIterator it =
            CALL(SymbolTable, *struct_symtab, find, /, &key);
        if (it == NULL) {
            report_semerr_fmt(node->line_no, SemErrorFieldNotExist,
                              "field \"%s\" does not exist in struct",
                              STRING_C_STR(*field_name));
        }
        usize field_type_idx = it->value.type_idx;
        SAVE_TYPE(field_type_idx);
    }
}

// Exp -> ID
DEF_VISITOR(ExpID) {
    NO_INFO;
    SAVE_SYMTAB();
    String *name = &DATA_CHILD(0)->str_val;
    ASSERT(name);
    // NOTE: key here is a hack! Do not drop it
    HString key = NSCALL(HString, from_inner, /, *name);
    GET_SYMTAB();
    MapSymbolTableIterator it = CALL(SymbolTable, *now_symtab, find_recursive,
                                     /, &key, self->symbol_manager);
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
DEF_VISITOR(ExpINT) {
    NO_INFO;
    SAVE_SYMTAB();
    SAVE_TYPE_BASIC(int);
}

// Exp -> FLOAT
DEF_VISITOR(ExpFLOAT) {
    NO_INFO;
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
DEF_VISITOR(Exp) {
    if (NUM_CHILDREN() == 3) {
        switch (DATA_CHILD(1)->grammar_symbol) {
        case GS_ASSIGNOP:
            // Exp -> Exp ASSIGNOP Exp
            DISPATCH_TO(ExpASSIGNOP);
            break;
        case GS_AND:
            // Exp -> Exp AND Exp
            DISPATCH_TO(ExpAND);
            break;
        case GS_OR:
            // Exp -> Exp OR Exp
            DISPATCH_TO(ExpOR);
            break;
        case GS_RELOP:
            // Exp -> Exp RELOP Exp
            DISPATCH_TO(ExpRELOP);
            break;
        case GS_PLUS:
            // Exp -> Exp PLUS Exp
            DISPATCH_TO(ExpPLUS);
            break;
        case GS_MINUS:
            // Exp -> Exp MINUS Exp
            DISPATCH_TO(ExpMINUS);
            break;
        case GS_STAR:
            // Exp -> Exp STAR Exp
            DISPATCH_TO(ExpSTAR);
            break;
        case GS_DIV:
            // Exp -> Exp DIV Exp
            DISPATCH_TO(ExpDIV);
            break;
        case GS_Exp:
            // Exp -> LP Exp RP
            DISPATCH_TO(ExpParen);
            break;
        case GS_LP:
            // Exp -> ID LP RP
            DISPATCH_TO(ExpCall);
            break;
        case GS_DOT:
            // Exp -> Exp DOT ID
            DISPATCH_TO(ExpField);
            break;
        default:
            PANIC("unexpected grammar symbol");
        }
    } else if (NUM_CHILDREN() == 2) {
        if (DATA_CHILD(0)->grammar_symbol == GS_MINUS) {
            // Exp -> MINUS Exp
            DISPATCH_TO(ExpUMINUS);
        } else if (DATA_CHILD(0)->grammar_symbol == GS_NOT) {
            // Exp -> NOT Exp
            DISPATCH_TO(ExpNOT);
        } else {
            PANIC("unexpected grammar symbol");
        }
    } else if (NUM_CHILDREN() == 1) {
        if (DATA_CHILD(0)->grammar_symbol == GS_ID) {
            // Exp -> ID
            DISPATCH_TO(ExpID);
        } else if (DATA_CHILD(0)->grammar_symbol == GS_INT) {
            // Exp -> INT
            DISPATCH_TO(ExpINT);
        } else if (DATA_CHILD(0)->grammar_symbol == GS_FLOAT) {
            // Exp -> FLOAT
            DISPATCH_TO(ExpFLOAT);
        } else {
            PANIC("unexpected grammar symbol");
        }
    } else if (NUM_CHILDREN() == 4) {
        if (DATA_CHILD(1)->grammar_symbol == GS_LP) {
            // Exp -> ID LP Args RP
            DISPATCH_TO(ExpCall);
        } else if (DATA_CHILD(1)->grammar_symbol == GS_LB) {
            // Exp -> Exp LB Exp RB
            DISPATCH_TO(ExpIndex);
        } else {
            PANIC("unexpected grammar symbol");
        }
    } else {
        PANIC("unexpected number of children");
    }
}

// Args -> Exp COMMA Args
// Args -> Exp
DEF_VISITOR(Args) {
    INFO;
    SAVE_SYMTAB();
    VISIT_CHILD(Exp, 0);

    usize param_type_idx = DATA_CHILD(0)->type_idx;
    usize call_type_idx = info->prefix_type_idx;
    CALL(TypeManager, *self->type_manager, add_fun_ret_par, /, call_type_idx,
         param_type_idx);

    if (NUM_CHILDREN() == 3) {
        // Args -> Exp COMMA Args
        VISIT_WITH(Args, 2, info);
    } // else Args -> Exp; no more actions

    SAVE_TYPE_BASIC(void);
}

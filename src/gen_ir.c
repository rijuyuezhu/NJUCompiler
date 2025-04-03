#include <stdbool.h>

#include "ast.h"
#include "debug.h"
#include "gen_ir.h"
#include "general_vec.h"
#include "grammar_symbol.h"
#include "ir.h"
#include "op.h"
#include "str.h"
#include "symbol.h"
#include "type.h"
#include "utils.h"

void MTD(IRGenerator, init, /, IRManager *ir_manager, TypeManager *type_manager,
         SymbolManager *symbol_manager) {
    self->ir_manager = ir_manager;
    self->type_manager = type_manager;
    self->symbol_manager = symbol_manager;
    self->gen_ir_error = false;
}

DECLARE_PLAIN_VEC(VecArgs, IREntity, FUNC_STATIC);
DEFINE_PLAIN_VEC(VecArgs, IREntity, FUNC_STATIC);

typedef struct IRPassInfo {
    // inherit
    IREntity *target;
    IREntity *true_label;
    IREntity *false_label;
    VecArgs *arglist;
} IRPassInfo;

static void MTD(IRPassInfo, init, /, IREntity *target, IREntity *true_label,
                IREntity *false_label, VecArgs *arglist) {
    self->target = target;
    self->true_label = true_label;
    self->false_label = false_label;
    self->arglist = arglist;
}

// Visitor operations

#define VISITOR(gs)                                                            \
    FUNC_STATIC void MTD(IRGenerator, CONCATENATE(visit_, gs), /,              \
                         ATTR_UNUSED AstNode * node,                           \
                         ATTR_UNUSED IRPassInfo * info)

#define DISPATCH_WITH(gs, info)                                                \
    ({ CALL(IRGenerator, *self, CONCATENATE(visit_, gs), /, node, info); })

#define DISPATCH(gs) DISPATCH_WITH(gs, info)

#define DISPATCH_ENTRY(gs, id)                                                 \
    case id:                                                                   \
        DISPATCH(CONCATENATE3(gs, Case, id));                                  \
        break;

// Declare Visitor functions;
// e.g. void MTD(SemResolver, visit_SEMI, /, AstNode * node); etc
#define DECLVISITOR_IRGENERATOR_GRAMMAR_SYMBOL_AID(gs) VISITOR(gs);
APPLY_GRAMMAR_SYMBOL_SYNTAX(DECLVISITOR_IRGENERATOR_GRAMMAR_SYMBOL_AID);

VISITOR(ExpCond);

// Info management
#define INFO ASSERT(info, "info must not be NULL")
#define NOINFO ASSERT(!info, "info must be NULL")
#define NEW_SINFO(...) IRPassInfo sinfo = CREOBJ(IRPassInfo, /, ##__VA_ARGS__)
#define SINFO(...) sinfo = CREOBJ(IRPassInfo, /, ##__VA_ARGS__)

#define INFO_HAS_TARGET ASSERT(info->target, "info->target must not be NULL")

#define INFO_NO_TARGET ASSERT(!info->target, "info->target must be NULL")

#define INFO_HAS_LABEL                                                         \
    ASSERT(info->true_label && info->false_label,                              \
           "info->true_label and info->false_label must not be NULL")

#define INFO_NO_LABEL                                                          \
    ASSERT(!(info->true_label && info->false_label),                           \
           "info->true_label and info->false_label must be NULL")

// Data/flow management

#define PROD_ID() (node->production_id)
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
        CALL(IRGenerator, *self, CONCATENATE(visit_, gs), /, DATA_CHILD(idx),  \
             info);                                                            \
    })
#define VISIT_CHILD(gs, idx) VISIT_WITH(gs, idx, NULL)

#define VISIT_WITH_UNCHK(gs, idx, info)                                        \
    ({                                                                         \
        ASSERT(idx < NUM_CHILDREN(), "index out of range");                    \
        CALL(IRGenerator, *self, CONCATENATE(visit_, gs), /, DATA_CHILD(idx),  \
             info);                                                            \
    })
#define VISIT_CHILD_UNCHK(gs, idx) VISIT_WITH_UNCHK(gs, idx, NULL)

/* ------ */

FUNC_STATIC IREntityKind MTD(IRGenerator, get_var_irentity_kind, /,
                             usize type_idx) {
    Type *t = CALL(TypeManager, *self->type_manager, get_type, /, type_idx);
    IREntityKind kind;
    if (t->kind == TypeKindInt) {
        kind = IREntityVar;
    } else if (t->kind == TypeKindArray || t->kind == TypeKindStruct) {
        kind = IREntityAddr;
    } else if (t->kind == TypeKindFloat) {
        kind = IREntityVar;
        report_genir_err("float value is not permitted");
    } else {
        PANIC("t->kind not correct");
    }
    return kind;
}

FUNC_STATIC usize MTD(IRGenerator, get_var_ir_idx, /, SymbolEntry *sym) {
    ASSERT(sym->kind == SymbolEntryVar);
    if (sym->as_var.ir_var_idx == (usize)-1) {
        IREntityKind kind =
            CALL(IRGenerator, *self, get_var_irentity_kind, /, sym->type_idx);
        IREntity ent = CALL(IRManager, *self->ir_manager, gen_ent_var, /, kind);
        sym->as_var.ir_var_idx = ent.var_idx;
    }
    return sym->as_var.ir_var_idx;
}

FUNC_STATIC IREntity MTD(IRGenerator, get_var_ent, /, SymbolEntry *sym) {
    ASSERT(sym && sym->kind == SymbolEntryVar);
    usize ir_idx = CALL(IRGenerator, *self, get_var_ir_idx, /, sym);
    usize type_idx = sym->type_idx;
    IREntityKind ent_kind =
        CALL(IRGenerator, *self, get_var_irentity_kind, /, type_idx);
    return NSCALL(IREntity, make_var, /, ent_kind, ir_idx);
}

#define NEW_VAR_FROM_TYPEIDX(type_idx)                                         \
    ({                                                                         \
        IREntityKind ent_kind =                                                \
            CALL(IRGenerator, *self, get_var_irentity_kind, /, type_idx);      \
        CALL(IRManager, *self->ir_manager, gen_ent_var, /, ent_kind);          \
    })

#define NEW_VAR_FROM_CHILD(idx)                                                \
    ({                                                                         \
        usize type_idx = DATA_CHILD(idx)->type_idx;                            \
        NEW_VAR_FROM_TYPEIDX(type_idx);                                        \
    })

#define GET_VAR_FROM_SYM(sym) CALL(IRGenerator, *self, get_var_ent, /, sym)

#define GET_VAR()                                                              \
    ({                                                                         \
        SymbolEntry *sym = node->symentry_ptr;                                 \
        GET_VAR_FROM_SYM(sym);                                                 \
    })

#define GET_VAR_FROM_CHILD(idx)                                                \
    ({                                                                         \
        SymbolEntry *sym = DATA_CHILD(idx)->symentry_ptr;                      \
        GET_VAR_FROM_SYM(sym);                                                 \
    })

#define NEW_LABEL() CALL(IRManager, *self->ir_manager, gen_ent_label, /)

#define ADDIR(kind, ...)                                                       \
    CALL(IRManager, *self->ir_manager, CONCATENATE(addir_, kind), /,           \
         ##__VA_ARGS__);

String MTD(IRGenerator, gen_ir, /, AstNode *node) {
    String ret = CREOBJ(String, /);
    CALL(IRGenerator, *self, visit_Program, /, node, NULL);
    return ret;
}

// Program -> ExtDefList
VISITOR(Program) {
    NOINFO;
    VISIT_CHILD(ExtDefList, 0);
}

// 0. ExtDefList -> ExtDef ExtDefList
// 1. ExtDefList -> \epsilon
VISITOR(ExtDefList) {
    NOINFO;
    if (PROD_ID() == 0) {
        // ExtDefList -> ExtDef ExtDefList
        VISIT_CHILD(ExtDef, 0);
        VISIT_CHILD(ExtDefList, 1);
    } else {
        // ExtDefList -> \epsilon
    }
}

// ExtDef -> Specifier ExtDecList SEMI
VISITOR(ExtDefCase0) {
    NOINFO;
    // Detected global variable
    report_genir_err("detected global variables");
}

// ExtDef -> Specifier SEMI
VISITOR(ExtDefCase1) { NOINFO; }

// ExtDef -> Specifier FunDec CompSt
VISITOR(ExtDefCase2) {
    NOINFO;
    VISIT_CHILD(FunDec, 1);
    VISIT_CHILD(CompSt, 2);
}

// ExtDef -> Specifier FunDec SEMI
VISITOR(ExtDefCase3) { NOINFO; }

// ExtDef Dispatcher
// 0. ExtDef -> Specifier ExtDecList SEMI
// 1. ExtDef -> Specifier SEMI
// 2. ExtDef -> Specifier FunDec CompSt
// 3. ExtDef -> Specifier FunDec SEMI
VISITOR(ExtDef) {
    switch (PROD_ID()) {
        DISPATCH_ENTRY(ExtDef, 0);
        DISPATCH_ENTRY(ExtDef, 1);
        DISPATCH_ENTRY(ExtDef, 2);
        DISPATCH_ENTRY(ExtDef, 3);
    default:
        PANIC("unexpected production id");
    }
}

// 0. ExtDecList -> VarDec
// 1. ExtDecList -> VarDec COMMA ExtDecList
VISITOR(ExtDecList) { PANIC("Unreachable"); }

// Specifier -> TYPE
VISITOR(SpecifierCase0) { PANIC("Unreachable"); }

// Specifier -> StructSpecifier
VISITOR(SpecifierCase1) { PANIC("Unreachable"); }

// Specifier Dispatcher
// 0. Specifier -> TYPE
// 1. Specifier -> StructSpecifier
VISITOR(Specifier) {
    switch (PROD_ID()) {
        DISPATCH_ENTRY(Specifier, 0);
        DISPATCH_ENTRY(Specifier, 1);
    default:
        PANIC("unexpected production id");
    }
}

// StructSpecifier -> STRUCT OptTag LC DefList RC
VISITOR(StructSpecifierCase0) { PANIC("Unreachable"); }

// StructSpecifier -> STRUCT Tag
VISITOR(StructSpecifierCase1) { PANIC("Unreachable"); }

// StructSpecifier Dispatcher
// 0. StructSpecifier -> STRUCT OptTag LC DefList RC
// 1. StructSpecifier -> STRUCT Tag
VISITOR(StructSpecifier) {
    switch (PROD_ID()) {
        DISPATCH_ENTRY(StructSpecifier, 0);
        DISPATCH_ENTRY(StructSpecifier, 1);
    default:
        PANIC("unexpected production id");
    }
}

// 0. OptTag -> ID
// 1. OptTag -> \epsilon
VISITOR(OptTag) { PANIC("Unreachable"); }

// Tag -> ID
VISITOR(Tag) { PANIC("Unreachable"); }

// VarDec -> ID
VISITOR(VarDecCase0) { PANIC("Unreachable"); }

// VarDec -> VarDec LB INT RB
VISITOR(VarDecCase1) { PANIC("Unreachable"); }

// VarDec Dispatcher
// 0. VarDec -> ID
// 1. VarDec -> VarDec LB INT RB
VISITOR(VarDec) {
    switch (PROD_ID()) {
        DISPATCH_ENTRY(VarDec, 0);
        DISPATCH_ENTRY(VarDec, 1);
    default:
        PANIC("unexpected production id");
    }
}

// 0. FunDec -> ID LP VarList RP
// 1. FunDec -> ID LP RP
VISITOR(FunDec) {
    NOINFO;
    SymbolEntry *sym = node->symentry_ptr;
    ASSERT(sym);
    IREntity fun = NSCALL(IREntity, make_fun, /, sym->name);
    ADDIR(fun, fun);
    if (PROD_ID() == 0) {
        // FunDec -> ID LP VarList BP
        VISIT_CHILD(VarList, 2);
    } else {
        // FunDec -> ID LP BP
        // do nothing
    }
}

// 0. VarList -> ParamDec COMMA VarList
// 1. VarList -> ParamDec
VISITOR(VarList) {
    NOINFO;
    VISIT_CHILD(ParamDec, 0);
    if (PROD_ID() == 0) {
        // VarList -> ParamDec COMMA VarList
        VISIT_CHILD(VarList, 2);
    } else {
        // VarList -> ParamDec
        // do nothing
    }
}

// ParamDec -> Specifier VarDec
VISITOR(ParamDec) {
    NOINFO;
    IREntity par = GET_VAR();
    ADDIR(param, par);
}

// CompSt -> LC DefList StmtList RC
VISITOR(CompSt) {
    NOINFO;
    VISIT_CHILD(DefList, 1);
    VISIT_CHILD(StmtList, 2);
}

// 0. StmtList -> Stmt StmtList
// 1. StmtList -> \epsilon
VISITOR(StmtList) {
    NOINFO;
    if (PROD_ID() == 0) {
        // StmtList -> Stmt StmtList
        VISIT_CHILD(Stmt, 0);
        VISIT_CHILD(StmtList, 1);
    } else {
        // StmtList -> \epsilon
        // do nothing
    }
}

// Stmt -> Exp SEMI
VISITOR(StmtCase0) {
    NOINFO;
    IREntity temp_var = NEW_VAR_FROM_CHILD(0);
    NEW_SINFO(&temp_var, NULL, NULL, NULL);
    VISIT_WITH(Exp, 0, &sinfo);
}

// Stmt -> CompSt
VISITOR(StmtCase1) {
    NOINFO;
    VISIT_CHILD(CompSt, 0);
}

// Stmt -> RETURN Exp SEMI
VISITOR(StmtCase2) {
    NOINFO;
    IREntity ret = NEW_VAR_FROM_CHILD(1);
    NEW_SINFO(&ret, NULL, NULL, NULL);
    VISIT_WITH(Exp, 1, &sinfo);
    if (ret.kind != IREntityVar) {
        report_genir_err("The return value is not simple");
    }
    ADDIR(return, ret);
}

// Stmt -> IF LP Exp RP Stmt
VISITOR(StmtCase3) {
    NOINFO;

    IREntity true_label = NEW_LABEL();
    IREntity false_label = NEW_LABEL();
    NEW_SINFO(NULL, &true_label, &false_label, NULL);
    VISIT_WITH_UNCHK(ExpCond, 2, &sinfo);
    ADDIR(label, true_label);
    VISIT_CHILD(Stmt, 4);
    ADDIR(label, false_label);
}

// Stmt -> IF LP Exp RP Stmt ELSE Stmt
VISITOR(StmtCase4) {
    NOINFO;

    IREntity true_label = NEW_LABEL();
    IREntity false_label = NEW_LABEL();
    IREntity end_label = NEW_LABEL();
    NEW_SINFO(NULL, &true_label, &false_label, NULL);
    VISIT_WITH_UNCHK(ExpCond, 2, &sinfo);
    ADDIR(label, true_label);
    VISIT_CHILD(Stmt, 4);
    ADDIR(goto, end_label);
    ADDIR(label, false_label);
    VISIT_CHILD(Stmt, 6);
    ADDIR(label, end_label);
}

// Stmt -> WHILE LP Exp RP Stmt
VISITOR(StmtCase5) {
    NOINFO;

    IREntity begin_label = NEW_LABEL();
    IREntity iter_start_label = NEW_LABEL();
    IREntity end_label = NEW_LABEL();
    NEW_SINFO(NULL, &iter_start_label, &end_label, NULL);
    ADDIR(label, begin_label);
    VISIT_WITH_UNCHK(ExpCond, 2, &sinfo);
    ADDIR(label, iter_start_label);
    VISIT_CHILD(Stmt, 4);
    ADDIR(goto, begin_label);
    ADDIR(label, end_label);
}

// Stmt Dispatcher
// 0. Stmt -> Exp SEMI
// 1. Stmt -> CompSt
// 2. Stmt -> RETURN Exp SEMI
// 3. Stmt -> IF LP Exp RP Stmt
// 4. Stmt -> IF LP Exp RP Stmt ELSE Stmt
// 5. Stmt -> WHILE LP Exp RP Stmt
VISITOR(Stmt) {
    switch (PROD_ID()) {
        DISPATCH_ENTRY(Stmt, 0);
        DISPATCH_ENTRY(Stmt, 1);
        DISPATCH_ENTRY(Stmt, 2);
        DISPATCH_ENTRY(Stmt, 3);
        DISPATCH_ENTRY(Stmt, 4);
        DISPATCH_ENTRY(Stmt, 5);
    default:
        PANIC("unexpected production id");
    }
}

// 0. DefList -> Def DefList
// 1. DefList -> \epsilon
VISITOR(DefList) {
    NOINFO;
    if (PROD_ID() == 0) {
        VISIT_CHILD(Def, 0);
        VISIT_CHILD(DefList, 1);
    }
}

// Def -> Specifier DecList SEMI
VISITOR(Def) {
    NOINFO;
    VISIT_CHILD(DecList, 1);
}

// 0. DecList -> Dec
// 1. DecList -> Dec COMMA DecList
VISITOR(DecList) {
    NOINFO;
    VISIT_CHILD(Dec, 0);
    if (PROD_ID() == 1) {
        VISIT_CHILD(DecList, 2);
    }
}

// Dec -> VarDec
VISITOR(DecCase0) { NOINFO; }

// Dec -> VarDec ASSIGNOP Exp
VISITOR(DecCase1) {
    NOINFO;
    IREntity var = GET_VAR();
    NEW_SINFO(&var, NULL, NULL, NULL);
    VISIT_WITH(Exp, 2, &sinfo);
}

// Dec Dispatcher
// 0 Dec -> VarDec
// 1 Dec -> VarDec ASSIGNOP Exp
VISITOR(Dec) {
    switch (PROD_ID()) {
        DISPATCH_ENTRY(Dec, 0);
        DISPATCH_ENTRY(Dec, 1);
    default:
        PANIC("unexpected production id");
    }
}

// Exp -> Exp ASSIGNOP Exp
VISITOR(ExpCase0) {
    AstNode *left = DATA_CHILD(0);
    ASSERT(left->production_id == 15); // Exp -> ID
    IREntity left_var = GET_VAR_FROM_SYM(left->symentry_ptr);
    IREntity temp_var = NEW_VAR_FROM_CHILD(2);
    NEW_SINFO(&temp_var, NULL, NULL, NULL);
    VISIT_WITH(Exp, 2, &sinfo);
    ADDIR(assign, left_var, temp_var);
    ADDIR(assign, *info->target, left_var);
}

// 1. Exp -> Exp AND Exp
// 2. Exp -> Exp OR Exp
// 3. Exp -> Exp RELOP Exp
// 10. Exp -> NOT Exp
VISITOR(ExpCaseLogical) {
    IREntity true_label = NEW_LABEL();
    IREntity false_label = NEW_LABEL();

    ADDIR(assign, *info->target, self->ir_manager->zero);
    NEW_SINFO(NULL, &true_label, &false_label, NULL);
    DISPATCH_WITH(ExpCond, &sinfo);
    ADDIR(label, true_label);
    ADDIR(assign, *info->target, self->ir_manager->one);
    ADDIR(label, false_label);
}

// 4. Exp -> Exp PLUS Exp
// 5. Exp -> Exp MINUS Exp
// 6. Exp -> Exp STAR Exp
// 7. Exp -> Exp DIV Exp
VISITOR(ExpCaseArith) {
    IREntity t1 = NEW_VAR_FROM_CHILD(0);
    NEW_SINFO(&t1, NULL, NULL, NULL);
    VISIT_WITH(Exp, 0, &sinfo);

    IREntity t2 = NEW_VAR_FROM_CHILD(2);
    SINFO(&t2, NULL, NULL, NULL);
    VISIT_WITH(Exp, 2, &sinfo);

    ArithopKind kind;
    switch (DATA_CHILD(1)->grammar_symbol) {
    case GS_PLUS:
        kind = ArithopAdd;
        break;
    case GS_MINUS:
        kind = ArithopSub;
        break;
    case GS_STAR:
        kind = ArithopMul;
        break;
    case GS_DIV:
        kind = ArithopDiv;
        break;
    default:
        PANIC("Unknown op");
    }
    ADDIR(arithassign, *info->target, t1, t2, kind);
}

// Exp -> LP Exp RP
VISITOR(ExpCase8) { VISIT_WITH(Exp, 1, info); }

// Exp -> MINUS Exp
VISITOR(ExpCase9) {
    IREntity t = NEW_VAR_FROM_CHILD(1);
    NEW_SINFO(&t, NULL, NULL, NULL);
    VISIT_WITH(Exp, 1, &sinfo);

    ADDIR(arithassign, *info->target, self->ir_manager->zero, t, ArithopSub);
}

// 11. Exp -> ID LP Args RP
// 12. Exp -> ID LP RP
VISITOR(ExpCase11_12) {
    SymbolEntry *fun_sym = node->symentry_ptr;
    ASSERT(fun_sym);
    if (fun_sym == self->symbol_manager->read_fun) {
        ASSERT(PROD_ID() == 12, "read_fun must be called with no args");
        ADDIR(read, *info->target);
    } else if (fun_sym == self->symbol_manager->write_fun) {
        ASSERT(PROD_ID() == 11, "write fun must be called with args");
        AstNode *args = DATA_CHILD(2);
        ASSERT(args->grammar_symbol == GS_Args && args->production_id == 1,
               "In write, args must be a single arg");
        AstNode *exp = args->children.data[0];
        ASSERT(exp->grammar_symbol == GS_Exp);
        IREntity temp_var = NEW_VAR_FROM_TYPEIDX(exp->type_idx);
        NEW_SINFO(&temp_var, NULL, NULL, NULL);
        CALL(IRGenerator, *self, visit_Exp, /, exp, &sinfo);
        ADDIR(write, temp_var);
    } else {
        VecArgs arglist = CREOBJ(VecArgs, /);
        if (PROD_ID() == 11) {
            // Exp -> ID LP Args RP
            NEW_SINFO(NULL, NULL, NULL, &arglist);
            VISIT_WITH(Args, 2, &sinfo);
        }
        for (usize i = arglist.size - 1; ~i; i--) {
            ADDIR(arg, arglist.data[i]);
        }
        IREntity fun =
            CALL(IRManager, *self->ir_manager, gen_ent_fun, /, fun_sym->name);
        ADDIR(call, *info->target, fun);
    }
}

// Exp -> Exp LB Exp RB
VISITOR(ExpCase13) { PANIC("Unimplemented"); }

// Exp -> Exp DOT ID
VISITOR(ExpCase14) { PANIC("Unimplemented"); }

// Exp -> ID
VISITOR(ExpCase15) {
    SymbolEntry *sym = node->symentry_ptr;
    IREntity ent = GET_VAR_FROM_SYM(sym);
    ADDIR(assign, *info->target, ent);
}

// Exp -> INT
VISITOR(ExpCase16) {
    int v = DATA_CHILD(0)->int_val;
    IREntity imm_int =
        CALL(IRManager, *self->ir_manager, gen_ent_imm_int, /, v);
    ADDIR(assign, *info->target, imm_int);
}

// Exp -> FLOAT
VISITOR(ExpCase17) { report_genir_err("float value is not permitted"); }

// Exp Dispatcher
// 0. Exp -> Exp ASSIGNOP Exp
// 1. Exp -> Exp AND Exp
// 2. Exp -> Exp OR Exp
// 3. Exp -> Exp RELOP Exp
// 4. Exp -> Exp PLUS Exp
// 5. Exp -> Exp MINUS Exp
// 6. Exp -> Exp STAR Exp
// 7. Exp -> Exp DIV Exp
// 8. Exp -> LP Exp RP
// 9. Exp -> MINUS Exp
// 10. Exp -> NOT Exp
// 11. Exp -> ID LP Args RP
// 12. Exp -> ID LP RP
// 13. Exp -> Exp LB Exp RB
// 14. Exp -> Exp DOT ID
// 15. Exp -> ID
// 16. Exp -> INT
// 17. Exp -> FLOAT
VISITOR(Exp) {
    INFO;
    INFO_HAS_TARGET;
    INFO_NO_LABEL;
    switch (PROD_ID()) {
        DISPATCH_ENTRY(Exp, 0);
        DISPATCH_ENTRY(Exp, 8);
        DISPATCH_ENTRY(Exp, 9);

    case 11:
    case 12:
        DISPATCH(ExpCase11_12);
        break;

        DISPATCH_ENTRY(Exp, 13);
        DISPATCH_ENTRY(Exp, 14);
        DISPATCH_ENTRY(Exp, 15);
        DISPATCH_ENTRY(Exp, 16);
        DISPATCH_ENTRY(Exp, 17);

    case 1:
    case 2:
    case 3:
    case 10:
        DISPATCH(ExpCaseLogical);
        break;

    case 4:
    case 5:
    case 6:
    case 7:
        DISPATCH(ExpCaseArith);
        break;
    default:
        PANIC("unexpected production id");
    }
}

// Exp -> Exp AND Exp
VISITOR(ExpCondCase1) {
    IREntity middle = NEW_LABEL();
    NEW_SINFO(NULL, &middle, info->false_label, NULL);
    VISIT_WITH_UNCHK(ExpCond, 0, &sinfo);
    ADDIR(label, middle);
    SINFO(NULL, info->true_label, info->false_label, NULL);
}

// Exp -> Exp OR Exp
VISITOR(ExpCondCase2) {
    IREntity middle = NEW_LABEL();
    NEW_SINFO(NULL, info->true_label, &middle, NULL);
    VISIT_WITH_UNCHK(ExpCond, 0, &sinfo);
    ADDIR(label, middle);
    SINFO(NULL, info->true_label, info->false_label, NULL);
}

// Exp -> Exp RELOP Exp
VISITOR(ExpCondCase3) {
    IREntity t1 = NEW_VAR_FROM_CHILD(0);
    NEW_SINFO(&t1, NULL, NULL, NULL);
    VISIT_WITH(Exp, 0, &sinfo);
    IREntity t2 = NEW_VAR_FROM_CHILD(0);
    SINFO(&t2, NULL, NULL, NULL);
    VISIT_WITH(Exp, 2, &sinfo);
    RelopKind relop_kind = DATA_CHILD(1)->relop_val;
    ADDIR(condgoto, *info->true_label, t1, t2, relop_kind);
    ADDIR(goto, *info->false_label);
}

// Exp -> NOT Exp
VISITOR(ExpCondCase10) {
    NEW_SINFO(NULL, info->false_label, info->true_label, NULL);
    VISIT_WITH_UNCHK(ExpCond, 1, &sinfo);
}

// Exp -> Exp ASSIGNOP Exp
// Exp -> Exp PLUS Exp
// Exp -> Exp MINUS Exp
// Exp -> Exp STAR Exp
// Exp -> Exp DIV Exp
// Exp -> LP Exp RP
// Exp -> MINUS Exp
// Exp -> ID LP Args RP
// Exp -> ID LP RP
// Exp -> Exp LB Exp RB
// Exp -> Exp DOT ID
// Exp -> ID
// Exp -> INT
// Exp -> FLOAT
VISITOR(ExpCondNonLogical) {
    IREntity temp_var = NEW_VAR_FROM_TYPEIDX(node->type_idx);
    NEW_SINFO(&temp_var, NULL, NULL, NULL);
    VISIT_WITH(Exp, 0, &sinfo);
    ADDIR(condgoto, *info->true_label, temp_var, self->ir_manager->zero,
          RelopNE);
    ADDIR(goto, *info->false_label);
}

VISITOR(ExpCond) {
    INFO;
    INFO_HAS_LABEL;
    INFO_NO_TARGET;
    switch (PROD_ID()) {
        DISPATCH_ENTRY(ExpCond, 1);
        DISPATCH_ENTRY(ExpCond, 2);
        DISPATCH_ENTRY(ExpCond, 3);
        DISPATCH_ENTRY(ExpCond, 10);
    case 0:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
        DISPATCH(ExpCondNonLogical);
        break;

    default:
        PANIC("unexpected production id");
    }
}

// 0. Args -> Exp COMMA Args
// 1. Args -> Exp
VISITOR(Args) {
    INFO;
    INFO_NO_LABEL;
    INFO_NO_TARGET;
    ASSERT(info->arglist, "info->arglist must not be NULL");
    IREntity arg = NEW_VAR_FROM_CHILD(0);
    NEW_SINFO(&arg, NULL, NULL, NULL);
    VISIT_WITH(Exp, 0, &sinfo);
    CALL(VecArgs, *info->arglist, push_back, /, arg);

    if (PROD_ID() == 0) {
        // Args -> Exp COMMA Args
        VISIT_CHILD(Args, 2);
    }
}

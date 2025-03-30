#include "gen_ir.h"
#include "ast.h"
#include "ir.h"
#include "str.h"
#include "symbol.h"
#include "utils.h"

void MTD(IRGenerator, init, /, IRManager *ir_manager) {
    self->ir_manager = ir_manager;
    self->gen_ir_error = false;
}

typedef struct IRPassInfo {
    IREntity *place;
    IREntity *true_label;
    IREntity *false_label;
} IRPassInfo;

static void MTD(IRPassInfo, init, /, IREntity *place, IREntity *true_label,
                IREntity *false_label) {
    self->place = place;
    self->true_label = true_label;
    self->false_label = false_label;
}

// Visitor operations

#define VISITOR(gs)                                                            \
    FUNC_STATIC void MTD(IRGenerator, CONCATENATE(visit_, gs), /,              \
                         ATTR_UNUSED AstNode * node,                           \
                         ATTR_UNUSED IRPassInfo * info)

#define DISPATCH(gs)                                                           \
    ({ CALL(IRGenerator, *self, CONCATENATE(visit_, gs), /, node, info); })

#define DISPATCH_ENTRY(gs, id)                                                 \
    case id:                                                                   \
        DISPATCH(CONCATENATE3(gs, Case, id));                                  \
        break;

// Declare Visitor functions;
// e.g. void MTD(SemResolver, visit_SEMI, /, AstNode * node); etc
#define DECLVISITOR_IRGENERATOR_GRAMMAR_SYMBOL_AID(gs) VISITOR(gs);
APPLY_GRAMMAR_SYMBOL_SYNTAX(DECLVISITOR_IRGENERATOR_GRAMMAR_SYMBOL_AID);

// Info management
#define INFO ASSERT(info, "info must not be NULL")
#define NOINFO ASSERT(!info, "info must be NULL")
#define NEW_SINFO(...) IRPassInfo sinfo = CREOBJ(IRPassInfo, /, ##__VA_ARGS__)
#define SINFO(...) sinfo = CREOBJ(IRPassInfo, /, ##__VA_ARGS__)

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
#define VISIT_CHILD(gs, idx) VISIT_WITH(gs, idx, NULL);

/* ------ */

String MTD(IRGenerator, gen_ir, /, AstNode *node) {
    String ret = CREOBJ(String, /);
    self->builder = &ret;
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
VISITOR(ExtDefCase0) {}

// ExtDef -> Specifier SEMI
VISITOR(ExtDefCase1) {}

// ExtDef -> Specifier FunDec CompSt
VISITOR(ExtDefCase2) {
    NOINFO;
    VISIT_CHILD(FunDec, 1);
    VISIT_CHILD(CompSt, 2);
}

// ExtDef -> Specifier FunDec SEMI
VISITOR(ExtDefCase3) {}

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
VISITOR(ExtDecList) {}

// Specifier -> TYPE
VISITOR(SpecifierCase0) {}

// Specifier -> StructSpecifier
VISITOR(SpecifierCase1) {}

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
VISITOR(StructSpecifierCase0) {}

// StructSpecifier -> STRUCT Tag
VISITOR(StructSpecifierCase1) {}

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
VISITOR(OptTag) {}

// Tag -> ID
VISITOR(Tag) {}

// VarDec -> ID
VISITOR(VarDecCase0) {}

// VarDec -> VarDec LB INT RB
VISITOR(VarDecCase1) {}

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
    IREntity fun = NSCALL(IREntity, make_fun, /, node->symentry_ptr->name);
    CALL(IRManager, *self->ir_manager, addir_fun, /, fun);
}

// 0. VarList -> ParamDec COMMA VarList
// 1. VarList -> ParamDec
VISITOR(VarList) {}

// ParamDec -> Specifier VarDec
VISITOR(ParamDec) {}

// CompSt -> LC DefList StmtList RC
VISITOR(CompSt) {}

// 0. StmtList -> Stmt StmtList
// 1. StmtList -> \epsilon
VISITOR(StmtList) {}

// Stmt -> Exp SEMI
VISITOR(StmtCase0) {}

// Stmt -> CompSt
VISITOR(StmtCase1) {}

// Stmt -> RETURN Exp SEMI
VISITOR(StmtCase2) {}

// Stmt -> IF LP Exp RP Stmt
VISITOR(StmtCase3) {}

// Stmt -> IF LP Exp RP Stmt ELSE Stmt
VISITOR(StmtCase4) {}

// Stmt -> WHILE LP Exp RP Stmt
VISITOR(StmtCase5) {}

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
VISITOR(DefList) {}

// Def -> Specifier DecList SEMI
VISITOR(Def) {}

// 0. DecList -> Dec
// 1. DecList -> Dec COMMA DecList
VISITOR(DecList) {}

// Dec -> VarDec
VISITOR(DecCase0) {}

// Dec -> VarDec ASSIGNOP Exp
VISITOR(DecCase1) {}

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
VISITOR(ExpCase0) {}

// Exp -> Exp AND Exp
VISITOR(ExpCase1) {}

// Exp -> Exp OR Exp
VISITOR(ExpCase2) {}

// Exp -> Exp RELOP Exp
VISITOR(ExpCase3) {}

// Exp -> Exp PLUS Exp
VISITOR(ExpCase4) {}

// Exp -> Exp MINUS Exp
VISITOR(ExpCase5) {}

// Exp -> Exp STAR Exp
VISITOR(ExpCase6) {}

// Exp -> Exp DIV Exp
VISITOR(ExpCase7) {}

// Exp -> LP Exp RP
VISITOR(ExpCase8) {}

// Exp -> MINUS Exp
VISITOR(ExpCase9) {}

// Exp -> NOT Exp
VISITOR(ExpCase10) {}

// 11. Exp -> ID LP Args RP
// 12. Exp -> ID LP RP
VISITOR(ExpCase11_12) {}

// Exp -> Exp LB Exp RB
VISITOR(ExpCase13) {}

// Exp -> Exp DOT ID
VISITOR(ExpCase14) {}

// Exp -> ID
VISITOR(ExpCase15) {}

// Exp -> INT
VISITOR(ExpCase16) {}

// Exp -> FLOAT
VISITOR(ExpCase17) {}

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
    switch (PROD_ID()) {
        DISPATCH_ENTRY(Exp, 0);
        DISPATCH_ENTRY(Exp, 1);
        DISPATCH_ENTRY(Exp, 2);
        DISPATCH_ENTRY(Exp, 3);
        DISPATCH_ENTRY(Exp, 4);
        DISPATCH_ENTRY(Exp, 5);
        DISPATCH_ENTRY(Exp, 6);
        DISPATCH_ENTRY(Exp, 7);
        DISPATCH_ENTRY(Exp, 8);
        DISPATCH_ENTRY(Exp, 9);
        DISPATCH_ENTRY(Exp, 10);

    case 11:
    case 12:
        DISPATCH(ExpCase11_12);
        break;

        DISPATCH_ENTRY(Exp, 13);
        DISPATCH_ENTRY(Exp, 14);
        DISPATCH_ENTRY(Exp, 15);
        DISPATCH_ENTRY(Exp, 16);
        DISPATCH_ENTRY(Exp, 17);
    default:
        PANIC("unexpected production id");
    }
}

// 0. Args -> Exp COMMA Args
// 1. Args -> Exp
VISITOR(Args) {}

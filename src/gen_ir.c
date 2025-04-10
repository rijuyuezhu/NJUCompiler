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

static IREntity MTD(IRGenerator, var_ent_from_sym, /, SymbolEntry *sym) {
    ASSERT(sym && sym->kind == SymbolEntryVar);
    if (sym->as_var.ir_var_idx == (usize)-1) {
        IREntity ret =
            CALL(IRManager, *self->ir_manager, new_ent_var, /, IREntityVar);
        sym->as_var.ir_var_idx = ret.var_idx;
        return ret;
    } else {
        usize ir_var_idx = sym->as_var.ir_var_idx;
        return NSCALL(IREntity, make_var, /, IREntityVar, ir_var_idx);
    }
}

static IREntity MTD(IRGenerator, var_ent_new_temp, /) {
    return CALL(IRManager, *self->ir_manager, new_ent_var, /, IREntityVar);
}

#define VAR_FROM_SYM(sym) CALL(IRGenerator, *self, var_ent_from_sym, /, sym);

#define VAR_NEW_TEMP() CALL(IRGenerator, *self, var_ent_new_temp, /)

#define VAR_NOWNODE()                                                          \
    ({                                                                         \
        SymbolEntry *sym = node->symentry_ptr;                                 \
        VAR_FROM_SYM(sym);                                                     \
    })

#define VAR_CHILD(idx)                                                         \
    ({                                                                         \
        SymbolEntry *sym = DATA_CHILD(idx)->symentry_ptr;                      \
        VAR_FROM_SYM(sym);                                                     \
    })

#define NEW_LABEL() CALL(IRManager, *self->ir_manager, new_ent_label, /)

#define ADDIR(kind, ...)                                                       \
    CALL(IRManager, *self->ir_manager, CONCATENATE(addir_, kind), /,           \
         ##__VA_ARGS__);

String MTD(IRGenerator, gen_ir, /, AstNode *node) {
    String ret = CREOBJ(String, /);
    CALL(IRGenerator, *self, visit_Program, /, node, NULL);
    return ret;
}

static void MTD(IRGenerator, copy_var, /, IREntity dst_ent, usize dst_type_idx,
                IREntity src_ent, usize src_type_idx) {
    Type *dst_type =
        CALL(TypeManager, *self->type_manager, get_type, /, dst_type_idx);
    Type *src_type =
        CALL(TypeManager, *self->type_manager, get_type, /, src_type_idx);

    ASSERT(dst_type->kind == src_type->kind,
           "should not happen: type checking fail because the major classes "
           "are not the same");
    TypeKind type_kind = dst_type->kind;
    if (type_kind == TypeKindInt) {
        ADDIR(assign, dst_ent, src_ent);
    } else if (type_kind == TypeKindArray || type_kind == TypeKindStruct) {
        dst_ent.kind = IREntityVar;
        usize copy_nelem = Min(dst_type->width, src_type->width);
        ASSERT(copy_nelem % 4 == 0);
        copy_nelem /= 4;

        // use a loop to generate copy

        IREntity start = NEW_LABEL();
        IREntity end = NEW_LABEL();
        IREntity dst = VAR_NEW_TEMP();
        IREntity dst_deref = dst;
        dst_deref.kind = IREntityDeref;
        IREntity src = VAR_NEW_TEMP();
        IREntity src_deref = src;
        src_deref.kind = IREntityDeref;
        IREntity i = VAR_NEW_TEMP();
        IREntity temp = VAR_NEW_TEMP();
        IREntity copy_nelem_imm = CALL(IRManager, *self->ir_manager,
                                       new_ent_imm_int, /, (int)copy_nelem);
        IREntity four_imm =
            CALL(IRManager, *self->ir_manager, new_ent_imm_int, /, 4);

        ADDIR(assign, dst, dst_ent);              // dst := dst_ent
        ADDIR(assign, src, src_ent);              // src := src_ent
        ADDIR(assign, i, self->ir_manager->zero); // i := 0
        ADDIR(label, start);                      // LABEL start:
        ADDIR(condgoto, end, i, copy_nelem_imm,
              RelopGE);                 // IF i >= #copy_nelem_imm GOTO end
        ADDIR(assign, temp, src_deref); // temp := *src
        ADDIR(assign, dst_deref, temp); // *dst := temp
        ADDIR(arithassign, temp, dst, four_imm, ArithopAdd); // temp := dst + 4
        ADDIR(assign, dst, temp);                            // dst := temp
        ADDIR(arithassign, temp, src, four_imm, ArithopAdd); // temp := src + 4
        ADDIR(assign, src, temp);                            // src := temp
        ADDIR(arithassign, temp, i, self->ir_manager->one,
              ArithopAdd);      // temp := i + 1
        ADDIR(assign, i, temp); // i := temp
        ADDIR(goto, start);     // GOTO start
        ADDIR(label, end);      // LABEL end :
    } else {
        PANIC("Unrecognized type kind");
    }
}

static IREntity MTD(IRGenerator, get_array_index, /, usize array_type_idx,
                    IREntity base, IREntity index) {
    Type *array_type =
        CALL(TypeManager, *self->type_manager, get_type, /, array_type_idx);
    ASSERT(array_type->kind == TypeKindArray);
    usize array_ele_type_idx = array_type->as_array.subtype_idx;
    Type *array_ele_type =
        CALL(TypeManager, *self->type_manager, get_type, /, array_ele_type_idx);
    usize ele_width = array_ele_type->width;
    IREntity ele_width_imm =
        CALL(IRManager, *self->ir_manager, new_ent_imm_int, /, (int)ele_width);
    IREntity offset = VAR_NEW_TEMP();
    ADDIR(arithassign, offset, index, ele_width_imm, ArithopMul);
    IREntity result = VAR_NEW_TEMP();
    ADDIR(arithassign, result, base, offset, ArithopAdd);
    if (array_ele_type->kind == TypeKindInt) {
        result.kind = IREntityDeref;
    }
    return result;
}

static IREntity MTD(IRGenerator, get_struct_field, /, usize struct_type_idx,
                    IREntity base, SymbolEntry *field) {
    Type *struct_type =
        CALL(TypeManager, *self->type_manager, get_type, /, struct_type_idx);
    ASSERT(struct_type->kind == TypeKindStruct);
    usize offset = field->offset;
    IREntity offset_imm =
        CALL(IRManager, *self->ir_manager, new_ent_imm_int, /, (int)offset);
    IREntity result = VAR_NEW_TEMP();
    ADDIR(arithassign, result, base, offset_imm, ArithopAdd);
    Type *field_type =
        CALL(TypeManager, *self->type_manager, get_type, /, field->type_idx);
    if (field_type->kind == TypeKindInt) {
        result.kind = IREntityDeref;
    }
    return result;
}

static IREntity MTD(IRGenerator, exp_array_index_addr, /, AstNode *node) {
    // only used for Exp -> Exp LB Exp RB
    ASSERT(node->grammar_symbol == GS_Exp);
    ASSERT(node->production_id == 13);

    IREntity base = VAR_NEW_TEMP();
    NEW_SINFO(&base, NULL, NULL, NULL);
    VISIT_WITH(Exp, 0, &sinfo);
    IREntity index = VAR_NEW_TEMP();
    SINFO(&index, NULL, NULL, NULL);
    VISIT_WITH(Exp, 2, &sinfo);
    usize array_type_idx = DATA_CHILD(0)->type_idx;
    IREntity addr = CALL(IRGenerator, *self, get_array_index, /, array_type_idx,
                         base, index);
    return addr;
}

static IREntity MTD(IRGenerator, exp_struct_field_addr, /, AstNode *node) {
    // only used for Exp -> Exp DOT ID
    ASSERT(node->grammar_symbol == GS_Exp);
    ASSERT(node->production_id == 14);

    IREntity base = VAR_NEW_TEMP();
    NEW_SINFO(&base, NULL, NULL, NULL);
    VISIT_WITH(Exp, 0, &sinfo);
    usize struct_type_idx = DATA_CHILD(0)->type_idx;
    SymbolEntry *sym = node->symentry_ptr;
    IREntity addr = CALL(IRGenerator, *self, get_struct_field, /,
                         struct_type_idx, base, sym);
    return addr;
}

struct IREntity MTD(IRGenerator, exp_id_itself, /, AstNode *node) {
    // only used for Exp -> ID
    ASSERT(node->grammar_symbol == GS_Exp);
    ASSERT(node->production_id == 15);
    return VAR_NOWNODE();
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
    }
}

// ExtDef -> Specifier ExtDecList SEMI
VISITOR(ExtDefCase0) {
    NOINFO;
    // Detected global variable
    report_genir_err("global variables are not allowed");
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
    IREntity fun =
        CALL(IRManager, *self->ir_manager, new_ent_fun, /, sym->name);
    ADDIR(fun, fun);
    if (PROD_ID() == 0) {
        // FunDec -> ID LP VarList BP
        VISIT_CHILD(VarList, 2);
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
    }
}

// ParamDec -> Specifier VarDec
VISITOR(ParamDec) {
    NOINFO;
    IREntity param = VAR_NOWNODE();
    ADDIR(param, param);
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
    }
}

// Stmt -> Exp SEMI
VISITOR(StmtCase0) {
    NOINFO;
    IREntity temp_var = VAR_NEW_TEMP();
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
    IREntity ret = VAR_NEW_TEMP();
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
VISITOR(DecCase0) {
    NOINFO;
    usize type_idx = DATA_CHILD(0)->type_idx;
    Type *type = CALL(TypeManager, *self->type_manager, get_type, /, type_idx);
    if (type->kind == TypeKindInt) {
        // do nothing
    } else if (type->kind == TypeKindFloat) {
        report_genir_err("float value is not permitted");
    } else if (type->kind == TypeKindArray || type->kind == TypeKindStruct) {
        usize width = type->width;
        IREntity dec = VAR_NEW_TEMP();
        IREntity imm =
            CALL(IRManager, *self->ir_manager, new_ent_imm_int, /, (int)width);
        ADDIR(dec, dec, imm);
        dec.kind = IREntityAddr;
        IREntity nowvar = VAR_NOWNODE();
        ADDIR(assign, nowvar, dec);
    } else {
        PANIC("Invalid type->kind");
    }
}

// Dec -> VarDec ASSIGNOP Exp
VISITOR(DecCase1) {
    // delegate to case 0 for allocating appropriate space
    DISPATCH(DecCase0);

    IREntity temp_var = VAR_NEW_TEMP();
    NEW_SINFO(&temp_var, NULL, NULL, NULL);
    VISIT_WITH(Exp, 2, &sinfo);
    IREntity dst_ent = VAR_NOWNODE();
    usize dst_type_idx = DATA_CHILD(0)->type_idx;
    usize src_type_idx = DATA_CHILD(2)->type_idx;
    CALL(IRGenerator, *self, copy_var, /, dst_ent, dst_type_idx, temp_var,
         src_type_idx);
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
    AstNode *lnode = DATA_CHILD(0);
    IREntity dst_ent;
    // see `semantic.c:is_lvalue` for three kinds of lvalue
    if (lnode->production_id == 13) {
        // Exp -> Exp LB Exp RB
        dst_ent = CALL(IRGenerator, *self, exp_array_index_addr, /, lnode);
    } else if (lnode->production_id == 14) {
        // Exp -> Exp DOT ID
        dst_ent = CALL(IRGenerator, *self, exp_struct_field_addr, /, lnode);
    } else if (lnode->production_id == 15) {
        // Exp -> ID
        dst_ent = CALL(IRGenerator, *self, exp_id_itself, /, lnode);
    } else {
        PANIC("Invalid production id");
    }
    IREntity src_var = VAR_NEW_TEMP();
    NEW_SINFO(&src_var, NULL, NULL, NULL);
    VISIT_WITH(Exp, 2, &sinfo);
    usize dst_type_idx = DATA_CHILD(0)->type_idx;
    usize src_type_idx = DATA_CHILD(2)->type_idx;
    CALL(IRGenerator, *self, copy_var, /, dst_ent, dst_type_idx, src_var,
         src_type_idx);
    ADDIR(assign, *info->target, dst_ent);
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
    IREntity t1 = VAR_NEW_TEMP();
    NEW_SINFO(&t1, NULL, NULL, NULL);
    VISIT_WITH(Exp, 0, &sinfo);

    IREntity t2 = VAR_NEW_TEMP();
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
    IREntity t = VAR_NEW_TEMP();
    NEW_SINFO(&t, NULL, NULL, NULL);
    VISIT_WITH(Exp, 1, &sinfo);

    ADDIR(arithassign, *info->target, self->ir_manager->zero, t, ArithopSub);
}

// 11. Exp -> ID LP Args RP
// 12. Exp -> ID LP RP
VISITOR(ExpCaseCall) {
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
        IREntity temp_var = VAR_NEW_TEMP();
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
        DROPOBJ(VecArgs, arglist);
        IREntity fun =
            CALL(IRManager, *self->ir_manager, new_ent_fun, /, fun_sym->name);
        ADDIR(call, *info->target, fun);
    }
}

// Exp -> Exp LB Exp RB
VISITOR(ExpCase13) {
    IREntity addr = CALL(IRGenerator, *self, exp_array_index_addr, /, node);
    ADDIR(assign, *info->target, addr);
}

// Exp -> Exp DOT ID
VISITOR(ExpCase14) {
    IREntity addr = CALL(IRGenerator, *self, exp_struct_field_addr, /, node);
    ADDIR(assign, *info->target, addr);
}

// Exp -> ID
VISITOR(ExpCase15) {
    // optimize: avoid copy
    *info->target = VAR_NOWNODE();
}

// Exp -> INT
VISITOR(ExpCase16) {
    int v = DATA_CHILD(0)->int_val;
    IREntity imm_int =
        CALL(IRManager, *self->ir_manager, new_ent_imm_int, /, v);
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
        DISPATCH(ExpCaseCall);
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
    VISIT_WITH_UNCHK(ExpCond, 2, &sinfo);
}

// Exp -> Exp OR Exp
VISITOR(ExpCondCase2) {
    IREntity middle = NEW_LABEL();
    NEW_SINFO(NULL, info->true_label, &middle, NULL);
    VISIT_WITH_UNCHK(ExpCond, 0, &sinfo);
    ADDIR(label, middle);
    SINFO(NULL, info->true_label, info->false_label, NULL);
    VISIT_WITH_UNCHK(ExpCond, 2, &sinfo);
}

// Exp -> Exp RELOP Exp
VISITOR(ExpCondCase3) {
    IREntity t1 = VAR_NEW_TEMP();
    NEW_SINFO(&t1, NULL, NULL, NULL);
    VISIT_WITH(Exp, 0, &sinfo);
    IREntity t2 = VAR_NEW_TEMP();
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
    IREntity temp_var = VAR_NEW_TEMP();
    NEW_SINFO(&temp_var, NULL, NULL, NULL);
    DISPATCH_WITH(Exp, &sinfo);
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
    IREntity arg = VAR_NEW_TEMP();
    NEW_SINFO(&arg, NULL, NULL, NULL);
    VISIT_WITH(Exp, 0, &sinfo);
    CALL(VecArgs, *info->arglist, push_back, /, arg);

    if (PROD_ID() == 0) {
        // Args -> Exp COMMA Args
        VISIT_WITH(Args, 2, info);
    }
}

DEFINE_PLAIN_VEC(VecArgs, IREntity, FUNC_STATIC);

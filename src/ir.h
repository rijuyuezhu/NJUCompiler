#pragma once

#include "general_vec.h"
#include "op.h"
#include "utils.h"

struct String;

typedef enum IREntityKind {
    IREntityInvalid,
    IREntityImmInt, // starts with #
    IREntityVar,    // starts with v
    IREntityAddr,   // starts with &v
    IREntityDeref,  // starts with *v
    IREntityLabel,  // starts with l
    IREntityFun,    // fun_name->name
} IREntityKind;

typedef struct IREntity {
    IREntityKind kind;
    union {
        int imm_int;
        usize var_idx;
        usize label_idx;
        struct String *fun_name;
    };
} IREntity;

IREntity NSMTD(IREntity, make_imm_int, /, int imm_int);
IREntity NSMTD(IREntity, make_var, /, IREntityKind kind, usize idx);
IREntity NSMTD(IREntity, make_label, /, usize idx);
IREntity NSMTD(IREntity, make_fun, /, struct String *fun_name);

void MTD(IREntity, build_str, /, struct String *builder);
void MTD(IREntity, build_str_nosymprefix, /, struct String *builder);

#define APPLY_IR_KIND(f)                                                       \
    f(Label)           /* LABEL {e1:label} : */                                \
        f(Function)    /* FUNCTION {e1:fun} : */                               \
        f(Assign)      /* {ret:var} := {e1:var/e1:imm_int/e1:addr/e1:deref},   \
                          {ret:deref} := {e1:var} */                           \
        f(ArithAssign) /* {ret:var} := {e1:var} {arithop_val} {e2:var} */      \
        f(Goto)        /* GOTO {ret:label} */                                  \
        f(CondGoto)    /* IF {e1:var} {relop_val} {e2:var} GOTO {ret:label} */ \
        f(Return)      /* RETURN {e1:var} */                                   \
        f(Dec)         /* DEC {ret:var} {e1:imm w/o #} */                      \
        f(Arg)         /* ARG {e1:var/e1:addr} */                              \
        f(Call)        /* {ret:var} := CALL {e1:fun} */                        \
        f(Param)       /* PARAM {e1:var/e1:addr} */                            \
        f(Read)        /* READ {ret:var} */                                    \
        f(Write)       /* WRITE {e1:var} */

#define ENUM_IR_KIND_AID(irkind) CONCATENATE(IR, irkind),
typedef enum IRKind { IRInvalid, APPLY_IR_KIND(ENUM_IR_KIND_AID) } IRKind;

typedef struct IR {
    IRKind kind;
    IREntity e1;
    IREntity e2;
    IREntity ret;

    union {
        ArithopKind arithop_val;
        RelopKind relop_val;
    };
} IR;

FUNC_STATIC DEFAULT_DROPER(IR);

IR *NSMTD(IR, creheap_label, /, IREntity label);
IR *NSMTD(IR, creheap_fun, /, IREntity fun);
IR *NSMTD(IR, creheap_assign, /, IREntity lhs, IREntity rhs);
IR *NSMTD(IR, creheap_arithassign, /, IREntity lhs, IREntity rhs1,
          IREntity rhs2, ArithopKind aop);
IR *NSMTD(IR, creheap_goto, /, IREntity label);
IR *NSMTD(IR, creheap_condgoto, /, IREntity label, IREntity rop1, IREntity rop2,
          RelopKind rop);
IR *NSMTD(IR, creheap_return, /, IREntity ret);
IR *NSMTD(IR, creheap_dec, /, IREntity dec, IREntity imm);
IR *NSMTD(IR, creheap_arg, /, IREntity arg);
IR *NSMTD(IR, creheap_call, /, IREntity lhs, IREntity fun);
IR *NSMTD(IR, creheap_param, /, IREntity par);
IR *NSMTD(IR, creheap_read, /, IREntity ret);
IR *NSMTD(IR, creheap_write, /, IREntity par);

void MTD(IR, build_str, /, struct String *builder);

typedef struct IRManager {
    VecPtr irs;
    usize idx_cur;

    // some `constants`
    IREntity zero;
    IREntity one;
} IRManager;

void MTD(IRManager, init, /);
void MTD(IRManager, drop, /);
usize MTD(IRManager, new_idx, /);

void MTD(IRManager, addir_label, /, IREntity label);
void MTD(IRManager, addir_fun, /, IREntity fun);
void MTD(IRManager, addir_assign, /, IREntity lhs, IREntity rhs);
void MTD(IRManager, addir_arithassign, /, IREntity lhs, IREntity rhs1,
         IREntity rhs2, ArithopKind aop);
void MTD(IRManager, addir_goto, /, IREntity label);
void MTD(IRManager, addir_condgoto, /, IREntity label, IREntity rop1,
         IREntity rop2, RelopKind rop);
void MTD(IRManager, addir_return, /, IREntity ret);
void MTD(IRManager, addir_dec, /, IREntity dec, IREntity imm);
void MTD(IRManager, addir_arg, /, IREntity arg);
void MTD(IRManager, addir_call, /, IREntity lhs, IREntity fun);
void MTD(IRManager, addir_param, /, IREntity par);
void MTD(IRManager, addir_read, /, IREntity ret);
void MTD(IRManager, addir_write, /, IREntity par);

IREntity MTD(IRManager, gen_ent_imm_int, /, int imm_int);
IREntity MTD(IRManager, gen_ent_var, /, IREntityKind kind);
IREntity MTD(IRManager, gen_ent_label, /);
IREntity MTD(IRManager, gen_ent_fun, /, struct String *fun_name);

void MTD(IRManager, build_str, /, struct String *builder);
struct String MTD(IRManager, get_ir_str, /);

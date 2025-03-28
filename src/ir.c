#include "ir.h"
#include "str.h"
#include "utils.h"

IREntity NSMTD(IREntity, make_imm_int, /, int imm_int) {
    return (IREntity){
        .kind = IREntityImmInt,
        .imm_int = imm_int,
    };
}
IREntity NSMTD(IREntity, make_var, /, IREntityKind kind, usize idx) {
    return (IREntity){
        .kind = kind,
        .var_idx = idx,
    };
}
IREntity NSMTD(IREntity, make_label, /, usize idx) {
    return (IREntity){
        .kind = IREntityLabel,
        .label_idx = idx,
    };
}
IREntity NSMTD(IREntity, make_fun, /, String *fun_name) {
    return (IREntity){
        .kind = IREntityFun,
        .fun_name = fun_name,
    };
}

void MTD(IREntity, build_str, /, String *builder) {
    switch (self->kind) {
    case IREntityImmInt:
        CALL(String, *builder, pushf, /, "#%d", self->imm_int);
        break;
    case IREntityVar:
        CALL(String, *builder, pushf, /, "v%zu", self->var_idx);
        break;
    case IREntityAddr:
        CALL(String, *builder, pushf, /, "&v%zu", self->var_idx);
        break;
    case IREntityDeref:
        CALL(String, *builder, pushf, /, "*v%zu", self->var_idx);
        break;
    case IREntityLabel:
        CALL(String, *builder, pushf, /, "l%zu", self->label_idx);
        break;
    case IREntityFun: {
        const char *fun_name = STRING_C_STR(*self->fun_name);
        CALL(String, *builder, pushf, /, "%s", fun_name);
        break;
    }
    default:
        PANIC("Invalid IREntityKind");
    }
}

void MTD(IREntity, build_str_nosymprefix, /, String *builder) {
    switch (self->kind) {
    case IREntityImmInt:
        CALL(String, *builder, pushf, /, "%d", self->imm_int);
        break;
    case IREntityVar:
    case IREntityAddr:
    case IREntityDeref:
        CALL(String, *builder, pushf, /, "v%zu", self->var_idx);
        break;
    case IREntityLabel:
        CALL(String, *builder, pushf, /, "l%zu", self->label_idx);
        break;
    case IREntityFun: {
        const char *fun_name = STRING_C_STR(*self->fun_name);
        CALL(String, *builder, pushf, /, "%s", fun_name);
        break;
    }
    default:
        PANIC("Invalid IREntityKind");
    }
}

IR *NSMTD(IR, creheap_label, /, IREntity label) {
    ASSERT(label.kind == IREntityLabel);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRLabel;
    ir->e1 = label;
    return ir;
}
IR *NSMTD(IR, creheap_fun, /, IREntity fun) {
    ASSERT(fun.kind == IREntityFun);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRLabel;
    ir->e1 = fun;
    return ir;
}
IR *NSMTD(IR, creheap_assign, /, IREntity lhs, IREntity rhs) {
    ASSERT((lhs.kind == IREntityVar &&
            (rhs.kind == IREntityVar || rhs.kind == IREntityAddr ||
             rhs.kind == IREntityDeref || rhs.kind == IREntityImmInt)) ||
           (lhs.kind == IREntityDeref && rhs.kind == IREntityVar));
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRAssign;
    ir->ret = lhs;
    ir->e1 = rhs;
    return ir;
}
IR *NSMTD(IR, creheap_arithassign, /, IREntity lhs, IREntity rhs1,
          IREntity rhs2, ArithopKind aop) {
    ASSERT(lhs.kind == IREntityVar && rhs1.kind == IREntityVar &&
           rhs2.kind == IREntityVar);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRArithAssign;
    ir->ret = lhs;
    ir->e1 = rhs1;
    ir->e2 = rhs2;
    ir->arithop_val = aop;
    return ir;
}
IR *NSMTD(IR, creheap_goto, /, IREntity label) {
    ASSERT(label.kind == IREntityLabel);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRGoto;
    ir->ret = label;
    return ir;
}
IR *NSMTD(IR, creheap_condgoto, /, IREntity label, IREntity rop1, IREntity rop2,
          RelopKind rop) {
    ASSERT(label.kind == IREntityLabel && rop1.kind == IREntityVar &&
           rop2.kind == IREntityVar);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRCondGoto;
    ir->ret = label;
    ir->e1 = rop1;
    ir->e2 = rop2;
    ir->relop_val = rop;
    return ir;
}
IR *NSMTD(IR, creheap_return, /, IREntity ret) {
    ASSERT(ret.kind == IREntityVar);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRReturn;
    ir->e1 = ret;
    return ir;
}
IR *NSMTD(IR, creheap_dec, /, IREntity dec, IREntity imm) {
    ASSERT(dec.kind == IREntityVar && imm.kind == IREntityImmInt);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRDec;
    ir->ret = dec;
    ir->e1 = imm;
    return ir;
}
IR *NSMTD(IR, creheap_call, /, IREntity lhs, IREntity fun) {
    ASSERT(lhs.kind == IREntityVar && fun.kind == IREntityFun);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRCall;
    ir->ret = lhs;
    ir->e1 = fun;
    return ir;
}
IR *NSMTD(IR, creheap_param, /, IREntity par) {
    ASSERT(par.kind == IREntityVar || par.kind == IREntityAddr);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRParam;
    ir->e1 = par;
    return ir;
}
IR *NSMTD(IR, creheap_read, /, IREntity ret) {
    ASSERT(ret.kind == IREntityVar);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRRead;
    ir->ret = ret;
    return ir;
}
IR *NSMTD(IR, creheap_write, /, IREntity par) {
    ASSERT(par.kind == IREntityVar);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRWrite;
    ir->e1 = par;
    return ir;
}

void MTD(IRManager, init, /) {
    CALL(VecPtr, self->irs, init, /);
    self->idx_cur = 0;
}

void MTD(IRManager, drop, /) {
    for (usize i = 0; i < self->irs.size; i++) {
        DROPOBJHEAP(IR, self->irs.data[i]);
    }
    DROPOBJ(VecPtr, self->irs);
}

usize MTD(IRManager, new_idx, /) { return self->idx_cur++; }

void MTD(IRManager, addir_label, /, IREntity label) {
    IR *ir = NSCALL(IR, creheap_label, /, label);
    CALL(VecPtr, self->irs, push_back, /, ir);
}

void MTD(IRManager, addir_fun, /, IREntity fun) {
    IR *ir = NSCALL(IR, creheap_fun, /, fun);
    CALL(VecPtr, self->irs, push_back, /, ir);
}
void MTD(IRManager, addir_assign, /, IREntity lhs, IREntity rhs) {
    IR *ir = NSCALL(IR, creheap_assign, /, lhs, rhs);
    CALL(VecPtr, self->irs, push_back, /, ir);
}
void MTD(IRManager, addir_arithassign, /, IREntity lhs, IREntity rhs1,
         IREntity rhs2, ArithopKind aop) {
    IR *ir = NSCALL(IR, creheap_arithassign, /, lhs, rhs1, rhs2, aop);
    CALL(VecPtr, self->irs, push_back, /, ir);
}
void MTD(IRManager, addir_goto, /, IREntity label) {
    IR *ir = NSCALL(IR, creheap_goto, /, label);
    CALL(VecPtr, self->irs, push_back, /, ir);
}
void MTD(IRManager, addir_condgoto, /, IREntity label, IREntity rop1,
         IREntity rop2, RelopKind rop) {
    IR *ir = NSCALL(IR, creheap_condgoto, /, label, rop1, rop2, rop);
    CALL(VecPtr, self->irs, push_back, /, ir);
}
void MTD(IRManager, addir_return, /, IREntity ret) {
    IR *ir = NSCALL(IR, creheap_return, /, ret);
    CALL(VecPtr, self->irs, push_back, /, ir);
}
void MTD(IRManager, addir_dec, /, IREntity dec, IREntity imm) {
    IR *ir = NSCALL(IR, creheap_dec, /, dec, imm);
    CALL(VecPtr, self->irs, push_back, /, ir);
}
void MTD(IRManager, addir_call, /, IREntity lhs, IREntity fun) {
    IR *ir = NSCALL(IR, creheap_call, /, lhs, fun);
    CALL(VecPtr, self->irs, push_back, /, ir);
}
void MTD(IRManager, addir_param, /, IREntity par) {
    IR *ir = NSCALL(IR, creheap_param, /, par);
    CALL(VecPtr, self->irs, push_back, /, ir);
}
void MTD(IRManager, addir_read, /, IREntity ret) {
    IR *ir = NSCALL(IR, creheap_read, /, ret);
    CALL(VecPtr, self->irs, push_back, /, ir);
}
void MTD(IRManager, addir_write, /, IREntity par) {
    IR *ir = NSCALL(IR, creheap_write, /, par);
    CALL(VecPtr, self->irs, push_back, /, ir);
}

IREntity MTD(IRManager, gen_ent_imm_int, /, int imm_int) {
    return NSCALL(IREntity, make_imm_int, /, imm_int);
}
IREntity MTD(IRManager, gen_ent_var, /, IREntityKind kind) {
    usize idx = CALL(IRManager, *self, new_idx, /);
    return NSCALL(IREntity, make_var, /, kind, idx);
}
IREntity MTD(IRManager, gen_ent_label, /) {
    usize idx = CALL(IRManager, *self, new_idx, /);
    return NSCALL(IREntity, make_label, /, idx);
}
IREntity MTD(IRManager, gen_ent_fun, /, struct String *fun_name) {
    return NSCALL(IREntity, make_fun, /, fun_name);
}

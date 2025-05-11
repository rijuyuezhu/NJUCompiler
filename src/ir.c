#include "ir.h"
#include "op.h"
#include "str.h"
#include "utils.h"

IREntity NSMTD(IREntity, make_imm_int, /, int imm_int) {
    return (IREntity){
        .kind = IREntityImmInt,
        .imm_int = imm_int,
    };
}
IREntity NSMTD(IREntity, make_var, /, IREntityKind kind, usize idx) {
    ASSERT(kind == IREntityVar || kind == IREntityAddr ||
           kind == IREntityDeref);
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
IREntity NSMTD(IREntity, make_fun, /, usize arity, String *name) {
    return (IREntity){
        .kind = IREntityFun,
        .as_fun =
            {
                .arity = arity,
                .name = name,
            },
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
        const char *fun_name = STRING_C_STR(*self->as_fun.name);
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
    ir->kind = IRFunction;
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
    ASSERT(lhs.kind == IREntityVar &&
           (rhs1.kind == IREntityVar || rhs1.kind == IREntityImmInt) &&
           (rhs2.kind == IREntityVar || rhs2.kind == IREntityImmInt));
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
    ASSERT(label.kind == IREntityLabel &&
           (rop1.kind == IREntityVar || rop1.kind == IREntityImmInt) &&
           (rop2.kind == IREntityVar || rop2.kind == IREntityImmInt));
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
IR *NSMTD(IR, creheap_arg, /, IREntity arg) {
    ASSERT(arg.kind == IREntityVar);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRArg;
    ir->e1 = arg;
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
IR *NSMTD(IR, creheap_param, /, IREntity param) {
    ASSERT(param.kind == IREntityVar);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRParam;
    ir->e1 = param;
    return ir;
}
IR *NSMTD(IR, creheap_read, /, IREntity ret) {
    ASSERT(ret.kind == IREntityVar);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRRead;
    ir->ret = ret;
    return ir;
}
IR *NSMTD(IR, creheap_write, /, IREntity param) {
    ASSERT(param.kind == IREntityVar);
    IR *ir = CREOBJRAWHEAP(IR);
    ir->kind = IRWrite;
    ir->e1 = param;
    return ir;
}

#define BUILD_STR_IR(irkind)                                                   \
    FUNC_STATIC void MTD(IR, CONCATENATE(build_str_, irkind), /,               \
                         String * builder)
#define DECLARE_BUILD_STR_IR_AID(irkind) BUILD_STR_IR(irkind);

APPLY_IR_KIND(DECLARE_BUILD_STR_IR_AID);

#define JUMP_TABLE_IR_BUILD_STR_IR(irkind)                                     \
    case CONCATENATE(IR, irkind): {                                            \
        CALL(IR, *self, CONCATENATE(build_str_, irkind), /, builder);          \
        break;                                                                 \
    }

BUILD_STR_IR(Label) {
    CALL(String, *builder, push_str, /, "LABEL ");
    CALL(IREntity, self->e1, build_str, /, builder);
    CALL(String, *builder, push_str, /, " :\n");
}

BUILD_STR_IR(Function) {
    CALL(String, *builder, push_str, /, "FUNCTION ");
    CALL(IREntity, self->e1, build_str, /, builder);
    CALL(String, *builder, push_str, /, " :\n");
}
BUILD_STR_IR(Assign) {
    CALL(IREntity, self->ret, build_str, /, builder);
    CALL(String, *builder, push_str, /, " := ");
    CALL(IREntity, self->e1, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}
BUILD_STR_IR(ArithAssign) {
    CALL(IREntity, self->ret, build_str, /, builder);
    CALL(String, *builder, push_str, /, " := ");
    CALL(IREntity, self->e1, build_str, /, builder);
    char op;
    switch (self->arithop_val) {
    case ArithopAdd:
        op = '+';
        break;
    case ArithopSub:
        op = '-';
        break;
    case ArithopMul:
        op = '*';
        break;
    case ArithopDiv:
        op = '/';
        break;
    default:
        PANIC("Invalid ArithopKind");
    }
    CALL(String, *builder, pushf, /, " %c ", op);
    CALL(IREntity, self->e2, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}

BUILD_STR_IR(Goto) {
    CALL(String, *builder, push_str, /, "GOTO ");
    CALL(IREntity, self->ret, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}

BUILD_STR_IR(CondGoto) {
    CALL(String, *builder, push_str, /, "IF ");
    CALL(IREntity, self->e1, build_str, /, builder);
    const char *op;
    switch (self->relop_val) {
    case RelopEQ:
        op = "==";
        break;
    case RelopNE:
        op = "!=";
        break;
    case RelopLT:
        op = "<";
        break;
    case RelopLE:
        op = "<=";
        break;
    case RelopGT:
        op = ">";
        break;
    case RelopGE:
        op = ">=";
        break;
    default:
        PANIC("Invalid RelopKind");
    }
    CALL(String, *builder, pushf, /, " %s ", op);
    CALL(IREntity, self->e2, build_str, /, builder);
    CALL(String, *builder, push_str, /, " GOTO ");
    CALL(IREntity, self->ret, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}
BUILD_STR_IR(Return) {
    CALL(String, *builder, push_str, /, "RETURN ");
    CALL(IREntity, self->e1, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}
BUILD_STR_IR(Dec) {
    CALL(String, *builder, push_str, /, "DEC ");
    CALL(IREntity, self->ret, build_str, /, builder);
    CALL(String, *builder, pushf, /, " %d\n", self->e1.imm_int);
}
BUILD_STR_IR(Arg) {
    CALL(String, *builder, push_str, /, "ARG ");
    CALL(IREntity, self->e1, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}
BUILD_STR_IR(Call) {
    CALL(IREntity, self->ret, build_str, /, builder);
    CALL(String, *builder, push_str, /, " := CALL ");
    CALL(IREntity, self->e1, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}
BUILD_STR_IR(Param) {
    CALL(String, *builder, push_str, /, "PARAM ");
    CALL(IREntity, self->e1, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}
BUILD_STR_IR(Read) {
    CALL(String, *builder, push_str, /, "READ ");
    CALL(IREntity, self->ret, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}
BUILD_STR_IR(Write) {
    CALL(String, *builder, push_str, /, "WRITE ");
    CALL(IREntity, self->e1, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}

void MTD(IR, build_str, /, String *builder) {
    switch (self->kind) {
        APPLY_IR_KIND(JUMP_TABLE_IR_BUILD_STR_IR);
    default:
        PANIC("Invalid IRKind");
    }
}

void MTD(IRManager, init, /) {
    CALL(VecPtr, self->irs, init, /);
    self->idx_cur = 0;
    self->zero = CALL(IRManager, *self, new_ent_imm_int, /, 0);
    self->one = CALL(IRManager, *self, new_ent_imm_int, /, 1);
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
void MTD(IRManager, addir_arg, /, IREntity arg) {
    IR *ir = NSCALL(IR, creheap_arg, /, arg);
    CALL(VecPtr, self->irs, push_back, /, ir);
}
void MTD(IRManager, addir_call, /, IREntity lhs, IREntity fun) {
    IR *ir = NSCALL(IR, creheap_call, /, lhs, fun);
    CALL(VecPtr, self->irs, push_back, /, ir);
}
void MTD(IRManager, addir_param, /, IREntity param) {
    IR *ir = NSCALL(IR, creheap_param, /, param);
    CALL(VecPtr, self->irs, push_back, /, ir);
}
void MTD(IRManager, addir_read, /, IREntity ret) {
    IR *ir = NSCALL(IR, creheap_read, /, ret);
    CALL(VecPtr, self->irs, push_back, /, ir);
}
void MTD(IRManager, addir_write, /, IREntity param) {
    IR *ir = NSCALL(IR, creheap_write, /, param);
    CALL(VecPtr, self->irs, push_back, /, ir);
}

IREntity MTD(IRManager, new_ent_imm_int, /, int imm_int) {
    return NSCALL(IREntity, make_imm_int, /, imm_int);
}
IREntity MTD(IRManager, new_ent_var, /, IREntityKind kind) {
    usize idx = CALL(IRManager, *self, new_idx, /);
    return NSCALL(IREntity, make_var, /, kind, idx);
}
IREntity MTD(IRManager, new_ent_label, /) {
    usize idx = CALL(IRManager, *self, new_idx, /);
    return NSCALL(IREntity, make_label, /, idx);
}
IREntity MTD(IRManager, new_ent_fun, /, usize arity, struct String *name) {
    return NSCALL(IREntity, make_fun, /, arity, name);
}

void MTD(IRManager, build_str, /, String *builder) {
    for (usize i = 0; i < self->irs.size; i++) {
        IR *ir = self->irs.data[i];
        CALL(IR, *ir, build_str, /, builder);
    }
}

String MTD(IRManager, get_ir_str, /) {
    String builder = CREOBJ(String, /);
    CALL(IRManager, *self, build_str, /, &builder);
    return builder;
}

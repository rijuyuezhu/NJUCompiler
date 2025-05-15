#include "ir_stmt.h"
#include "op.h"
#include "str.h"
#include "utils.h"

// IRStmtAssign
void MTD(IRStmtAssign, init, /, usize dst, IRValue src) {
    CALL(IRStmtAssign, *self, base_init, /);
    self->dst = dst;
    self->src = src;
}
DEFAULT_DROPER(IRStmtAssign);
void MTD(IRStmtAssign, build_str, /, String *builder) {
    CALL(String, *builder, pushf, /, "v%zu := ", self->dst);
    CALL(IRValue, self->src, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}

usize MTD(IRStmtAssign, get_def, /) { return self->dst; }
SliceIRValue MTD(IRStmtAssign, get_use, /) {
    return (SliceIRValue){
        .data = self->use_repr,
        .size = LENGTH(self->use_repr),
    };
}

// IRStmtArith
void MTD(IRStmtLoad, init, /, usize dst, IRValue src_addr) {
    CALL(IRStmtLoad, *self, base_init, /);
    self->dst = dst;
    self->src_addr = src_addr;
}
DEFAULT_DROPER(IRStmtLoad);
void MTD(IRStmtLoad, build_str, /, String *builder) {
    CALL(String, *builder, pushf, /, "v%zu := *", self->dst);
    CALL(IRValue, self->src_addr, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}
usize MTD(IRStmtLoad, get_def, /) { return self->dst; }
SliceIRValue MTD(IRStmtLoad, get_use, /) {
    return (SliceIRValue){
        .data = self->use_repr,
        .size = LENGTH(self->use_repr),
    };
}

// IRStmtStore
void MTD(IRStmtStore, init, /, IRValue dst_addr, IRValue src) {
    CALL(IRStmtStore, *self, base_init, /);
    self->dst_addr = dst_addr;
    self->src = src;
}
DEFAULT_DROPER(IRStmtStore);
void MTD(IRStmtStore, build_str, /, String *builder) {
    CALL(String, *builder, push_str, /, "*");
    CALL(IRValue, self->dst_addr, build_str, /, builder);
    CALL(String, *builder, push_str, /, " := ");
    CALL(IRValue, self->src, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}
usize MTD(IRStmtStore, get_def, /) { return (usize)-1; }
SliceIRValue MTD(IRStmtStore, get_use, /) {
    return (SliceIRValue){
        .data = self->use_repr,
        .size = LENGTH(self->use_repr),
    };
}

// IRStmtArith
void MTD(IRStmtArith, init, /, usize dst, IRValue src1, IRValue src2,
         ArithopKind aop) {
    CALL(IRStmtArith, *self, base_init, /);
    self->dst = dst;
    self->src1 = src1;
    self->src2 = src2;
    self->aop = aop;
}
DEFAULT_DROPER(IRStmtArith);
static const char *ArithopKind_to_str(ArithopKind aop) {
    switch (aop) {
    case ArithopAdd:
        return "+";
    case ArithopSub:
        return "-";
    case ArithopMul:
        return "*";
    case ArithopDiv:
        return "/";
    default:
        PANIC("Should not reach here");
    }
}
void MTD(IRStmtArith, build_str, /, String *builder) {
    CALL(String, *builder, pushf, /, "v%zu := ", self->dst);
    CALL(IRValue, self->src1, build_str, /, builder);
    CALL(String, *builder, pushf, /, " %s ", ArithopKind_to_str(self->aop));
    CALL(IRValue, self->src2, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}
usize MTD(IRStmtArith, get_def, /) { return self->dst; }
SliceIRValue MTD(IRStmtArith, get_use, /) {
    return (SliceIRValue){
        .data = self->use_repr,
        .size = LENGTH(self->use_repr),
    };
}

// IRStmtGoto
void MTD(IRStmtGoto, init, /, usize label) {
    CALL(IRStmtGoto, *self, base_init, /);
    self->label = label;
}
DEFAULT_DROPER(IRStmtGoto);
void MTD(IRStmtGoto, build_str, /, String *builder) {
    CALL(String, *builder, pushf, /, "GOTO l%zu\n", self->label);
}
usize MTD(IRStmtGoto, get_def, /) { return (usize)-1; }
SliceIRValue MTD(IRStmtGoto, get_use, /) {
    return (SliceIRValue){
        .data = NULL,
        .size = 0,
    };
}

// IRStmtIf
void MTD(IRStmtIf, init, /, IRValue src1, IRValue src2, RelopKind rop,
         usize label) {
    CALL(IRStmtIf, *self, base_init, /);
    self->src1 = src1;
    self->src2 = src2;
    self->rop = rop;
    self->true_label = label;
    self->false_label = (usize)-1;
}
DEFAULT_DROPER(IRStmtIf);
static const char *RelopKind_to_str(RelopKind rop) {
    switch (rop) {
    case RelopEQ:
        return "==";
    case RelopNE:
        return "!=";
    case RelopLT:
        return "<";
    case RelopLE:
        return "<=";
    case RelopGT:
        return ">";
    case RelopGE:
        return ">=";
    default:
        PANIC("Should not reach here");
    }
}
void MTD(IRStmtIf, build_str, /, String *builder) {
    CALL(String, *builder, push_str, /, "IF ");
    CALL(IRValue, self->src1, build_str, /, builder);
    CALL(String, *builder, pushf, /, " %s ", RelopKind_to_str(self->rop));
    CALL(IRValue, self->src2, build_str, /, builder);
    CALL(String, *builder, pushf, /, " GOTO l%zu\n", self->true_label);
    if (self->false_label != (usize)-1) {
        CALL(String, *builder, pushf, /, "GOTO l%zu\n", self->false_label);
    }
}
usize MTD(IRStmtIf, get_def, /) { return (usize)-1; }
SliceIRValue MTD(IRStmtIf, get_use, /) {
    return (SliceIRValue){
        .data = self->use_repr,
        .size = LENGTH(self->use_repr),
    };
}
static RelopKind RelopKind_flip(RelopKind rop) {
    switch (rop) {
    case RelopEQ:
        return RelopNE;
    case RelopNE:
        return RelopEQ;
    case RelopLT:
        return RelopGE;
    case RelopLE:
        return RelopGT;
    case RelopGT:
        return RelopLE;
    case RelopGE:
        return RelopLT;
    default:
        PANIC("Should not reach here");
    }
}
void MTD(IRStmtIf, flip, /) {
    usize temp_label = self->true_label;
    self->true_label = self->false_label;
    self->false_label = temp_label;
    self->rop = RelopKind_flip(self->rop);
}

// IRStmtCall
void MTD(IRStmtCall, init, /, usize dst, String func_name, VecIRValue args) {
    CALL(IRStmtCall, *self, base_init, /);
    self->dst = dst;
    self->func_name = func_name;
    self->args = args;
}
void MTD(IRStmtCall, drop, /) {
    DROPOBJ(VecIRValue, self->args);
    DROPOBJ(String, self->func_name);
}
void MTD(IRStmtCall, build_str, /, String *builder) {
    for (usize i = 0; i < self->args.size; i++) {
        CALL(String, *builder, push_str, /, "ARG ");
        CALL(IRValue, self->args.data[i], build_str, /, builder);
        CALL(String, *builder, push_str, /, "\n");
    }
    const char *func_name = STRING_C_STR(self->func_name);
    CALL(String, *builder, pushf, /, "v%zu := CALL %s\n", self->dst, func_name);
}

usize MTD(IRStmtCall, get_def, /) { return self->dst; }
SliceIRValue MTD(IRStmtCall, get_use, /) {
    return (SliceIRValue){
        .data = self->args.data,
        .size = self->args.size,
    };
}

// IRStmtReturn
void MTD(IRStmtReturn, init, /, IRValue src) {
    CALL(IRStmtReturn, *self, base_init, /);
    self->src = src;
}
DEFAULT_DROPER(IRStmtReturn);
void MTD(IRStmtReturn, build_str, /, String *builder) {
    CALL(String, *builder, push_str, /, "RETURN ");
    CALL(IRValue, self->src, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}
usize MTD(IRStmtReturn, get_def, /) { return (usize)-1; }
SliceIRValue MTD(IRStmtReturn, get_use, /) {
    return (SliceIRValue){
        .data = self->use_repr,
        .size = LENGTH(self->use_repr),
    };
}

// IRStmtRead
void MTD(IRStmtRead, init, /, usize dst) {
    CALL(IRStmtRead, *self, base_init, /);
    self->dst = dst;
}
DEFAULT_DROPER(IRStmtRead);
void MTD(IRStmtRead, build_str, /, String *builder) {
    CALL(String, *builder, pushf, /, "READ v%zu\n", self->dst);
}
usize MTD(IRStmtRead, get_def, /) { return self->dst; }
SliceIRValue MTD(IRStmtRead, get_use, /) {
    return (SliceIRValue){
        .data = NULL,
        .size = 0,
    };
}

// IRStmtWrite
void MTD(IRStmtWrite, init, /, IRValue src) {
    CALL(IRStmtWrite, *self, base_init, /);
    self->src = src;
}
DEFAULT_DROPER(IRStmtWrite);
void MTD(IRStmtWrite, build_str, /, String *builder) {
    CALL(String, *builder, push_str, /, "WRITE ");
    CALL(IRValue, self->src, build_str, /, builder);
    CALL(String, *builder, push_str, /, "\n");
}
usize MTD(IRStmtWrite, get_def, /) { return (usize)-1; }
SliceIRValue MTD(IRStmtWrite, get_use, /) {
    return (SliceIRValue){
        .data = self->use_repr,
        .size = LENGTH(self->use_repr),
    };
}

DEFINE_PLAIN_VEC(VecIRValue, IRValue, FUNC_EXTERN)
DEFINE_LIST(ListDynIRStmt, IRStmtBase *, FUNC_EXTERN);

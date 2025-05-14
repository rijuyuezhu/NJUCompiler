#include "ir.h"
#include "str.h"
#include "utils.h"

// IRStmtAssign
void MTD(IRStmtAssign, init, /, usize dst, usize src) {
    CALL(IRStmtAssign, *self, base_init, /);
    self->dst = dst;
    self->src = src;
}
DEFAULT_DROPER(IRStmtAssign);
void MTD(IRStmtAssign, build_str, /, String *builder) {
    CALL(String, *builder, pushf, /, "v%zu := v%zu", self->dst, self->src);
}
usize MTD(IRStmtAssign, get_def, /) { return self->dst; }
SliceUSize MTD(IRStmtAssign, get_use, /) {
    return (SliceUSize){
        .data = self->use_repr,
        .size = LENGTH(self->use_repr),
    };
}

// IRStmtArith
void MTD(IRStmtLoad, init, /, usize dst, usize src_addr) {
    CALL(IRStmtLoad, *self, base_init, /);
    self->dst = dst;
    self->src_addr = src_addr;
}
DEFAULT_DROPER(IRStmtLoad);
void MTD(IRStmtLoad, build_str, /, String *builder) {
    CALL(String, *builder, pushf, /, "v%zu := *v%zu", self->dst,
         self->src_addr);
}
usize MTD(IRStmtLoad, get_def, /) { return self->dst; }
SliceUSize MTD(IRStmtLoad, get_use, /) {
    return (SliceUSize){
        .data = self->use_repr,
        .size = LENGTH(self->use_repr),
    };
}

// IRStmtStore
void MTD(IRStmtStore, init, /, usize dst_addr, usize src) {
    CALL(IRStmtStore, *self, base_init, /);
    self->dst_addr = dst_addr;
    self->src = src;
}
DEFAULT_DROPER(IRStmtStore);
void MTD(IRStmtStore, build_str, /, String *builder) {
    CALL(String, *builder, pushf, /, "*v%zu := v%zu", self->dst_addr,
         self->src);
}
usize MTD(IRStmtStore, get_def, /) { return (usize)-1; }
SliceUSize MTD(IRStmtStore, get_use, /) {
    return (SliceUSize){
        .data = self->use_repr,
        .size = LENGTH(self->use_repr),
    };
}

// IRStmtArith
void MTD(IRStmtArith, init, /, usize dst, usize src1, usize src2,
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
    CALL(String, *builder, pushf, /, "v%zu := v%zu %s v%zu", self->dst,
         self->src1, ArithopKind_to_str(self->aop), self->src2);
}
usize MTD(IRStmtArith, get_def, /) { return self->dst; }
SliceUSize MTD(IRStmtArith, get_use, /) {
    return (SliceUSize){
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
    CALL(String, *builder, pushf, /, "GOTO L%zu", self->label);
}
usize MTD(IRStmtGoto, get_def, /) { return (usize)-1; }
SliceUSize MTD(IRStmtGoto, get_use, /) {
    return (SliceUSize){
        .data = NULL,
        .size = 0,
    };
}

// IRStmtIf
void MTD(IRStmtIf, init, /, usize src1, usize src2, RelopKind rop,
         usize label) {
    CALL(IRStmtIf, *self, base_init, /);
    self->src1 = src1;
    self->src2 = src2;
    self->rop = rop;
    self->label = label;
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
    CALL(String, *builder, pushf, /, "IF v%zu %s v%zu GOTO l%zu", self->src1,
         RelopKind_to_str(self->rop), self->src2, self->label);
}
usize MTD(IRStmtIf, get_def, /) { return (usize)-1; }
SliceUSize MTD(IRStmtIf, get_use, /) {
    return (SliceUSize){
        .data = self->use_repr,
        .size = LENGTH(self->use_repr),
    };
}

// IRStmtCall
void MTD(IRStmtCall, init, /, usize dst, String func_name, VecUSize args) {
    CALL(IRStmtCall, *self, base_init, /);
    self->dst = dst;
    self->func_name = func_name;
    self->args = args;
}
void MTD(IRStmtCall, drop, /) {
    DROPOBJ(VecUSize, self->args);
    DROPOBJ(String, self->func_name);
}
void MTD(IRStmtCall, build_str, /, String *builder) {
    const char *func_name = STRING_C_STR(self->func_name);
    CALL(String, *builder, pushf, /, "v%zu := CALL %s", self->dst, func_name);
}

usize MTD(IRStmtCall, get_def, /) { return self->dst; }
SliceUSize MTD(IRStmtCall, get_use, /) {
    return (SliceUSize){
        .data = self->args.data,
        .size = self->args.size,
    };
}

// IRStmtReturn
void MTD(IRStmtReturn, init, /, usize src) {
    CALL(IRStmtReturn, *self, base_init, /);
    self->src = src;
}
DEFAULT_DROPER(IRStmtReturn);
void MTD(IRStmtReturn, build_str, /, String *builder) {
    CALL(String, *builder, pushf, /, "RETURN v%zu", self->src);
}
usize MTD(IRStmtReturn, get_def, /) { return (usize)-1; }
SliceUSize MTD(IRStmtReturn, get_use, /) {
    return (SliceUSize){
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
    CALL(String, *builder, pushf, /, "READ v%zu", self->dst);
}
usize MTD(IRStmtRead, get_def, /) { return self->dst; }
SliceUSize MTD(IRStmtRead, get_use, /) {
    return (SliceUSize){
        .data = self->use_repr,
        .size = LENGTH(self->use_repr),
    };
}

// IRStmtWrite
void MTD(IRStmtWrite, init, /, usize src) {
    CALL(IRStmtWrite, *self, base_init, /);
    self->src = src;
}
DEFAULT_DROPER(IRStmtWrite);
void MTD(IRStmtWrite, build_str, /, String *builder) {
    CALL(String, *builder, pushf, /, "WRITE v%zu", self->src);
}
usize MTD(IRStmtWrite, get_def, /) { return (usize)-1; }
SliceUSize MTD(IRStmtWrite, get_use, /) {
    return (SliceUSize){
        .data = NULL,
        .size = 0,
    };
}

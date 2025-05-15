#pragma once

#include "ir_value.h"
#include "op.h"
#include "str.h"
#include "tem_list.h"
#include "tem_memory_primitive.h"
#include "utils.h"

typedef struct SliceIRValue {
    IRValue *data;
    usize size;
} SliceIRValue;

#define APPLY_IRSTMT_KIND(f)                                                   \
    f(Assign)     /* x := y */                                                 \
        f(Load)   /* x := *y */                                                \
        f(Store)  /* *x := y */                                                \
        f(Arith)  /* x := y aop z */                                           \
        f(Goto)   /* GOTO l */                                                 \
        f(If)     /* IF x rop y GOTO l */                                      \
        f(Call)   /* x := CALL f */                                            \
        f(Return) /* RETURN x */                                               \
        f(Read)   /* READ x */                                                 \
        f(Write)  /* WRITE y */

#define ENUM_IRSTMT_KIND_AID(kind) CONCATENATE(IRStmtKind, kind),
typedef enum IRStmtKind {
    IRStmtKindInvalid,
    APPLY_IRSTMT_KIND(ENUM_IRSTMT_KIND_AID)
} IRStmtKind;

struct IRStmtBase;

typedef struct IRStmtBaseVTable {
    void (*drop)(struct IRStmtBase *self);
    void (*build_str)(struct IRStmtBase *self, String *builder);
    usize (*get_def)(struct IRStmtBase *self);
    SliceIRValue (*get_use)(struct IRStmtBase *self);
} IRStmtBaseVTable;

typedef struct IRStmtBase {
    const IRStmtBaseVTable *vtable;

    // other
    IRStmtKind kind;
    bool is_dead;
} IRStmtBase;

typedef struct IRStmtAssign {
    IRStmtBase base;
    usize dst;
    union {
        IRValue use_repr[1];
        IRValue src;
    };
} IRStmtAssign;
void MTD(IRStmtAssign, init, /, usize dst, IRValue src);

typedef struct IRStmtLoad {
    IRStmtBase base;
    usize dst;
    union {
        IRValue use_repr[1];
        IRValue src_addr;
    };
} IRStmtLoad;
void MTD(IRStmtLoad, init, /, usize dst, IRValue src_addr);

typedef struct IRStmtStore {
    IRStmtBase base;
    union {
        IRValue use_repr[2];
        struct {
            IRValue dst_addr;
            IRValue src;
        };
    };
} IRStmtStore;
void MTD(IRStmtStore, init, /, IRValue dst_addr, IRValue src);

typedef struct IRStmtArith {
    IRStmtBase base;
    usize dst;
    union {
        IRValue use_repr[2];
        struct {
            IRValue src1;
            IRValue src2;
        };
    };
    ArithopKind aop;
} IRStmtArith;
void MTD(IRStmtArith, init, /, usize dst, IRValue src1, IRValue src2,
         ArithopKind aop);

typedef struct IRStmtGoto {
    IRStmtBase base;
    usize label;
} IRStmtGoto;
void MTD(IRStmtGoto, init, /, usize label);

typedef struct IRStmtIf {
    IRStmtBase base;
    union {
        IRValue use_repr[2];
        struct {
            IRValue src1;
            IRValue src2;
        };
    };
    RelopKind rop;
    usize true_label;
    usize false_label;
} IRStmtIf;
void MTD(IRStmtIf, init, /, IRValue src1, IRValue src2, RelopKind rop,
         usize label);
void MTD(IRStmtIf, flip, /);

typedef struct IRStmtCall {
    IRStmtBase base;
    usize dst;
    String func_name;
    VecIRValue args;
} IRStmtCall;
void MTD(IRStmtCall, init, /, usize dst, String func_name, VecIRValue args);

typedef struct IRStmtReturn {
    IRStmtBase base;
    union {
        IRValue use_repr[1];
        IRValue src;
    };
} IRStmtReturn;
void MTD(IRStmtReturn, init, /, IRValue src);

typedef struct IRStmtRead {
    IRStmtBase base;
    usize dst;
} IRStmtRead;
void MTD(IRStmtRead, init, /, usize dst);

typedef struct IRStmtWrite {
    IRStmtBase base;
    union {
        IRValue use_repr[1];
        IRValue src;
    };
} IRStmtWrite;
void MTD(IRStmtWrite, init, /, IRValue src);

#define DEFINE_IRSTMT_STRUCT(kindname, classname)                              \
    /* require define */                                                       \
    void MTD(classname, drop, /);                                              \
    void MTD(classname, build_str, /, String * builder);                       \
    usize MTD(classname, get_def, /);                                          \
    SliceIRValue MTD(classname, get_use, /);                                   \
                                                                               \
    /* auto gen */                                                             \
    FUNC_STATIC void VMTD(classname, v_drop, /) {                              \
        CALL(classname, *(classname *)self, drop, /);                          \
    }                                                                          \
    FUNC_STATIC void VMTD(classname, v_build_str, /, String * builder) {       \
        CALL(classname, *(classname *)self, build_str, /, builder);            \
    }                                                                          \
    FUNC_STATIC usize VMTD(classname, v_get_def, /) {                          \
        return CALL(classname, *(classname *)self, get_def, /);                \
    }                                                                          \
    FUNC_STATIC SliceIRValue VMTD(classname, v_get_use, /) {                   \
        return CALL(classname, *(classname *)self, get_use, /);                \
    }                                                                          \
    FUNC_STATIC void MTD(classname, base_init, /) {                            \
        static const IRStmtBaseVTable vtable = {                               \
            .drop = MTDNAME(classname, v_drop),                                \
            .build_str = MTDNAME(classname, v_build_str),                      \
            .get_def = MTDNAME(classname, v_get_def),                          \
            .get_use = MTDNAME(classname, v_get_use)};                         \
        self->base.vtable = &vtable;                                           \
        self->base.kind = kindname;                                            \
        self->base.is_dead = false;                                            \
    }

#define DEFINE_IRSTMT_STRUCT_AID(kind)                                         \
    DEFINE_IRSTMT_STRUCT(CONCATENATE(IRStmtKind, kind),                        \
                         CONCATENATE(IRStmt, kind))

APPLY_IRSTMT_KIND(DEFINE_IRSTMT_STRUCT_AID);

DECLARE_LIST(ListDynIRStmt, IRStmtBase *, FUNC_EXTERN, GENERATOR_CUSTOM_VALUE);
FUNC_STATIC void NSMTD(ListDynIRStmt, drop_value, /, IRStmtBase **value) {
    VDROPOBJHEAP(IRStmtBase, *value);
}
FUNC_STATIC IRStmtBase *NSMTD(ListDynIRStmt, clone_value, /,
                              ATTR_UNUSED IRStmtBase *const *other) {
    PANIC("disable clone");
}

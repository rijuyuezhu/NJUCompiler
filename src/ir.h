#pragma once

#include "general_vec.h"
#include "op.h"
#include "str.h"
#include "utils.h"

typedef struct SliceUSize {
    usize *data;
    usize size;
} SliceUSize;

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

typedef struct IRStmtBase {
    // virtual table
    void (*drop)(struct IRStmtBase *self);
    void (*build_str)(struct IRStmtBase *self, String *builder);
    usize (*get_def)(struct IRStmtBase *self);
    SliceUSize (*get_use)(struct IRStmtBase *self);

    // other
    IRStmtKind kind;
    bool is_dead;
} IRStmtBase;

typedef struct IRStmtAssign {
    IRStmtBase base;
    usize dst;
    union {
        usize use_repr[1];
        usize src;
    };
} IRStmtAssign;
void MTD(IRStmtAssign, init, /, usize dst, usize src);

typedef struct IRStmtLoad {
    IRStmtBase base;
    usize dst;
    union {
        usize use_repr[1];
        usize src_addr;
    };
} IRStmtLoad;

typedef struct IRStmtStore {
    IRStmtBase base;
    union {
        usize use_repr[2];
        struct {
            usize dst_addr;
            usize src;
        };
    };
} IRStmtStore;

typedef struct IRStmtArith {
    IRStmtBase base;
    usize dst;
    union {
        usize use_repr[2];
        struct {
            usize src1;
            usize src2;
        };
    };
    ArithopKind aop;
} IRStmtArith;

typedef struct IRStmtGoto {
    IRStmtBase base;
    usize label;
} IRStmtGoto;

typedef struct IRStmtIf {
    IRStmtBase base;
    union {
        usize use_repr[2];
        struct {
            usize src1;
            usize src2;
        };
    };
    RelopKind rop;
    usize label;
} IRStmtIf;

typedef struct IRStmtCall {
    IRStmtBase base;
    usize dst;
    String func_name;
    VecUSize args;
} IRStmtCall;

typedef struct IRStmtReturn {
    IRStmtBase base;
    union {
        usize use_repr[1];
        usize src;
    };
} IRStmtReturn;

typedef struct IRStmtRead {
    IRStmtBase base;
    union {
        usize use_repr[1];
        usize dst;
    };
} IRStmtRead;

typedef struct IRStmtWrite {
    IRStmtBase base;
    usize src;
} IRStmtWrite;

#define DEFINE_IRSTMT_STRUCT(kindname, classname)                              \
    /* require define */                                                       \
    void MTD(classname, drop, /);                                              \
    void MTD(classname, build_str, /, String * builder);                       \
    usize MTD(classname, get_def, /);                                          \
    SliceUSize MTD(classname, get_use, /);                                     \
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
    FUNC_STATIC SliceUSize VMTD(classname, v_get_use, /) {                     \
        return CALL(classname, *(classname *)self, get_use, /);                \
    }                                                                          \
    FUNC_STATIC void MTD(classname, base_init, /) {                            \
        self->base.drop = MTDNAME(classname, v_drop);                          \
        self->base.build_str = MTDNAME(classname, v_build_str);                \
        self->base.get_def = MTDNAME(classname, v_get_def);                    \
        self->base.get_use = MTDNAME(classname, v_get_use);                    \
        self->base.kind = kindname;                                            \
        self->base.is_dead = false;                                            \
    }

#define DEFINE_IRSTMT_STRUCT_AID(kind)                                         \
    DEFINE_IRSTMT_STRUCT(CONCATENATE(IRStmtKind, kind),                        \
                         CONCATENATE(IRStmt, kind))

APPLY_IRSTMT_KIND(DEFINE_IRSTMT_STRUCT_AID)

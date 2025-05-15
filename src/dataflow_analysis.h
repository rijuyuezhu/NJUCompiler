#pragma once
#include "ir_basic_block.h"
#include "ir_program.h"

typedef struct DataflowAnalysisBase DataflowAnalysisBase;

typedef struct DataflowAnalysisBaseVTable {
    void (*drop)(DataflowAnalysisBase *self);
    bool (*is_forward)(DataflowAnalysisBase *self);
    Any (*new_boundary_fact)(DataflowAnalysisBase *self, IRFunction *func);
    Any (*new_initial_fact)(DataflowAnalysisBase *self);
    void (*drop_fact)(DataflowAnalysisBase *self, Any fact);
    void (*set_in_fact)(DataflowAnalysisBase *self, IRBasicBlock *bb, Any fact);
    void (*set_out_fact)(DataflowAnalysisBase *self, IRBasicBlock *bb,
                         Any fact);
    Any (*get_in_fact)(DataflowAnalysisBase *self, IRBasicBlock *bb);
    Any (*get_out_fact)(DataflowAnalysisBase *self, IRBasicBlock *bb);
    bool (*meet_into)(DataflowAnalysisBase *self, Any fact, Any target);
    void (*transfer_stmt)(DataflowAnalysisBase *self, IRStmtBase *stmt,
                          Any fact);
    void (*debug_print)(DataflowAnalysisBase *self, IRFunction *func);
} DataflowAnalysisBaseVTable;

struct DataflowAnalysisBase {
    const DataflowAnalysisBaseVTable *vtable;
};

bool MTD(DataflowAnalysisBase, transfer_bb, /, IRBasicBlock *bb, Any in_fact,
         Any out_fact);

typedef void (*DAIterCallback)(DataflowAnalysisBase *analysis,
                               ListDynIRStmtNode *iter, Any fact);
void MTD(DataflowAnalysisBase, iter_bb, /, IRBasicBlock *bb,
         DAIterCallback callback);
void MTD(DataflowAnalysisBase, iter_func, /, IRFunction *func,
         DAIterCallback callback);

#define DEFINE_DATAFLOW_ANALYSIS_STRUCT(classname)                             \
    /* require define */                                                       \
    void MTD(classname, drop, /);                                              \
    bool MTD(classname, is_forward, /);                                        \
    Any MTD(classname, new_boundary_fact, /, IRFunction * func);               \
    Any MTD(classname, new_initial_fact, /);                                   \
    void MTD(classname, drop_fact, /, Any fact);                               \
    void MTD(classname, set_in_fact, /, IRBasicBlock * bb, Any fact);          \
    void MTD(classname, set_out_fact, /, IRBasicBlock * bb, Any fact);         \
    Any MTD(classname, get_in_fact, /, IRBasicBlock * bb);                     \
    Any MTD(classname, get_out_fact, /, IRBasicBlock * bb);                    \
    bool MTD(classname, meet_into, /, Any fact, Any target);                   \
    void MTD(classname, transfer_stmt, /, IRStmtBase * stmt, Any fact);        \
    void MTD(classname, debug_print, /, IRFunction * func);                    \
                                                                               \
    /* auto gen */                                                             \
    FUNC_STATIC void VMTD(classname, v_drop, /) {                              \
        CALL(classname, *(classname *)base_self, drop, /);                     \
    }                                                                          \
    FUNC_STATIC bool VMTD(classname, v_is_forward, /) {                        \
        return CALL(classname, *(classname *)base_self, is_forward, /);        \
    }                                                                          \
    FUNC_STATIC Any VMTD(classname, v_new_boundary_fact, /,                    \
                         IRFunction * func) {                                  \
        return CALL(classname, *(classname *)base_self, new_boundary_fact, /,  \
                    func);                                                     \
    }                                                                          \
    FUNC_STATIC Any VMTD(classname, v_new_initial_fact, /) {                   \
        return CALL(classname, *(classname *)base_self, new_initial_fact, /);  \
    }                                                                          \
    FUNC_STATIC void VMTD(classname, v_drop_fact, /, Any fact) {               \
        CALL(classname, *(classname *)base_self, drop_fact, /, fact);          \
    }                                                                          \
    FUNC_STATIC void VMTD(classname, v_set_in_fact, /, IRBasicBlock * bb,      \
                          Any fact) {                                          \
        CALL(classname, *(classname *)base_self, set_in_fact, /, bb, fact);    \
    }                                                                          \
    FUNC_STATIC void VMTD(classname, v_set_out_fact, /, IRBasicBlock * bb,     \
                          Any fact) {                                          \
        CALL(classname, *(classname *)base_self, set_out_fact, /, bb, fact);   \
    }                                                                          \
    FUNC_STATIC Any VMTD(classname, v_get_in_fact, /, IRBasicBlock * bb) {     \
        return CALL(classname, *(classname *)base_self, get_in_fact, /, bb);   \
    }                                                                          \
    FUNC_STATIC Any VMTD(classname, v_get_out_fact, /, IRBasicBlock * bb) {    \
        return CALL(classname, *(classname *)base_self, get_out_fact, /, bb);  \
    }                                                                          \
    FUNC_STATIC bool VMTD(classname, v_meet_into, /, Any fact, Any target) {   \
        return CALL(classname, *(classname *)base_self, meet_into, /, fact,    \
                    target);                                                   \
    }                                                                          \
    FUNC_STATIC void VMTD(classname, v_transfer_stmt, /, IRStmtBase * stmt,    \
                          Any fact) {                                          \
        CALL(classname, *(classname *)base_self, transfer_stmt, /, stmt,       \
             fact);                                                            \
    }                                                                          \
    FUNC_STATIC void VMTD(classname, v_debug_print, /, IRFunction * func) {    \
        CALL(classname, *(classname *)base_self, debug_print, /, func);        \
    }                                                                          \
                                                                               \
    FUNC_STATIC void MTD(classname, base_init, /) {                            \
        static const DataflowAnalysisBaseVTable vtable = {                     \
            .drop = MTDNAME(classname, v_drop),                                \
            .is_forward = MTDNAME(classname, v_is_forward),                    \
            .new_boundary_fact = MTDNAME(classname, v_new_boundary_fact),      \
            .new_initial_fact = MTDNAME(classname, v_new_initial_fact),        \
            .drop_fact = MTDNAME(classname, v_drop_fact),                      \
            .set_in_fact = MTDNAME(classname, v_set_in_fact),                  \
            .set_out_fact = MTDNAME(classname, v_set_out_fact),                \
            .get_in_fact = MTDNAME(classname, v_get_in_fact),                  \
            .get_out_fact = MTDNAME(classname, v_get_out_fact),                \
            .meet_into = MTDNAME(classname, v_meet_into),                      \
            .transfer_stmt = MTDNAME(classname, v_transfer_stmt),              \
            .debug_print = MTDNAME(classname, v_debug_print),                  \
        };                                                                     \
        self->base.vtable = &vtable;                                           \
    }

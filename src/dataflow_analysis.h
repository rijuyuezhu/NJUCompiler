#include "ir_basic_block.h"
#include "ir_program.h"

typedef struct DataflowAnalysisBase DataflowAnalysisBase;

typedef struct DataflowAnalysisBaseVTable {
    void (*drop)(DataflowAnalysisBase *self);
    bool (*is_forward)(DataflowAnalysisBase *self);
    Any (*new_boundary_fact)(DataflowAnalysisBase *self, IRFunction *func);
    Any (*new_initial_fact)(DataflowAnalysisBase *self);
    void (*set_in_fact)(DataflowAnalysisBase *self, IRBasicBlock *bb, Any fact);
    void (*set_out_fact)(DataflowAnalysisBase *self, IRBasicBlock *bb,
                         Any fact);
    Any (*get_in_fact)(DataflowAnalysisBase *self, IRBasicBlock *bb);
    Any (*get_out_fact)(DataflowAnalysisBase *self, IRBasicBlock *bb);
    bool (*meet_into)(DataflowAnalysisBase *self, Any fact, Any target);
    bool (*transfer_bb)(DataflowAnalysisBase *self, Any in_fact, Any out_fact);
    void (*print_result)(DataflowAnalysisBase *self, IRFunction *func);
} DataflowAnalysisBaseVTable;

struct DataflowAnalysisBase {
    const DataflowAnalysisBaseVTable *vtable;
};

#define DEFINE_DATAFLOW_ANALYSIS_STRUCT(classname)                             \
    /* require define */                                                       \
    void MTD(classname, drop, /);                                              \
    bool MTD(classname, is_forward, /);                                        \
    Any MTD(classname, new_boundary_fact, /, IRFunction * func);               \
    Any MTD(classname, new_initial_fact, /);                                   \
    void MTD(classname, set_in_fact, /, IRBasicBlock * bb, Any fact);          \
    void MTD(classname, set_out_fact, /, IRBasicBlock * bb, Any fact);         \
    Any MTD(classname, get_in_fact, /, IRBasicBlock * bb);                     \
    Any MTD(classname, get_out_fact, /, IRBasicBlock * bb);                    \
    bool MTD(classname, meet_into, /, Any fact, Any target);                   \
    bool MTD(classname, transfer_bb, /, Any in_fact, Any out_fact);            \
    void MTD(classname, print_result, /, IRFunction * func);                   \
                                                                               \
    /* auto gen */                                                             \
    FUNC_STATIC void VMTD(classname, v_drop, /) {                              \
        CALL(classname, *(classname *)self, drop, /);                          \
    }                                                                          \
    FUNC_STATIC bool VMTD(classname, v_is_forward, /) {                        \
        return CALL(classname, *(classname *)self, is_forward, /);             \
    }                                                                          \
    FUNC_STATIC Any VMTD(classname, v_new_boundary_fact, /,                    \
                         IRFunction * func) {                                  \
        return CALL(classname, *(classname *)self, new_boundary_fact, /,       \
                    func);                                                     \
    }                                                                          \
    FUNC_STATIC Any VMTD(classname, v_new_initial_fact, /) {                   \
        return CALL(classname, *(classname *)self, new_initial_fact, /);       \
    }                                                                          \
    FUNC_STATIC void VMTD(classname, v_set_in_fact, /, IRBasicBlock * bb,      \
                          Any fact) {                                          \
        CALL(classname, *(classname *)self, set_in_fact, /, bb, fact);         \
    }                                                                          \
    FUNC_STATIC void VMTD(classname, v_set_out_fact, /, IRBasicBlock * bb,     \
                          Any fact) {                                          \
        CALL(classname, *(classname *)self, set_out_fact, /, bb, fact);        \
    }                                                                          \
    FUNC_STATIC Any VMTD(classname, v_get_in_fact, /, IRBasicBlock * bb) {     \
        return CALL(classname, *(classname *)self, get_in_fact, /, bb);        \
    }                                                                          \
    FUNC_STATIC Any VMTD(classname, v_get_out_fact, /, IRBasicBlock * bb) {    \
        return CALL(classname, *(classname *)self, get_out_fact, /, bb);       \
    }                                                                          \
    FUNC_STATIC bool VMTD(classname, v_meet_into, /, Any fact, Any target) {   \
        return CALL(classname, *(classname *)self, meet_into, /, fact,         \
                    target);                                                   \
    }                                                                          \
    FUNC_STATIC bool VMTD(classname, v_transfer_bb, /, Any in_fact,            \
                          Any out_fact) {                                      \
        return CALL(classname, *(classname *)self, transfer_bb, /, in_fact,    \
                    out_fact);                                                 \
    }                                                                          \
    FUNC_STATIC void VMTD(classname, v_print_result, /, IRFunction * func) {   \
        CALL(classname, *(classname *)self, print_result, /, func);            \
    }                                                                          \
                                                                               \
    FUNC_STATIC void MTD(classname, base_init, /) {                            \
        static const DataflowAnalysisBaseVTable vtable = {                     \
            .drop = MTDNAME(classname, v_drop),                                \
            .is_forward = MTDNAME(classname, v_is_forward),                    \
            .new_boundary_fact = MTDNAME(classname, v_new_boundary_fact),      \
            .new_initial_fact = MTDNAME(classname, v_new_initial_fact),        \
            .set_in_fact = MTDNAME(classname, v_set_in_fact),                  \
            .set_out_fact = MTDNAME(classname, v_set_out_fact),                \
            .get_in_fact = MTDNAME(classname, v_get_in_fact),                  \
            .get_out_fact = MTDNAME(classname, v_get_out_fact),                \
            .meet_into = MTDNAME(classname, v_meet_into),                      \
            .transfer_bb = MTDNAME(classname, v_transfer_bb),                  \
            .print_result = MTDNAME(classname, v_print_result),                \
        };                                                                     \
        self->base.vtable = &vtable;                                           \
    }

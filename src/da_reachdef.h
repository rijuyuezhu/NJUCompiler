#pragma once

#include "dataflow_analysis.h"
#include "general_container.h"

typedef struct RDFact {
    MapUSizeToSetPtr reach_defs;
} RDFact;

struct IRStmtBase;
struct IRBasicBlock;
struct MapUSizeToDynIRStmt;

void MTD(RDFact, init, /);
void MTD(RDFact, drop, /);
SetPtr *MTD(RDFact, get, /, usize var);
void MTD(RDFact, redef, /, usize var, struct IRStmtBase *bb);
void MTD(RDFact, debug_print, /);

DECLARE_MAPPING(MapBBToRDFact, struct IRBasicBlock *, RDFact *, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_CUSTOM_VALUE,
                GENERATOR_PLAIN_COMPARATOR);
FUNC_STATIC void NSMTD(MapBBToRDFact, drop_value, /, RDFact **value) {
    DROPOBJHEAP(RDFact, *value);
}
FUNC_STATIC RDFact *NSMTD(MapBBToRDFact, clone_value, /,
                          ATTR_UNUSED RDFact *const *other) {
    PANIC("Disable clone");
}

typedef struct ReachDefDA {
    DataflowAnalysisBase base;

    struct MapUSizeToDynIRStmt *param_to_stmt;
    MapBBToRDFact in_facts;
    MapBBToRDFact out_facts;
} ReachDefDA;
void MTD(ReachDefDA, init, /, struct MapUSizeToDynIRStmt *param_to_stmt);

DEFINE_DATAFLOW_ANALYSIS_STRUCT(ReachDefDA);

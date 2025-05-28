#pragma once
#include "dataflow_analysis.h"
#include "general_container.h"
#include "ir_basic_block.h"
#include "tem_map.h"

typedef struct LVFact {
    SetUSize live_vars;
} LVFact;

void MTD(LVFact, init, /);
void MTD(LVFact, drop, /);
bool MTD(LVFact, get, /, usize key);
bool MTD(LVFact, set, /, usize key, bool value);
void MTD(LVFact, debug_print, /);

DECLARE_MAPPING(MapBBToLVFact, IRBasicBlock *, LVFact *, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_CUSTOM_VALUE,
                GENERATOR_PLAIN_COMPARATOR);
FUNC_STATIC void NSMTD(MapBBToLVFact, drop_value, /, LVFact **value) {
    DROPOBJHEAP(LVFact, *value);
}
FUNC_STATIC LVFact *NSMTD(MapBBToLVFact, clone_value, /,
                          ATTR_UNUSED LVFact *const *other) {
    PANIC("Disable clone");
}

typedef struct LiveVarDA {
    DataflowAnalysisBase base;

    MapBBToLVFact in_facts;
    MapBBToLVFact out_facts;
} LiveVarDA;

void MTD(LiveVarDA, init, /);
void MTD(LiveVarDA, dead_code_eliminate_bb, /, IRBasicBlock *bb);
void MTD(LiveVarDA, dead_code_eliminate_func_meta, /, IRFunction *func);
DEFINE_DATAFLOW_ANALYSIS_STRUCT(LiveVarDA);

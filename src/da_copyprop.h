#pragma once
#include "dataflow_analysis.h"
#include "general_container.h"

typedef struct CYPFact {
    bool is_universal;
    MapUSizeUSize dst_to_src;
    MapUSizeToSetUSize src_to_dst;
} CYPFact;
void MTD(CYPFact, init, /);
void MTD(CYPFact, drop, /);
bool MTD(CYPFact, kill_dst, /, usize dst);
bool MTD(CYPFact, kill_src, /, usize src);
void MTD(CYPFact, add_assign, /, usize dst, usize src);
usize MTD(CYPFact, get_src, /, usize dst);
void MTD(CYPFact, debug_print, /);

DECLARE_MAPPING(MapBBToCYPFact, IRBasicBlock *, CYPFact *, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_CUSTOM_VALUE,
                GENERATOR_PLAIN_COMPARATOR);
FUNC_STATIC void NSMTD(MapBBToCYPFact, drop_value, /, CYPFact **value) {
    DROPOBJHEAP(CYPFact, *value);
}
FUNC_STATIC CYPFact *NSMTD(MapBBToCYPFact, clone_value, /,
                           ATTR_UNUSED CYPFact *const *other) {
    PANIC("Disable clone");
}

typedef struct CopyPropDA {
    DataflowAnalysisBase base;

    MapBBToCYPFact in_facts;
    MapBBToCYPFact out_facts;
} CopyPropDA;
void MTD(CopyPropDA, init, /);
bool MTD(CopyPropDA, copy_propagate, /, IRFunction *func);
DEFINE_DATAFLOW_ANALYSIS_STRUCT(CopyPropDA);

#pragma once

#include "dataflow_analysis.h"
#include "general_container.h"
#include "ir_function.h"
#include "tem_map.h"

typedef struct DomFact {
    bool is_universal;
    SetPtr doms; // the set of dominators (ptr to bbs)
} DomFact;

void MTD(DomFact, init, /);
void MTD(DomFact, drop, /);
bool MTD(DomFact, get, /, IRBasicBlock *key);
bool MTD(DomFact, set, /, IRBasicBlock *key, bool value);

DECLARE_MAPPING(MapBBToDomFact, IRBasicBlock *, DomFact *, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_CUSTOM_VALUE,
                GENERATOR_PLAIN_COMPARATOR);

FUNC_STATIC void NSMTD(MapBBToDomFact, drop_value, /, DomFact **value) {
    DROPOBJHEAP(DomFact, *value);
}
FUNC_STATIC DomFact *NSMTD(MapBBToDomFact, clone_value, /,
                           ATTR_UNUSED DomFact *const *other) {
    PANIC("Disable clone");
}

typedef struct DominatorDA {
    DataflowAnalysisBase base;

    MapPtrPtr *stmt_to_bb;
    MapBBToDomFact in_facts;
    MapBBToDomFact out_facts;
} DominatorDA;
void MTD(DominatorDA, init, /, MapPtrPtr *stmt_to_bb);
void MTD(DominatorDA, prepare, /, IRFunction *func);
DEFINE_DATAFLOW_ANALYSIS_STRUCT(DominatorDA);

#pragma once

#include "dataflow_analysis.h"
#include "general_container.h"

struct IRBasicBlock;
struct IRStmtBase;
struct IRFunction;

typedef struct DomFact {
    bool is_universal;
    SetPtr doms; // the set of dominators (ptr to bbs)
} DomFact;

void MTD(DomFact, init, /, bool is_universal);
void MTD(DomFact, drop, /);
bool MTD(DomFact, get, /, struct IRBasicBlock *key);
bool MTD(DomFact, set, /, struct IRBasicBlock *key, bool value);

DECLARE_MAPPING(MapBBToDomFact, struct IRBasicBlock *, DomFact *, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_CUSTOM_VALUE,
                GENERATOR_PLAIN_COMPARATOR);

FUNC_STATIC void NSMTD(MapBBToDomFact, drop_value, /, DomFact **value) {
    DROPOBJHEAP(DomFact, *value);
}
FUNC_STATIC DomFact *NSMTD(MapBBToDomFact, clone_value, /,
                           ATTR_UNUSED DomFact *const *other) {
    PANIC("Disable clone");
}

typedef struct BBInfo {
    struct IRBasicBlock *bb;
    usize idx;
} BBInfo;

DECLARE_MAPPING(MapStmtToBBInfo, struct IRStmtBase *, BBInfo, FUNC_EXTERN,
                GENERATOR_PLAIN_KEY, GENERATOR_PLAIN_VALUE,
                GENERATOR_PLAIN_COMPARATOR);

typedef struct DominatorDA {
    DataflowAnalysisBase base;

    MapStmtToBBInfo *stmt_to_bb_info;
    MapBBToDomFact in_facts;
    MapBBToDomFact out_facts;
} DominatorDA;
void MTD(DominatorDA, init, /, MapStmtToBBInfo *stmt_to_bb_info);
void MTD(DominatorDA, prepare, /, struct IRFunction *func);
DEFINE_DATAFLOW_ANALYSIS_STRUCT(DominatorDA);
